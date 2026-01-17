#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <utility>

#include "nano_graphrag/Embedding.hpp"
#include "nano_graphrag/Types.hpp"

namespace nano_graphrag {

struct StorageNameSpace {
  std::string namespace_name;
  std::unordered_map<std::string, std::string> global_config;

  inline virtual ~StorageNameSpace() = default;
  inline virtual void index_start_callback() {}
  inline virtual void index_done_callback() {}
  inline virtual void query_done_callback() {}
};

class BaseVectorStorage : public StorageNameSpace {
 public:
  EmbeddingFunc embedding_func;
  std::unordered_map<std::string, bool> meta_fields; // key existence map

  virtual ~BaseVectorStorage() = default;
  virtual std::vector<std::unordered_map<std::string, std::string>> query(const std::string& query, int top_k) = 0;
  virtual void upsert(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& data) = 0;
};

// T is value type stored in KV
template <typename T>
class BaseKVStorage : public StorageNameSpace {
 public:
  virtual ~BaseKVStorage() = default;
  virtual std::vector<std::string> all_keys() = 0;
  virtual std::optional<T> get_by_id(const std::string& id) = 0;
  virtual std::vector<std::optional<T>> get_by_ids(const std::vector<std::string>& ids) = 0;
  virtual std::vector<std::string> filter_keys(const std::vector<std::string>& data) = 0; // return un-exist keys
  virtual void upsert(const std::unordered_map<std::string, T>& data) = 0;
  virtual void drop() = 0;
};

class BaseGraphStorage : public StorageNameSpace {
 public:
  virtual ~BaseGraphStorage() = default;
  virtual bool has_node(const std::string& node_id) const = 0;
  virtual bool has_edge(const std::string& source_node_id, const std::string& target_node_id) const = 0;
  virtual int node_degree(const std::string& node_id) const = 0;
  virtual int edge_degree(const std::string& src_id, const std::string& tgt_id) const = 0;

  virtual std::optional<std::unordered_map<std::string, std::string>> get_node(const std::string& node_id) const = 0;
  virtual std::optional<std::unordered_map<std::string, std::string>> get_edge(const std::string& source_node_id, const std::string& target_node_id) const = 0;

  virtual std::vector<std::pair<std::string, std::string>> get_node_edges(const std::string& source_node_id) const = 0; // may be empty

  virtual void upsert_node(const std::string& node_id, const std::unordered_map<std::string, std::string>& node_data) = 0;
  virtual void upsert_nodes_batch(const std::vector<std::pair<std::string, std::unordered_map<std::string, std::string>>>& nodes_data) = 0;

  virtual void upsert_edge(const std::string& source_node_id, const std::string& target_node_id, const std::unordered_map<std::string, std::string>& edge_data) = 0;
  virtual void upsert_edges_batch(const std::vector<std::tuple<std::string, std::string, std::unordered_map<std::string, std::string>>>& edges_data) = 0;

  virtual void clustering(const std::string& algorithm) = 0;
  virtual std::unordered_map<std::string, SingleCommunity> community_schema() const = 0;
};

} // namespace nano_graphrag
