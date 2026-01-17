#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <cmath>
#include <algorithm>
#include <memory>

#include "NanoVectorDB.hpp"

#include "nano_graphrag/storage/Base.hpp"

namespace nano_graphrag
{

class NanoVectorDBStorage : public BaseVectorStorage
{
public:
  explicit NanoVectorDBStorage(const std::string& ns = "",
                               const std::unordered_map<std::string, std::string>& cfg = {},
                               const EmbeddingFunc& emb = {})
  {
    this->namespace_name = ns;
    this->global_config = cfg;
    this->embedding_func = emb;
    cosine_better_than_threshold_ = parse_double(cfg, "query_better_than_threshold", 0.2);
    std::string metric = cfg.count("metric") ? cfg.at("metric") : std::string("cosine");
    std::string storage_file = cfg.count("storage_file") ? cfg.at("storage_file") : std::string("nano-vectordb.json");
    if (embedding_func.embedding_dim > 0)
    {
      db_ = std::make_unique<nano_vectordb::NanoVectorDB>(static_cast<int>(embedding_func.embedding_dim), metric, storage_file);
    }
  }

  void upsert(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& data) override
  {
    if (data.empty())
      return;
    std::vector<std::string> contents; contents.reserve(data.size());
    std::vector<std::string> ids; ids.reserve(data.size());
    for (const auto& kv : data)
    {
      ids.push_back(kv.first);
      auto itc = kv.second.find("content");
      contents.push_back(itc != kv.second.end() ? itc->second : std::string{});
      // capture meta fields
      std::unordered_map<std::string, std::string> meta;
      for (const auto& mf : meta_fields)
      {
        if (mf.second)
        {
          auto itm = kv.second.find(mf.first);
          if (itm != kv.second.end()) meta[mf.first] = itm->second;
        }
      }
      metas_[kv.first] = std::move(meta);
    }
    auto embeddings = embedding_func(contents);
    if (embeddings.size() != ids.size())
    {
      embeddings.assign(ids.size(), std::vector<float>(embedding_func.embedding_dim, 0.0f));
    }
    if (!db_ && embedding_func.embedding_dim > 0)
    {
      std::string metric = global_config.count("metric") ? global_config.at("metric") : std::string("cosine");
      std::string storage_file = global_config.count("storage_file") ? global_config.at("storage_file") : std::string("nano-vectordb.json");
      db_ = std::make_unique<nano_vectordb::NanoVectorDB>(static_cast<int>(embedding_func.embedding_dim), metric, storage_file);
    }
    std::vector<nano_vectordb::Data> datas;
    datas.reserve(ids.size());
    for (size_t i = 0; i < ids.size(); ++i)
    {
      nano_vectordb::Vector v(static_cast<int>(embeddings[i].size()));
      for (int j = 0; j < v.size(); ++j) v[j] = embeddings[i][j];
      datas.push_back({ ids[i], std::move(v) });
    }
    if (db_) db_->upsert(datas);
  }

  std::vector<std::unordered_map<std::string, std::string>> query(const std::string& query, int top_k) override
  {
    auto qembs = embedding_func(std::vector<std::string>{ query });
    std::vector<float> q;
    if (!qembs.empty()) q = qembs[0]; else q.assign(embedding_func.embedding_dim, 0.0f);
    std::vector<std::unordered_map<std::string, std::string>> out;
    if (db_)
    {
      nano_vectordb::Vector v(static_cast<int>(q.size()));
      for (int i = 0; i < v.size(); ++i) v[i] = q[i];
      std::optional<nano_vectordb::Float> th = std::nullopt;
      if (cosine_better_than_threshold_ > 0.0) th = static_cast<nano_vectordb::Float>(cosine_better_than_threshold_);
      auto results = db_->query(v, top_k, th);
      out.reserve(results.size());
      for (const auto& r : results)
      {
        std::unordered_map<std::string, std::string> row = metas_[r.data.id];
        row["id"] = r.data.id;
        row["similarity"] = std::to_string(r.score);
        out.push_back(std::move(row));
      }
      return out;
    }
    return {};
  }

private:
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> metas_;
  double cosine_better_than_threshold_{ 0.2 };
  std::unique_ptr<nano_vectordb::NanoVectorDB> db_;

  static inline double parse_double(const std::unordered_map<std::string, std::string>& cfg,
                                    const std::string& key, double def)
  {
    auto it = cfg.find(key);
    if (it == cfg.end())
      return def;
    try
    {
      return std::stod(it->second);
    }
    catch (...)
    {
      return def;
    }
  }
};

}  // namespace nano_graphrag
