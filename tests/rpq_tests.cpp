#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <numeric>
#include <print>
#include <set>
#include <string_view>

#include <fast_matrix_market/fast_matrix_market.hpp>

#include "regular_path_query.hpp"

struct Config {
  std::vector<std::string> graph_data;
  std::vector<std::string> automaton_data;
  std::vector<uint32_t> sources;
  std::string expexted;
  std::string meta_data;
  std::string sources_data;
};

static bool test_on_config(const Config &config) {
  cuBool_Initialize(CUBOOL_HINT_NO);

  std::vector<cuBool_Matrix> graph;
  std::vector<cuBool_Index> source_vertices;
  std::vector<cuBool_Matrix> automaton;
  std::vector<cuBool_Index> start_states;

  int64_t nrows = 0, ncols = 0;
  std::vector<cuBool_Index> rows, cols;
  std::vector<bool> vals;

  // load graph
  cuBool_Index graph_rows = 0;
  graph.reserve(config.graph_data.size());
  for (const auto &data : config.graph_data) {
    graph.emplace_back();
    if (data.empty()) {
      continue;
    }

    std::ifstream file(data);
    if (!file) {
      std::cout << data << std::endl;
      throw std::runtime_error("error open file");
    }
    fast_matrix_market::read_matrix_market_triplet(file, nrows, ncols, rows, cols, vals);

    cuBool_Matrix_New(&graph.back(), nrows, ncols);
    cuBool_Matrix_Build(graph.back(), rows.data(), cols.data(), vals.size(), CUBOOL_HINT_NO);

    graph_rows = std::max(graph_rows, (cuBool_Index)nrows);
  }

  cuBool_Index automaton_rows = 0;
  // load automaton
  automaton.reserve(config.automaton_data.size());
  for (const auto &data : config.automaton_data) {
    automaton.emplace_back();
    if (data.empty()) {
      continue;
    }

    std::ifstream file(data);
    if (!file) {
      throw std::runtime_error("error open file");
    }
    fast_matrix_market::read_matrix_market_triplet(file, nrows, ncols, rows, cols, vals);

    cuBool_Matrix_New(&automaton.back(), nrows, ncols);
    cuBool_Matrix_Build(automaton.back(), rows.data(), cols.data(), vals.size(), CUBOOL_HINT_NO);

    automaton_rows = std::max(automaton_rows, (cuBool_Index)nrows);
  }

  // temporary hardcoded
  source_vertices = {0};
  start_states = {0};

  source_vertices = config.sources;
  for (cuBool_Index &s : source_vertices) {
    s--;
  }

  auto answer = regular_path_query(graph, source_vertices, automaton, start_states);

  cuBool_Vector P, F;
  cuBool_Vector_New(&P, graph_rows);
  cuBool_Vector_New(&F, automaton_rows);

  std::vector<cuBool_Index> final_states(automaton_rows);
  std::iota(final_states.begin(), final_states.end(), 0);

  cuBool_Vector_Build(F, final_states.data(), final_states.size(), CUBOOL_HINT_NO);
  cuBool_VxM(P, F, answer, CUBOOL_HINT_NO);
  uint32_t result = 0;
  cuBool_Vector_Nvals(P, &result);

  bool test_result = true;

  // validate answer
  cuBool_Index nvals;
  cuBool_Matrix_Nvals(answer, &nvals);
  rows.resize(nvals);
  cols.resize(nvals);
  cuBool_Matrix_ExtractPairs(answer, rows.data(), cols.data(), &nvals);

  std::set<std::pair<cuBool_Index, cuBool_Index>> indexes {};
  for (int i = 0; i < nvals; i++) {
    indexes.insert({rows[i], cols[i]});
  }

  std::ifstream expected_file(config.expexted.data());
  cuBool_Index expected_nvals;
  expected_file >> expected_nvals;

  if (expected_nvals != nvals) {
    test_result = false;
  } else {
    for (int k = 0; k < expected_nvals; k++) {
      int i, j;
      expected_file >> i >> j;
      if (!indexes.contains({i, j})) {
        test_result = false;
        break;
      }
    }
  }

  cuBool_Vector_Free(F);
  cuBool_Vector_Free(P);
  cuBool_Matrix_Free(answer);

  for (auto matrix : graph) {
    if (matrix != nullptr) {
      cuBool_Matrix_Free(matrix);
    }
  }
  for (auto matrix : automaton) {
    if (matrix != nullptr) {
      cuBool_Matrix_Free(matrix);
    }
  }

  cuBool_Finalize();

  return test_result;
}

static std::string get_path(std::string_view file) {
  return std::format("{}/rpq/{}", TESTS_DATA_PATH, file);
}

TEST(RPQ, Tests) {
  std::vector<Config> configs {
    {
      .graph_data = {get_path("a.mtx"), get_path("b.mtx")},
      .automaton_data {get_path("1/a.mtx"), ""},
      .sources = {1},
      .expexted = get_path("1/expected.txt"),
    },
    {
      .graph_data = {get_path("a.mtx"), get_path("b.mtx")},
      .automaton_data {get_path("2/a.mtx"), get_path("2/b.mtx")},
      .sources = {2},
      .expexted = get_path("2/expected.txt"),
    },
    {
      .graph_data = {get_path("a.mtx"), get_path("b.mtx")},
      .automaton_data {get_path("3/a.mtx"), get_path("3/b.mtx")},
      .sources = {3, 6},
      .expexted = get_path("3/expected.txt"),
    },
    {
      .graph_data = {get_path("a.mtx"), get_path("b.mtx")},
      .automaton_data {"", get_path("4/b.mtx")},
      .sources = {4},
      .expexted = get_path("4/expected.txt"),
    },
    {
      .graph_data = {get_path("5/graph_a.mtx"), get_path("5/graph_b.mtx")},
      .automaton_data = {get_path("5/automaton_a.mtx"), get_path("5/automaton_b.mtx")},
      .sources = {1},
      .expexted = get_path("5/expected.txt"),
    },
  };

  for (const auto &config : configs) {
    EXPECT_TRUE(test_on_config(config));
  }
}
