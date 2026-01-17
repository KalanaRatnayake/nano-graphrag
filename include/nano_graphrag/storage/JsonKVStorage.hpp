#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

#include "nano_graphrag/storage/Base.hpp"

namespace nano_graphrag {

template <typename T>
class JsonKVStorage : public BaseKVStorage<T> {
 public:
  using Map = std::unordered_map<std::string, T>;

  explicit JsonKVStorage(const std::string& ns = "", const std::unordered_map<std::string, std::string>& cfg = {}) {
    this->namespace_name = ns; this->global_config = cfg;
  }

  std::vector<std::string> all_keys() override {
    std::vector<std::string> keys; keys.reserve(data_.size());
    for (auto& kv : data_) keys.push_back(kv.first);
    return keys;
  }

  std::optional<T> get_by_id(const std::string& id) override {
    auto it = data_.find(id);
    if (it == data_.end()) return std::nullopt;
    return it->second;
  }

  std::vector<std::optional<T>> get_by_ids(const std::vector<std::string>& ids) override {
    std::vector<std::optional<T>> out; out.reserve(ids.size());
    for (auto& id : ids) out.push_back(get_by_id(id));
    return out;
  }

  std::vector<std::string> filter_keys(const std::vector<std::string>& ids) override {
    std::vector<std::string> missing;
    for (auto& id : ids) if (data_.find(id) == data_.end()) missing.push_back(id);
    return missing;
  }

  void upsert(const std::unordered_map<std::string, T>& data) override {
    for (auto& kv : data) data_[kv.first] = kv.second;
  }

  void drop() override { data_.clear(); }

 private:
  Map data_{};
};

} // namespace nano_graphrag
