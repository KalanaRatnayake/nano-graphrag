#pragma once

#include <string>
#include <vector>
#include <functional>

namespace nano_graphrag
{

struct EmbeddingFunc
{
  size_t embedding_dim{ 0 };
  size_t max_token_size{ 0 };
  std::function<std::vector<std::vector<float>>(const std::vector<std::string>&)> func;

  inline std::vector<std::vector<float>> operator()(const std::vector<std::string>& texts) const
  {
    return func ? func(texts) : std::vector<std::vector<float>>{};
  }
};

class TokenizerWrapper
{
public:
  enum class Type
  {
    Tiktoken,
    HuggingFace,
    Simple
  };

  explicit TokenizerWrapper(Type t = Type::Simple, const std::string& model = "")
    : type_(t), model_name_(model)
  {
  }

  std::vector<int> encode(const std::string& text) const
  {
    // Simple whitespace tokenizer fallback
    std::vector<int> tokens;
    tokens.reserve(text.size());
    int acc = 0;
    for (char c : text)
    {
      acc += static_cast<unsigned char>(c);
      if (c == ' ' || c == '\n' || c == '\t')
      {
        tokens.push_back(acc);
        acc = 0;
      }
    }
    if (acc)
      tokens.push_back(acc);
    return tokens;
  }

  std::string decode(const std::vector<int>& tokens) const
  {
    // Simple placeholder decode: join with spaces
    std::string out;
    bool first = true;
    for (int t : tokens)
    {
      if (!first)
        out += ' ';
      first = false;
      out += std::to_string(t);
    }
    return out;
  }

  std::vector<std::string> decode_batch(const std::vector<std::vector<int>>& tokens_list) const
  {
    std::vector<std::string> results;
    results.reserve(tokens_list.size());
    for (const auto& tk : tokens_list)
      results.push_back(decode(tk));
    return results;
  }

  Type type() const
  {
    return type_;
  }

private:
  Type type_;
  std::string model_name_;
};

}  // namespace nano_graphrag
