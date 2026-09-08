#include "core/readings.h"

#include <functional>
#include <mutex>

namespace roomsense {

ReadingsStore& ReadingsStore::Instance() {
  // Intentionally never destroyed: objects with static storage duration must be
  // trivially destructible, which std::mutex is not.
  static ReadingsStore& store = *new ReadingsStore();
  return store;
}

void ReadingsStore::Update(const std::function<void(Readings&)>& updater) {
  const std::scoped_lock lock(mutex_);
  updater(data_);
}

Readings ReadingsStore::Snapshot() const {
  const std::scoped_lock lock(mutex_);
  return data_;
}

}  // namespace roomsense
