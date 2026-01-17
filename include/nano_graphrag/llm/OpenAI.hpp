#pragma once

#include <string>
#include <vector>
#include <cstdlib>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>

#include "nano_graphrag/RestClient.hpp"

namespace nano_graphrag
{

class OpenAIClient
{
public:
  OpenAIClient() : base_url_("https://api.openai.com")
  {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (key)
      api_key_ = key;
  }

  void set_api_key(const std::string& key)
  {
    api_key_ = key;
  }
  void set_base_url(const std::string& url)
  {
    base_url_ = url;
  }

  // Chat-like completion via Responses API; returns assistant text
  std::string chat_complete(const std::string& model,
                            const std::string& user_prompt,
                            const std::string& system_prompt = std::string{},
                            const Poco::JSON::Object* extra = nullptr)
  {
    Poco::JSON::Object body;
    body.set("model", model);

    // Build input as array of messages per OpenAI Responses API
    Poco::JSON::Array input_array;
    if (!system_prompt.empty())
    {
      Poco::JSON::Object sys_msg;
      sys_msg.set("type", "message");
      sys_msg.set("role", "system");
      Poco::JSON::Array sys_content;
      Poco::JSON::Object sys_text;
      sys_text.set("type", "input_text");
      sys_text.set("text", system_prompt);
      sys_content.add(sys_text);
      sys_msg.set("content", sys_content);
      input_array.add(sys_msg);
    }
    {
      Poco::JSON::Object user_msg;
      user_msg.set("type", "message");
      user_msg.set("role", "user");
      Poco::JSON::Array user_content;
      Poco::JSON::Object user_text;
      user_text.set("type", "input_text");
      user_text.set("text", user_prompt);
      user_content.add(user_text);
      user_msg.set("content", user_content);
      input_array.add(user_msg);
    }
    body.set("input", input_array);

    if (extra)
    {
      for (auto it = extra->begin(); it != extra->end(); ++it)
        body.set(it->first, it->second);
    }

    try
    {
      RestClient rc;
      rc.set_method("POST");
      rc.set_auth_bearer(api_key_);
      auto ptr = rc.post_json(body, base_url_ + "/v1/responses");
      // Parse `output` array, find first content item with `text`
      Poco::JSON::Array::Ptr output = ptr->getArray("output");
      if (!output) return std::string{};
      for (size_t i = 0; i < output->size(); ++i)
      {
        Poco::JSON::Object::Ptr out_item = output->getObject(i);
        Poco::JSON::Array::Ptr content = out_item->getArray("content");
        if (!content) continue;
        for (size_t j = 0; j < content->size(); ++j)
        {
          Poco::JSON::Object::Ptr cobj = content->getObject(j);
          if (cobj->has("text"))
            return cobj->getValue<std::string>("text");
        }
      }
      return std::string{};
    }
    catch (...)
    {
      return std::string{};
    }
  }

  // OpenAI embeddings; returns float vectors per input
  std::vector<std::vector<float>> embeddings(const std::string& model, const std::vector<std::string>& texts)
  {
    Poco::JSON::Object body;
    body.set("model", model);
    Poco::JSON::Array input;
    for (const auto& t : texts)
      input.add(t);
    body.set("input", input);
    body.set("encoding_format", "float");

    try
    {
      RestClient rc;
      rc.set_method("POST");
      rc.set_auth_bearer(api_key_);
      auto ptr = rc.post_json(body, base_url_ + "/v1/embeddings");
      std::vector<std::vector<float>> out;
      Poco::JSON::Array::Ptr data = ptr->getArray("data");
      if (!data)
        return out;
      for (size_t i = 0; i < data->size(); ++i)
      {
        Poco::JSON::Object::Ptr item = data->getObject(i);
        Poco::Dynamic::Var embVar = item->get("embedding");
        Poco::JSON::Array::Ptr embArr = embVar.extract<Poco::JSON::Array::Ptr>();
        std::vector<float> vec;
        vec.reserve(embArr->size());
        for (size_t j = 0; j < embArr->size(); ++j)
          vec.push_back(static_cast<float>(embArr->getElement<double>(j)));
        out.push_back(std::move(vec));
      }
      return out;
    }
    catch (...)
    {
      return {};
    }
  }

private:
  std::string api_key_;
  std::string base_url_;
};

}  // namespace nano_graphrag
