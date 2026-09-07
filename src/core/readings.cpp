#include "readings.h"


void ReadingsStore::update(const std::function<void(Readings &)> &fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    fn(data_);
}

Readings ReadingsStore::snapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    Readings copy = data_;
    return copy;
}
