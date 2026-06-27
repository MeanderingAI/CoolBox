#include "tsp.h"

#include <algorithm>
#include <climits>

namespace trekker {
namespace algorithm {
namespace graphs {

template<typename N>
TSP<N>::TSP(const sv<sv<N>>& distance_matrix)
    : distance_matrix_(distance_matrix), best_cost_(std::numeric_limits<double>::max()) {}


template<typename N>
TSPResult<N> TSP<N>::solve() const {
    int n = distance_matrix_.size();
    sv<int> vertices(n);
    for (int i = 0; i < n; ++i) {
        vertices[i] = i;
    }

    do {
        double current_cost = 0;
        for (int i = 0; i < n - 1; ++i) {
            current_cost += distance_matrix_[vertices[i]][vertices[i + 1]];
        }
        current_cost += distance_matrix_[vertices[n - 1]][vertices[0]];

        if (current_cost < best_cost_) {
            best_cost_ = current_cost;
            best_tour_ = sv<N>(vertices.begin(), vertices.end());
        }
    } while (std::next_permutation(vertices.begin(), vertices.end()));

    TSPResult<N> result;
    result.totalCost = best_cost_;
    result.tour = best_tour_;
    for (size_t i = 0; i < best_tour_.size(); ++i) {
        int from = best_tour_[i];
        int to = best_tour_[(i + 1) % best_tour_.size()];
        N cost = distance_matrix_[from][to];
        result.steps.push_back({from, to, cost});
    }

    return result;
}


}
}
}