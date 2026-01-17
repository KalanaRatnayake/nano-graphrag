# Examples: C++ Demo

This demo showcases the header-only GraphRAG C++ implementation with OpenAI integration.

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build -j 4
```

## Run

Set your OpenAI API key (optional; if omitted, the demo prints only the context):

```bash
export OPENAI_API_KEY=sk-...
```

Run the demo, passing a question as an argument or using the default:

```bash
./build/demo "What is NanoGraphRAG?"
```

## Behavior

- Naive RAG mode retrieves the top-1 most relevant chunk and caps context size.
- With `OPENAI_API_KEY` set, it asks the model to compose an answer.
- Without a key, it prints the retrieved context.

## Tuning

Adjust parameters in `examples-cpp/demo.cpp`:
- `qp.top_k`: number of chunks to include (default 1).
- `qp.naive_max_token_for_text_unit`: context token cap (default 1024).
- `rag.set_chat_model(...)` / `rag.set_embed_model(...)` to switch models.
