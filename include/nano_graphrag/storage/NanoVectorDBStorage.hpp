#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <cmath>
#include <algorithm>
#include <memory>

#if defined(USE_NANO_VECTORDB)
#include "NanoVectorDB.hpp"
#endif

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
#if defined(USE_NANO_VECTORDB)
    std::string metric = cfg.count("metric") ? cfg.at("metric") : std::string("cosine");
    std::string storage_file = cfg.count("storage_file") ? cfg.at("storage_file") : std::string("nano-vectordb.json");
    if (embedding_func.embedding_dim > 0)
    {
      db_ = std::make_unique<nano_vectordb::NanoVectorDB>(static_cast<int>(embedding_func.embedding_dim), metric, storage_file);
    }
#endif
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
#if defined(USE_NANO_VECTORDB)
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
#else
    for (size_t i = 0; i < ids.size(); ++i)
    {
      Item item; item.id = ids[i]; item.embedding = embeddings[i]; item.meta = metas_[ids[i]];
      items_.push_back(std::move(item));
    }
#endif
  }

  std::vector<std::unordered_map<std::string, std::string>> query(const std::string& query, int top_k) override
  {
    auto qembs = embedding_func(std::vector<std::string>{ query });
    std::vector<float> q;
    if (!qembs.empty()) q = qembs[0]; else q.assign(embedding_func.embedding_dim, 0.0f);
    std::vector<std::unordered_map<std::string, std::string>> out;
#if defined(USE_NANO_VECTORDB)
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
#endif
    // Fallback to internal cosine implementation
    if (items_.empty()) return {};
    struct Scored { size_t idx; double sim; };
    std::vector<Scored> scored; scored.reserve(items_.size());
    for (size_t i = 0; i < items_.size(); ++i)
    {
      double sim = cosine_similarity(q, items_[i].embedding);
      scored.push_back({ i, sim });
    }
    std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) { return a.sim > b.sim; });
    if (top_k < (int)scored.size()) scored.resize(top_k);
    out.reserve(scored.size());
    for (const auto& s : scored)
    {
      std::unordered_map<std::string, std::string> row = items_[s.idx].meta;
      row["id"] = items_[s.idx].id;
      row["similarity"] = std::to_string(s.sim);
      out.push_back(std::move(row));
    }
    return out;
  }

private:
  struct Item
  {
    std::string id;
    std::vector<float> embedding;
    std::unordered_map<std::string, std::string> meta;
  };
  std::vector<Item> items_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> metas_;
  double cosine_better_than_threshold_{ 0.2 };
#if defined(USE_NANO_VECTORDB)
  std::unique_ptr<nano_vectordb::NanoVectorDB> db_;
#endif

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
  static inline double cosine_similarity(const std::vector<float>& a, const std::vector<float>& b)
  {
    double dot = 0.0, na = 0.0, nb = 0.0;
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
    {
      dot += a[i] * b[i];
      na += a[i] * a[i];
      nb += b[i] * b[i];
    }
    if (na == 0.0 || nb == 0.0)
      return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
  }
};

}  // namespace nano_graphrag
