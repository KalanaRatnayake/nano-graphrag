#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <nlohmann/json.hpp>

#include "nano_graphrag/storage/base.hpp"

namespace nano_graphrag
{

/**
 * @brief Simple in-memory JSON-like key-value storage.
 *
 * Stores typed values `T` keyed by string ids. This backend is non-persistent
 * (lives in memory) and intended for prototyping or as a cache layer for
 * documents, chunks, and community reports.
 */
template <typename T>
class JsonKVStorage : public BaseKVStorage<T>
{
public:
  using Map = std::unordered_map<std::string, T>;

  explicit JsonKVStorage(const std::string& ns = "",
                         const std::unordered_map<std::string, std::string>& cfg = {})
  {
    this->namespace_name = ns;
    this->global_config = cfg;
    std::string dir = cfg.count("working_dir") ? cfg.at("working_dir") : std::string("./nano_cache");
    storage_file_ = dir + "/" + ns + ".json";
    load();
  }

  /**
   * @brief Return all keys stored.
   */
  std::vector<std::string> all_keys() override
  {
    std::vector<std::string> keys;
    keys.reserve(data_.size());
    for (auto& kv : data_)
      keys.push_back(kv.first);
    return keys;
  }

  /**
   * @brief Get a single value by id.
   */
  std::optional<T> get_by_id(const std::string& id) override
  {
    auto it = data_.find(id);
    if (it == data_.end())
      return std::nullopt;
    return it->second;
  }

  /**
   * @brief Batch get values by ids.
   */
  std::vector<std::optional<T>> get_by_ids(const std::vector<std::string>& ids) override
  {
    std::vector<std::optional<T>> out;
    out.reserve(ids.size());
    for (auto& id : ids)
      out.push_back(get_by_id(id));
    return out;
  }

  /**
   * @brief Return ids that are missing from the store.
   */
  std::vector<std::string> filter_keys(const std::vector<std::string>& ids) override
  {
    std::vector<std::string> missing;
    for (auto& id : ids)
      if (data_.find(id) == data_.end())
        missing.push_back(id);
    return missing;
  }

  /**
   * @brief Upsert batch id->value pairs.
   */
  void upsert(const std::unordered_map<std::string, T>& data) override
  {
    for (auto& kv : data)
      data_[kv.first] = kv.second;
    save();
  }

  /**
   * @brief Clear all stored data.
   */
  void drop() override
  {
    data_.clear();
    save();
  }

private:
  Map data_{};
  std::string storage_file_;

  void save()
  {
    nlohmann::json j;
    for (const auto& kv : data_)
    {
      j[kv.first] = to_json(kv.second);
    }
    std::ofstream f(storage_file_);
    if (f.is_open())
      f << j.dump(2);
  }

  void load()
  {
    std::ifstream f(storage_file_);
    if (!f.is_open())
      return;
    nlohmann::json j;
    try
    {
      f >> j;
      for (auto it = j.begin(); it != j.end(); ++it)
      {
        data_[it.key()] = from_json(it.value());
      }
    }
    catch (...)
    {
      // ignore errors
    }
  }

  // Default to_json/from_json for types that are themselves serializable
  template <typename U = T>
  static nlohmann::json to_json(const U& value)
  {
    nlohmann::json j;
    j["value"] = value;
    return j;
  }

  template <typename U = T>
  static U from_json(const nlohmann::json& j)
  {
    return j.at("value").get<U>();
  }
};

}  // namespace nano_graphrag
