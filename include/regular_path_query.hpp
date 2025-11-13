#include <vector>

#include <cubool/cubool.h>

cuBool_Matrix regular_path_query_with_transposed(
        // vector of sparse graph matrices for each label
        const std::vector<cuBool_Matrix>& graph, const std::vector<cuBool_Index>& source_vertices,
        // vector of sparse automaton matrices for each label
        const std::vector<cuBool_Matrix>& automaton, const std::vector<cuBool_Index>& start_states,
        // transposed matrices for graph and automaton
        const std::vector<cuBool_Matrix>& graph_transposed,
        const std::vector<cuBool_Matrix>& automaton_transposed,

        const std::vector<bool>& inversed_labels = {}, bool all_labels_are_inversed = false);

cuBool_Matrix regular_path_query(
        // vector of sparse graph matrices for each label
        const std::vector<cuBool_Matrix>& graph, const std::vector<cuBool_Index>& source_vertices,
        // vector of sparse automaton matrices for each label
        const std::vector<cuBool_Matrix>& automaton, const std::vector<cuBool_Index>& start_states,

        const std::vector<bool>& inversed_labels = {}, bool all_labels_are_inversed = false);
