#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "nano_graphrag/Embedding.hpp"
#include "nano_graphrag/Types.hpp"

namespace nano_graphrag
{

inline std::vector<TextChunk> chunking_by_token_size(const std::vector<std::vector<int>>& tokens_list,
                                                     const std::vector<std::string>& docs,
                                                     const std::vector<std::string>& doc_keys,
                                                     const TokenizerWrapper& tokenizer_wrapper,
                                                     int overlap_token_size = 128, int max_token_size = 1024)
{
  std::vector<TextChunk> results;
  for (size_t index = 0; index < tokens_list.size(); ++index)
  {
    const auto& tokens = tokens_list[index];
    std::vector<std::vector<int>> chunk_token;
    std::vector<int> lengths;
    std::vector<int> starts;
    for (int start = 0; start < (int)tokens.size(); start += (max_token_size - overlap_token_size))
    {
      std::vector<int> sub(tokens.begin() + start,
                           tokens.begin() + std::min<int>(start + max_token_size, tokens.size()));
      chunk_token.push_back(std::move(sub));
      lengths.push_back(std::min(max_token_size, (int)tokens.size() - start));
      starts.push_back(start);
    }
    std::vector<std::string> chunk_texts;
    chunk_texts.reserve(chunk_token.size());
    if (tokenizer_wrapper.type() == TokenizerWrapper::Type::Simple)
    {
      // For Simple fallback, reconstruct text by word slicing
      const std::string& doc = docs[index];
      // Split doc into words preserving simple whitespace separation
      std::vector<std::string> words;
      words.reserve(doc.size() / 4);
      std::string current;
      for (char c : doc)
      {
        if (c == ' ' || c == '\n' || c == '\t')
        {
          if (!current.empty())
          {
            words.push_back(current);
            current.clear();
          }
        }
        else
        {
          current.push_back(c);
        }
      }
      if (!current.empty())
        words.push_back(current);
      for (size_t i = 0; i < chunk_token.size(); ++i)
      {
        int start_idx = starts[i];
        int len = lengths[i];
        int end_idx = std::min<int>(start_idx + len, (int)words.size());
        std::string text;
        for (int w = start_idx; w < end_idx; ++w)
        {
          if (!text.empty())
            text.push_back(' ');
          text += words[w];
        }
        chunk_texts.push_back(std::move(text));
      }
    }
    else
    {
      chunk_texts = tokenizer_wrapper.decode_batch(chunk_token);
    }
    for (size_t i = 0; i < chunk_texts.size(); ++i)
    {
      results.push_back(TextChunk{ lengths[i], chunk_texts[i], doc_keys[index], (int)i });
    }
  }
  return results;
}

inline std::unordered_map<std::string, TextChunk>
get_chunks(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& new_docs,
           const TokenizerWrapper& tokenizer_wrapper, int overlap_token_size = 128, int max_token_size = 1024)
{
  std::unordered_map<std::string, TextChunk> inserting_chunks;
  std::vector<std::string> docs;
  docs.reserve(new_docs.size());
  std::vector<std::string> keys;
  keys.reserve(new_docs.size());
  for (const auto& kv : new_docs)
  {
    keys.push_back(kv.first);
    docs.push_back(kv.second.at("content"));
  }
  std::vector<std::vector<int>> tokens;
  tokens.reserve(docs.size());
  for (const auto& d : docs)
    tokens.push_back(tokenizer_wrapper.encode(d));
  auto chunks =
      chunking_by_token_size(tokens, docs, keys, tokenizer_wrapper, overlap_token_size, max_token_size);
  for (const auto& chunk : chunks)
  {
    // simple md5-like id: use std::hash
    std::hash<std::string> h;
    std::string id = "chunk-" + std::to_string(h(chunk.content));
    inserting_chunks.emplace(id, chunk);
  }
  return inserting_chunks;
}

}  // namespace nano_graphrag
