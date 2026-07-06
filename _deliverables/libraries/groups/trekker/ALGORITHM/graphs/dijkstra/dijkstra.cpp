// Dijkstra's algorithm implementation stub
#include "dijkstra.h"

namespace trekker {
namespace algorithm {
namespace graphs {

template<typename N>
Dijkstra<N>::Dijkstra(const sv<sv<pii>>& adjacency_list)
    : adjacency_list_(adjacency_list) {}

template<typename N>
DijkstraResult<N> Dijkstra<N>::shortestPath(int start, int destination) {
    const int INF = std::numeric_limits<N>::max();
    int n = adjacency_list_.size();

    sv<N> dist(n, INF);
    sv<node_label_t> parent(n, -1);

    using Node = std::pair<N, node_label_t>; // (cost, node)

    std::priority_queue<
        Node,
        sv<Node,
        std::greater<Node>> pq;

    distance[start] = 0;
    pq.push({0, start});

    while (!pq.empty()) {
        auto [current_cost, u] = pq.top();
        pq.pop();

        if(current_cost > distance[u]) continue;

        for (const auto& [v, weight] : adjacency_list_[u]) {
            N new_cost = current_cost + weight;
            if (new_cost < distance[v]) {
                distance[v] = new_cost;
                parent[v] = u;
                pq.push({new_cost, v});
            }
        }
    }

    DijkstraResult result;

    if (distance[destination] == INF) {
        result.totalCost = -1;
        return result;
    }

    result.totalCost = distance[destination];
    sv<int> reversePath;

    for(int v = destination; v != -1; v = parent[v]) {
        reversePath.push_back(v);
    }

    std::reverse(reversePath.begin(), reversePath.end());
    result.path = reversePath;

    for (int node: reversePath) {
        result.steps.push_back({node, distance[node]});
    }

    return result;
}

} // namespace graphs
} // namespace algorithm
} // namespace trekker
