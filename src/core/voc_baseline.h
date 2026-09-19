#pragma once

namespace roomsense {

// The two numbers the SGP41 gas index algorithm uses to carry its learned
// baseline across a restart.
struct VocBaseline {
  float state0 = 0;
  float state1 = 0;
};

// Reads the stored baseline. False if nothing was stored yet or NVS failed,
// `baseline` is then left untouched.
bool LoadVocBaseline(VocBaseline& baseline);

// Stores the baseline. False if NVS failed.
bool SaveVocBaseline(const VocBaseline& baseline);

}  // namespace roomsense
