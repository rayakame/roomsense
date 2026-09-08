#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace roomsense {

struct SosSection {
  float b1;
  float b2;
  float a1;
  float a2;
};

template <std::size_t N>
class SosFilter {
 public:
  SosFilter(float gain, const std::array<SosSection, N>& sections)
      : gain_(gain), sections_(sections) {}

  void Process(std::span<float> samples) {
    for (float& x : samples) {
      for (std::size_t i = 0; i < N; ++i) {
        const SosSection& c = sections_[i];
        State& s = state_[i];
        const float w = x + c.a1 * s.w0 + c.a2 * s.w1;
        x = w + c.b1 * s.w0 + c.b2 * s.w1;
        s.w1 = s.w0;
        s.w0 = w;
      }
      x *= gain_;
    }
  }

  void Reset() { state_ = {}; }

 private:
  struct State {
    float w0 = 0;
    float w1 = 0;
  };

  float gain_;
  std::array<SosSection, N> sections_;
  std::array<State, N> state_{};
};

}  // namespace roomsense