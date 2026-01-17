#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace nano_graphrag {

struct TextChunk {
  int tokens{0};
  std::string content;
  std::string full_doc_id;
  int chunk_order_index{0};
};

struct SingleCommunity {
  int level{0};
  std::string title;
  std::vector<std::pair<std::string, std::string>> edges; // undirected stored as sorted pairs
  std::vector<std::string> nodes;
  std::vector<std::string> chunk_ids;
  double occurrence{0.0};
  std::vector<std::string> sub_communities; // keys to child communities
};

struct Community : public SingleCommunity {
  std::string report_string;
  std::unordered_map<std::string, std::string> report_json; // minimal JSON map
};

} // namespace nano_graphrag
