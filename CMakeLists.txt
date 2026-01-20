cmake_minimum_required(VERSION 3.16)
project(nano_graphrag_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library setup
add_library(nano_graphrag INTERFACE)

target_include_directories(nano_graphrag INTERFACE include)

# Dependencies: Poco JSON, Net, NetSSL
find_package(Poco QUIET COMPONENTS JSON Net NetSSL)

target_link_libraries(nano_graphrag INTERFACE Poco::JSON Poco::Net Poco::NetSSL)

# Integrate nano-vectordb submodule (header-only) as core
find_package(Eigen3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

add_library(nano_vectordb_cpp INTERFACE)
target_include_directories(nano_vectordb_cpp INTERFACE ${CMAKE_SOURCE_DIR}/external/nano-vectordb-cpp/include)
target_link_libraries(nano_vectordb_cpp INTERFACE Eigen3::Eigen OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json SQLite::SQLite3)

target_link_libraries(nano_graphrag INTERFACE nano_vectordb_cpp)

# Integrate cpp-tiktoken library
set(CPP_TIKTOKEN_INSTALL OFF CACHE BOOL "Disable install for cpp-tiktoken" FORCE)
set(CPP_TIKTOKEN_TESTING OFF CACHE BOOL "Disable tests for cpp-tiktoken" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/cpp-tiktoken)
target_link_libraries(nano_graphrag INTERFACE tiktoken)

# Executables
add_executable(demo src/demo.cpp)
target_link_libraries(demo PRIVATE nano_graphrag)

add_executable(test_christmas_carol src/test_christmas_carol.cpp)
target_link_libraries(test_christmas_carol PRIVATE nano_graphrag)

