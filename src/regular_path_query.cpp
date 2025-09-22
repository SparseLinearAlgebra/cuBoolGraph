#include <cassert>
#include <cstdint>
#include <iostream>
#include <print>
#include <set>

#include "regular_path_query.hpp"

cuBool_Matrix regular_path_query_with_transposed(
        // vector of sparse graph matrices for each label
        const std::vector<cuBool_Matrix>& graph, const std::vector<cuBool_Index>& source_vertices,
        // vector of sparse automaton matrices for each label
        const std::vector<cuBool_Matrix>& automaton, const std::vector<cuBool_Index>& start_states,
        // transposed matrices for graph and automaton
        const std::vector<cuBool_Matrix>& graph_transposed,
        const std::vector<cuBool_Matrix>& automaton_transposed,

        const std::vector<bool>& inversed_labels_input, bool all_labels_are_inversed) {
    cuBool_Status status;

    auto inversed_labels = inversed_labels_input;
    inversed_labels.resize(std::max(graph.size(), automaton.size()));

    for (uint32_t i = 0; i < inversed_labels.size(); i++) {
        bool is_inverse = inversed_labels[i];
        is_inverse ^= all_labels_are_inversed;
        inversed_labels[i] = is_inverse;
    }

    cuBool_Index graph_nodes_number     = 0;
    cuBool_Index automaton_nodes_number = 0;

    // get number of graph nodes
    for (auto label_matrix : graph) {
        if (label_matrix != nullptr) {
            cuBool_Matrix_Nrows(label_matrix, &graph_nodes_number);
            break;
        }
    }

    // get number of automaton nodes
    for (auto label_matrix : automaton) {
        if (label_matrix != nullptr) {
            cuBool_Matrix_Nrows(label_matrix, &automaton_nodes_number);
            break;
        }
    }

    // this will be answer
    cuBool_Matrix reacheble{};
    status = cuBool_Matrix_New(&reacheble, automaton_nodes_number, graph_nodes_number);
    assert(status == CUBOOL_STATUS_SUCCESS);

    // allocate neccessary for algorithm matrices
    cuBool_Matrix frontier{}, symbol_frontier{}, next_frontier{};
    status = cuBool_Matrix_New(&next_frontier, automaton_nodes_number, graph_nodes_number);
    assert(status == CUBOOL_STATUS_SUCCESS);
    status = cuBool_Matrix_New(&frontier, automaton_nodes_number, graph_nodes_number);
    assert(status == CUBOOL_STATUS_SUCCESS);
    status = cuBool_Matrix_New(&symbol_frontier, automaton_nodes_number, graph_nodes_number);
    assert(status == CUBOOL_STATUS_SUCCESS);

    // init start values of algorithm matricies
    for (const auto state : start_states) {
        for (const auto vert : source_vertices) {
            assert(state < automaton_nodes_number);
            assert(vert < graph_nodes_number);
            cuBool_Matrix_SetElement(next_frontier, state, vert);
            cuBool_Matrix_SetElement(reacheble, state, vert);
        }
    }

    cuBool_Index states = source_vertices.size();

    // temporary matrix for write result of cubool functions
    cuBool_Matrix result;
    status = cuBool_Matrix_New(&result, automaton_nodes_number, graph_nodes_number);
    assert(status == CUBOOL_STATUS_SUCCESS);

    const auto label_number = std::min(graph.size(), automaton.size());
    while (states > 0) {
        std::swap(frontier, next_frontier);

        // clear next_frontier
        status = cuBool_Matrix_Build(next_frontier, nullptr, nullptr, 0, CUBOOL_HINT_NO);
        assert(status == CUBOOL_STATUS_SUCCESS);

        for (int i = 0; i < label_number; i++) {
            if (graph[i] == nullptr || automaton[i] == nullptr) {
                continue;
            }

            cuBool_Matrix automaton_matrix = all_labels_are_inversed ? automaton[i] : automaton_transposed[i];
            status                         = cuBool_MxM(symbol_frontier, automaton_matrix, frontier, CUBOOL_HINT_NO);
            assert(status == CUBOOL_STATUS_SUCCESS);

            // next_frontier += (symbol_frontier * graph[i]) & (!reachible)
            // multiply 2 matrices
            cuBool_Matrix graph_matrix = inversed_labels[i] ? graph_transposed[i] : graph[i];
            status                     = cuBool_MxM(next_frontier, symbol_frontier, graph_matrix, CUBOOL_HINT_ACCUMULATE);
            assert(status == CUBOOL_STATUS_SUCCESS);
            // apply invert mask
            status = cuBool_Matrix_EWiseMulInverted(result, next_frontier, reacheble, CUBOOL_HINT_NO);
            assert(status == CUBOOL_STATUS_SUCCESS);
            std::swap(result, next_frontier);
        }

        // this must be accumulate with mask and save old value: reacheble += next_frontier & reacheble
        status = cuBool_Matrix_EWiseAdd(result, reacheble, next_frontier, CUBOOL_HINT_NO);
        assert(status == CUBOOL_STATUS_SUCCESS);
        std::swap(result, reacheble);

        cuBool_Matrix_Nvals(next_frontier, &states);
    }

    // free matrix necessary for algorithm
    cuBool_Matrix_Free(next_frontier);
    cuBool_Matrix_Free(frontier);
    cuBool_Matrix_Free(symbol_frontier);
    cuBool_Matrix_Free(result);

    return reacheble;
}


