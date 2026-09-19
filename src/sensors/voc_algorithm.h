#pragma once

#include <VOCGasIndexAlgorithm.h>
#include <algorithm/sensirion_gas_index_algorithm.h>

namespace roomsense {

// VOCGasIndexAlgorithm with access to its learned baseline. The library keeps
// the parameter block protected, hence the subclass. VOC only; NOx has no
// equivalent and always starts from scratch.
class VocAlgorithm : public VOCGasIndexAlgorithm {
 public:
  // Only meaningful after kVocBaselineMinRuntimeMs of continuous operation.
  void GetStates(float& state0, float& state1) const {
    GasIndexAlgorithm_get_states(params, &state0, &state1);
  }

  // Once after construction, before the first process() call.
  void SetStates(float state0, float state1) {
    GasIndexAlgorithm_set_states(params, state0, state1);
  }
};

}  // namespace roomsense
