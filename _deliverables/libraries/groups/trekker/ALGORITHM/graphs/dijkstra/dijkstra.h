#pragma once

#include <vector>


namespace trekker {
namespace algorithm {
namespace graphs {

template<typename N>
using sv = std::vector<N>;

using pii = std::pair<int, int>;

typedef int node_label_t;

template<typename N>
struct PathStep {
    node_label_t node;
    N cumulativeCost;
};

template<typename N>
struct DijkstraResult {
    N totalCost;
    sv<node_label_t> path;
    sv<PathStep<N>> steps;
};

template<typename N>
class Dijkstra {
public:
    explicit Dijkstra(const sv<sv<pii>>& adjacency_list);
    DijkstraResult<N> shortestPath(int start, int destination);
private:
    sv<sv<pii>> adjacency_list_;
};


} // namespace graphs
} // namespace algorithm
} // namespace trekker
