#pragma once

#include <cstdint>

namespace roomsense {

// True once `now` has reached `deadline`. Both are millis() timestamps; the
// comparison is robust against the counter wrapping around.
inline bool Reached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

}  // namespace roomsense
