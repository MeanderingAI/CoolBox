#pragma once

namespace trekker {
namespace algorithm {
namespace graphs {

#include <vector>

template<typename N>
using sv = std::vector<N>;

typedef int node_label_t;

template<typename N>
using target_cost_t = std::pair<node_label_t, N>;

template<typename N>
struct AStarStep {
    node_label_t node;
    N gCost;
};

template<typename N>
struct AStarResult {
    N totalCost;
    sv<node_label_t> path;
    sv<AStarStep<N>> steps;
};

template<typename N>
class AStar {
public:
    AStar(
        const sv<sv<target_cost_t<N>>> &adjacency_list, 
        const sv<N> &heuristic
    );


    AStarResult<N> findPath(node_label_t start, node_label_t destination);
private:
    sv<sv<target_cost_t<N>>> adjacency_list_;
    sv<N> heuristic_;
};

} // namespace graphs
} // namespace algorithm
} // namespace trekker
