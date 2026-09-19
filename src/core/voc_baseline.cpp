#include "core/voc_baseline.h"

#include <cmath>

#include <Preferences.h>

#include "core/settings.h"

namespace roomsense {
namespace {

// Same NVS namespace as the settings, so a factory reset drops the learned
// baseline together with everything else.
constexpr const char* kState0 = "voc_state0";
constexpr const char* kState1 = "voc_state1";

}  // namespace

bool LoadVocBaseline(VocBaseline& baseline) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/true)) {
    return false;
  }
  const float state0 = prefs.getFloat(kState0, NAN);
  const float state1 = prefs.getFloat(kState1, NAN);
  prefs.end();

  if (std::isnan(state0) || std::isnan(state1)) {
    return false;
  }
  baseline.state0 = state0;
  baseline.state1 = state1;
  return true;
}

bool SaveVocBaseline(const VocBaseline& baseline) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/false)) {
    return false;
  }
  bool written = prefs.putFloat(kState0, baseline.state0) != 0;
  written = prefs.putFloat(kState1, baseline.state1) != 0 && written;
  prefs.end();
  return written;
}

}  // namespace roomsense
