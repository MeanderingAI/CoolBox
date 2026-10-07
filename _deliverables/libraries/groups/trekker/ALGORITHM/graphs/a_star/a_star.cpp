// A* algorithm implementation stub
#include "a_star.h"

namespace trekker {
namespace algorithm {
namespace graphs {

#include <queue>
#include <limits>
#include <algorithm>

template<typename N>
struct QueueNode {
    node_label_t node;
    N FSiSt;

    bool operator>(const QueueNode& other) const {
        return FSiSt > other.FSiSt;
    }
};

template<typename N>
AStar<N>::AStar(
    const sv<sv<target_cost_t<N>>>& adjacency_list,
    const sv<N>& heuristic
) : adjacency_list_(adjacency_list), heuristic_(heuristic) {}

template<typename N>
AStarResult<N> AStar<N>::findPath(
    node_label_t start, 
    node_label_t destination
) {
    const N INF = std::numeric_limits<N>::max();
    int n = adjacency_list_.size();

    sv<N> gCost(n, INF);
    sv<node_label_t> parent(n, -1);

    using Node = QueueNode<N>;
    std::priority_queue<Node, sv<Node>, std::greater<Node>> pq;

    gCost[start] = 0;
    pq.push({start, heuristic_[start]});

    while (!pq.empty()) {
        auto [current_node, current_fCost] = pq.top();
        pq.pop();

        if (current_node == destination) break;

        for (const auto& [neighbor, cost] : adjacency_list_[current_node]) {
            N tentative_gCost = gCost[current_node] + cost;
            if (tentative_gCost < gCost[neighbor]) {
                gCost[neighbor] = tentative_gCost;
                parent[neighbor] = current_node;
                N fCost = tentative_gCost + heuristic_[neighbor];
                pq.push({neighbor, fCost});
            }
        }
    }

    AStarResult<N> result;
    if (gCost[destination] == INF) {
        result.totalCost = -1;
        return result;
    }

    result.totalCost = gCost[destination];
    sv<node_label_t> reversePath;

    for (int v = destination; v != -1; v = parent[v]) {
        reversePath.push_back(v);
    }

    std::reverse(reversePath.begin(), reversePath.end());
    result.path = std::move(reversePath);

    return result;
}

} // namespace graphs
} // namespace algorithm
} // namespace trekker
