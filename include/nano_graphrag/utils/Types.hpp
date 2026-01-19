#include <nlohmann/json.hpp>
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace nano_graphrag
{


struct TextChunk
{
  int tokens{ 0 };
  std::string content;
  std::string full_doc_id;
  int chunk_order_index{ 0 };
};

// nlohmann::json serialization for TextChunk (free functions)
inline void to_json(nlohmann::json& j, const TextChunk& c)
{
  j = nlohmann::json{ { "tokens", c.tokens },
                      { "content", c.content },
                      { "full_doc_id", c.full_doc_id },
                      { "chunk_order_index", c.chunk_order_index } };
}

inline void from_json(const nlohmann::json& j, TextChunk& c)
{
  j.at("tokens").get_to(c.tokens);
  j.at("content").get_to(c.content);
  j.at("full_doc_id").get_to(c.full_doc_id);
  j.at("chunk_order_index").get_to(c.chunk_order_index);
}

struct SingleCommunity
{
  int level{ 0 };
  std::string title;
  std::vector<std::pair<std::string, std::string>> edges;  // undirected stored as sorted pairs
  std::vector<std::string> nodes;
  std::vector<std::string> chunk_ids;
  double occurrence{ 0.0 };
  std::vector<std::string> sub_communities;  // keys to child communities
};

struct Community : public SingleCommunity
{
  std::string report_string;
  std::unordered_map<std::string, std::string> report_json;  // minimal JSON map
};

// nlohmann::json serialization for Community (free functions)
inline void to_json(nlohmann::json& j, const Community& c)
{
  j = nlohmann::json{ { "level", c.level },
                      { "title", c.title },
                      { "edges", c.edges },
                      { "nodes", c.nodes },
                      { "chunk_ids", c.chunk_ids },
                      { "occurrence", c.occurrence },
                      { "sub_communities", c.sub_communities },
                      { "report_string", c.report_string },
                      { "report_json", c.report_json } };
}

inline void from_json(const nlohmann::json& j, Community& c)
{
  j.at("level").get_to(c.level);
  j.at("title").get_to(c.title);
  j.at("edges").get_to(c.edges);
  j.at("nodes").get_to(c.nodes);
  j.at("chunk_ids").get_to(c.chunk_ids);
  j.at("occurrence").get_to(c.occurrence);
  j.at("sub_communities").get_to(c.sub_communities);
  j.at("report_string").get_to(c.report_string);
  j.at("report_json").get_to(c.report_json);
}

struct QueryParam
{
  std::string mode{ "global" };  // "local" | "global" | "naive"
  bool only_need_context{ false };
  std::string response_type{ "Multiple Paragraphs" };
  int level{ 2 };
  int top_k{ 20 };

  // naive search
  int naive_max_token_for_text_unit{ 12000 };

  // local search
  int local_max_token_for_text_unit{ 4000 };
  int local_max_token_for_local_context{ 4800 };
  int local_max_token_for_community_report{ 3200 };
  bool local_community_single_one{ false };

  // global search
  double global_min_community_rating{ 0.0 };
  int global_max_consider_community{ 512 };
  int global_max_token_for_community_report{ 16384 };

  // extra llm kwargs used for community mapping (JSON response)
  std::unordered_map<std::string, std::string> global_special_community_map_llm_kwargs{ { "response_format",
                                                                                          "json_object" } };
};

}  // namespace nano_graphrag
