#include <iostream>
#include <vector>

#include "nano_graphrag/GraphRAG.hpp"
#include "nano_graphrag/Embedding.hpp"

int main(int argc, char** argv)
{
  using namespace nano_graphrag;
  GraphRAG rag("./nano_cache");

  // Build OpenAI embedding via client (synchronous wrapper)
  OpenAIClient client;

  // Set API key from environment if available
  const char* api_key = std::getenv("OPENAI_API_KEY");
  if (api_key)
  {
    rag.set_openai_api_key(api_key);
    client.set_api_key(api_key);
  }

  EmbeddingFunc emb{ 1536, 8192, [&client](const std::vector<std::string>& texts) {
                      return client.embeddings("text-embedding-3-small", texts);
                    } };
                    
  rag.set_embedding_func(emb);
  rag.enable_naive(true);

  rag.insert({
      "NanoGraphRAG is a lightweight GraphRAG implementation using simple storages.",
      "OpenAI embeddings and chat completions can be used for RAG responses.",
  });

  QueryParam qp;
  qp.mode = "naive";
  qp.response_type = "Multiple Paragraphs";
  qp.top_k = 1;                             // limit to the single most relevant chunk
  qp.naive_max_token_for_text_unit = 1024;  // cap context size
  if (!api_key)
  {
    // If no API key, skip network and just print context
    qp.only_need_context = true;
  }
  std::string question = "What is NanoGraphRAG?";
  if (argc > 1)
  {
    question = argv[1];
  }
  std::string answer = rag.query(question, qp);
  std::cout << "Question:\n" << question << "\n\n";
  std::cout << "Answer:\n" << answer << std::endl;

  return 0;
}
