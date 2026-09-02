# Trekker ALGORITHM Package Plan (v1.3.0)

## Overview
This update introduces a modular ALGORITHM package under trekker, with robust implementations for core graph algorithms and list operations.

## Directory Structure
- _libraries/groups/trekker/ALGORITHM/
  - graphs/
    - a_star/
    - bfs/
    - dfs/
    - dijkstra/
  - lists/
    - woodford/
    - sorts/

## Implemented Algorithms
### Graphs
- Depth-First Search (DFS)
  - Recursive implementation
  - Utility to collect all reachable nodes
- Breadth-First Search (BFS)
  - Returns order of nodes visited
- Dijkstra's Algorithm
  - Returns shortest distances from start node

### Lists
- (Planned) Woodford algorithms
- (Planned) Sort implementations

## Next Steps
- Implement A* algorithm
- Add Woodford and sort algorithms
- Add CMakeLists.txt and test scaffolding
- Integrate with main build and test suites

---
This plan is tracked in v1.3.0. Update as new algorithms and features are added.
