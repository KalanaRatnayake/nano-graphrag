#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

#include "nano_graphrag/Embedding.hpp"
#include "nano_graphrag/QueryParam.hpp"
#include "nano_graphrag/Types.hpp"
#include "nano_graphrag/ops/Chunking.hpp"
#include "nano_graphrag/storage/Base.hpp"
#include "nano_graphrag/storage/JsonKVStorage.hpp"
#include "nano_graphrag/storage/GraphStorage.hpp"
#include "nano_graphrag/storage/NanoVectorDBStorage.hpp"
#include "nano_graphrag/llm/OpenAI.hpp"
#include "nano_graphrag/Prompts.hpp"

namespace nano_graphrag
{

class GraphRAG
{
public:
  // config
  std::string working_dir;
  bool enable_local{ true };
  bool enable_naive_rag{ false };

  // tokenizer
  TokenizerWrapper tokenizer_wrapper{ TokenizerWrapper::Type::Simple };
  int chunk_token_size{ 1200 };
  int chunk_overlap_token_size{ 100 };

  // storage
  std::unique_ptr<BaseKVStorage<std::unordered_map<std::string, std::string>>> full_docs;
  std::unique_ptr<BaseKVStorage<TextChunk>> text_chunks;
  std::unique_ptr<BaseKVStorage<Community>> community_reports;
  std::unique_ptr<BaseGraphStorage> chunk_entity_relation_graph;
  std::unique_ptr<BaseVectorStorage> entities_vdb;
  std::unique_ptr<BaseVectorStorage> chunks_vdb;

  EmbeddingFunc embedding_func;  // to be set by user
  OpenAIClient openai;           // simple client
  std::string chat_model{ "gpt-4o" };
  std::string embed_model{ "text-embedding-3-small" };

  explicit GraphRAG(const std::string& workdir = std::string{ "./nano_graphrag_cache" })
    : working_dir(workdir)
  {
    std::filesystem::create_directories(working_dir);
    std::unordered_map<std::string, std::string> cfg;
    cfg["working_dir"] = working_dir;

    full_docs =
        std::make_unique<JsonKVStorage<std::unordered_map<std::string, std::string>>>("full_docs", cfg);
    text_chunks = std::make_unique<JsonKVStorage<TextChunk>>("text_chunks", cfg);
    community_reports = std::make_unique<JsonKVStorage<Community>>("community_reports", cfg);
    chunk_entity_relation_graph = std::make_unique<InMemoryGraphStorage>("chunk_entity_relation", cfg);
  }

  void set_embedding_func(const EmbeddingFunc& f)
  {
    embedding_func = f;
  }
  void set_openai_api_key(const std::string& key)
  {
    openai.set_api_key(key);
  }
  void set_chat_model(const std::string& m)
  {
    chat_model = m;
  }
  void set_embed_model(const std::string& m)
  {
    embed_model = m;
  }

  void enable_naive(bool v)
  {
    enable_naive_rag = v;
    if (enable_naive_rag)
    {
      std::unordered_map<std::string, std::string> cfg;
      cfg["working_dir"] = working_dir;
      cfg["query_better_than_threshold"] = "0.0";
      chunks_vdb = std::make_unique<NanoVectorDBStorage>("chunks", cfg, embedding_func);
    }
  }

  void insert(const std::vector<std::string>& docs)
  {
    // compute new doc ids
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> new_docs;
    std::hash<std::string> h;
    for (const auto& c : docs)
    {
      std::string id = "doc-" + std::to_string(h(c));
      new_docs[id] = { { "content", c } };
    }

    // chunking
    auto inserting_chunks =
        get_chunks(new_docs, tokenizer_wrapper, chunk_overlap_token_size, chunk_token_size);

    // upsert vector DB for naive
    if (enable_naive_rag && chunks_vdb)
    {
      std::unordered_map<std::string, std::unordered_map<std::string, std::string>> vdb_data;
      for (const auto& kv : inserting_chunks)
      {
        vdb_data[kv.first] = { { "content", kv.second.content } };
      }
      chunks_vdb->upsert(vdb_data);
    }

    // upsert KV stores
    full_docs->upsert(new_docs);
    text_chunks->upsert(inserting_chunks);
  }

  std::string query(const std::string& q, const QueryParam& param = QueryParam{})
  {
    if (param.mode == "naive")
    {
      return naive_query(q, param);
    }
    // local/global not implemented fully
    return std::string{ "Sorry, I'm not able to provide an answer to that question." };
  }

private:
  std::string naive_query(const std::string& q, const QueryParam& param)
  {
    if (!chunks_vdb)
      return std::string{ "Sorry, I'm not able to provide an answer to that question." };
    auto results = chunks_vdb->query(q, param.top_k);
    if (results.empty())
      return std::string{ "Sorry, I'm not able to provide an answer to that question." };
    std::vector<std::string> ids;
    ids.reserve(results.size());
    for (auto& r : results)
      ids.push_back(r["id"]);
    auto chunks = text_chunks->get_by_ids(ids);
    std::string section;
    int tokens = 0;
    for (const auto& optChunk : chunks)
    {
      if (!optChunk.has_value())
        continue;
      const auto& c = optChunk.value();
      tokens += c.tokens;
      if (tokens > param.naive_max_token_for_text_unit)
        break;
      section += c.content + "\n--New Chunk--\n";
    }
    if (!section.empty())
      section.erase(section.size() - std::string("\n--New Chunk--\n").size());
    if (param.only_need_context)
      return section;
    auto sys_prompt = Prompts::naive_rag_response(section, param.response_type);
    auto resp = openai.chat_complete(chat_model, q, sys_prompt);
    if (resp.empty())
      return section;  // graceful fallback if network fails
    return resp;
  }
};

}  // namespace nano_graphrag
