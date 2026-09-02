#pragma once

namespace trekker {
namespace algorithm {
namespace graphs {


#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace trekker {
namespace algorithm {
namespace graphs {

// Recursive DFS: fills visited with all reachable nodes from start
void dfs(int node, const std::unordered_map<int, std::vector<int>>& graph, std::unordered_set<int>& visited);

// Utility: returns all nodes reachable from start
std::unordered_set<int> dfs_reachable(int start, const std::unordered_map<int, std::vector<int>>& graph);


} // namespace graphs
} // namespace algorithm
} // namespace trekker