cuBool_Matrix regular_path_query(
        // vector of sparse graph matrices for each label
        const std::vector<cuBool_Matrix>& graph, const std::vector<cuBool_Index>& source_vertices,
        // vector of sparse automaton matrices for each label
        const std::vector<cuBool_Matrix>& automaton, const std::vector<cuBool_Index>& start_states,
        // work with inverted labels
        const std::vector<bool>& inversed_labels_input, bool all_labels_are_inversed) {
    cuBool_Status status;

    // transpose graph matrices
    std::vector<cuBool_Matrix> graph_transposed;
    graph_transposed.reserve(graph.size());
    for (uint32_t i = 0; i < graph.size(); i++) {
        graph_transposed.emplace_back();

        auto label_matrix = graph[i];
        if (label_matrix == nullptr) {
            continue;
        }

        cuBool_Index nrows, ncols;
        cuBool_Matrix_Nrows(label_matrix, &nrows);
        cuBool_Matrix_Ncols(label_matrix, &ncols);

        status = cuBool_Matrix_New(&graph_transposed.back(), ncols, nrows);
        assert(status == CUBOOL_STATUS_SUCCESS);
        status = cuBool_Matrix_Transpose(graph_transposed.back(), label_matrix, CUBOOL_HINT_NO);
        assert(status == CUBOOL_STATUS_SUCCESS);
    }

    // transpose automaton matrices
    std::vector<cuBool_Matrix> automaton_transposed;
    automaton_transposed.reserve(automaton.size());
    for (auto label_matrix : automaton) {
        automaton_transposed.emplace_back();
        if (label_matrix == nullptr) {
            continue;
        }

        cuBool_Index nrows, ncols;
        cuBool_Matrix_Nrows(label_matrix, &nrows);
        cuBool_Matrix_Ncols(label_matrix, &ncols);

        status = cuBool_Matrix_New(&automaton_transposed.back(), ncols, nrows);
        assert(status == CUBOOL_STATUS_SUCCESS);
        status = cuBool_Matrix_Transpose(automaton_transposed.back(), label_matrix, CUBOOL_HINT_NO);
        assert(status == CUBOOL_STATUS_SUCCESS);
    }

    auto result = regular_path_query_with_transposed(
            graph, source_vertices,
            automaton, start_states,
            graph_transposed, automaton_transposed,
            inversed_labels_input, all_labels_are_inversed);

    for (cuBool_Matrix matrix : graph_transposed) {
        if (matrix != nullptr) {
            cuBool_Matrix_Free(matrix);
        }
    }
    for (cuBool_Matrix matrix : automaton_transposed) {
        if (matrix != nullptr) {
            cuBool_Matrix_Free(matrix);
        }
    }

    return result;
}
