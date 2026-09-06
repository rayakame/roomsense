#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_DPS310.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include <sps30.h>
#include "driver/i2s_std.h"

Adafruit_SHT4x sht45;
Adafruit_VEML7700 veml;
Adafruit_DPS310 dps;
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm vocAlgo;
NOxGasIndexAlgorithm noxAlgo;

// ---- Mikrofon (ICS-43434 via I2S) ----
static const gpio_num_t MIC_BCLK = GPIO_NUM_5;
static const gpio_num_t MIC_WS   = GPIO_NUM_6;
static const gpio_num_t MIC_DIN  = GPIO_NUM_9;
static const uint32_t MIC_SAMPLE_RATE = 48000;
static const int MIC_SAMPLES = 4800;            // 100 ms pro Messung
static int32_t micBuf[MIC_SAMPLES];
i2s_chan_handle_t micRx = nullptr;

void micInit() {
    i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chanCfg, nullptr, &micRx);

    i2s_std_config_t stdCfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_BCLK,
            .ws   = MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = MIC_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    stdCfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;   // SEL auf GND = linker Kanal

    i2s_channel_init_std_mode(micRx, &stdCfg);
    i2s_channel_enable(micRx);
}

// Liest 100 ms Audio und gibt den unbewerteten Schallpegel in dB SPL zurueck.
// ICS-43434: 24 Bit Daten in 32-Bit-Slot, -26 dBFS bei 94 dB SPL.
float readMicDb() {
    size_t bytesRead = 0;
    i2s_channel_read(micRx, micBuf, sizeof(micBuf), &bytesRead, pdMS_TO_TICKS(500));
    int n = bytesRead / sizeof(int32_t);
    if (n == 0) return -1.0f;

    double sum = 0;
    for (int i = 0; i < n; i++) sum += (micBuf[i] >> 8);
    double mean = sum / n;
    double sq = 0;
    for (int i = 0; i < n; i++) {
        double s = (micBuf[i] >> 8) - mean;
        sq += s * s;
    }
    double rms = sqrt(sq / n);
    double dbfs = 20.0 * log10(rms / 8388608.0);   // 2^23 = Vollausschlag 24 Bit
    return (float)(dbfs + 94.0 + 26.0);
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    delay(2000);

    pinMode(I2C_POWER, OUTPUT);
    digitalWrite(I2C_POWER, HIGH);
    delay(100);

    Wire.begin();

    if (!sht45.begin(&Wire)) {
        Serial.println("SHT45 not found");
        while (true) delay(1000);
    }
    sht45.setPrecision(SHT4X_HIGH_PRECISION);
    sht45.setHeater(SHT4X_NO_HEATER);

    if (!veml.begin(&Wire)) {
        Serial.println("VEML7700 not found");
        while (true) delay(1000);
    }

    if (!dps.begin_I2C(DPS310_I2CADDR_DEFAULT, &Wire)) {
        Serial.println("DPS310 not found");
        while (true) delay(1000);
    }
    dps.configurePressure(DPS310_4HZ, DPS310_64SAMPLES);
    dps.configureTemperature(DPS310_4HZ, DPS310_64SAMPLES);

    sgp41.begin(Wire);
    uint16_t sgpSerial[3];
    if (sgp41.getSerialNumber(sgpSerial) != 0) {
        Serial.println("SGP41 not found");
        while (true) delay(1000);
    }

    sensirion_i2c_init();
    if (sps30_probe() != 0) {
        Serial.println("SPS30 not found");
        while (true) delay(1000);
    }
    sps30_set_fan_auto_cleaning_interval_days(4);
    if (sps30_start_measurement() < 0) {
        Serial.println("SPS30 start failed");
        while (true) delay(1000);
    }

    micInit();

    Serial.println("SGP41 conditioning...");
    uint16_t srawVoc = 0;
    for (int i = 0; i < 10; i++) {
        sgp41.executeConditioning(0x8000, 0x6666, srawVoc);
        delay(1000);
    }

    Serial.println("sensors ready");
}

void loop() {
    sensors_event_t hum, temp;
    sht45.getEvent(&hum, &temp);

    float lux = veml.readLux(VEML_LUX_AUTO);

    sensors_event_t dpsTemp, pressure;
    dps.getEvents(&dpsTemp, &pressure);

    uint16_t compRh = (uint16_t)(hum.relative_humidity * 65535.0f / 100.0f);
    uint16_t compT  = (uint16_t)((temp.temperature + 45.0f) * 65535.0f / 175.0f);
    uint16_t srawVoc = 0, srawNox = 0;
    int32_t vocIndex = 0, noxIndex = 0;
    if (sgp41.measureRawSignals(compRh, compT, srawVoc, srawNox) == 0) {
        vocIndex = vocAlgo.process(srawVoc);
        noxIndex = noxAlgo.process(srawNox);
    }

    struct sps30_measurement pm = {};
    uint16_t pmReady = 0;
    sps30_read_data_ready(&pmReady);
    if (pmReady) sps30_read_measurement(&pm);

    float db = readMicDb();

    Serial.printf("T %.2f C  RH %.2f %%  Light %.1f lux  P %.2f hPa  VOC %ld  NOx %ld  "
                  "PM2.5 %.1f  PM10 %.1f  Noise %.1f dB\n",
                  temp.temperature, hum.relative_humidity, lux, pressure.pressure,
                  vocIndex, noxIndex, pm.mc_2p5, pm.mc_10p0, db);
    delay(1000);
}