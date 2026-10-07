#pragma once

#include <vector>

namespace trekker {
namespace algorithm {
namespace graphs {

template<typename T>
using sv = std::vector<T>;

template<typename N>
struct Step {
    int from;
    int to;
    N cost;
};

template<typename N>
struct TSPResult {
    N totalCost;
    sv<int> tour;
    sv<Step<N>> steps;
};

template<typename N>
class TSP {
public:
    explicit TSP(const sv<sv<N>>& distance_matrix);

    TSPRResult<N> solve()const;
private:
    sv<sv<N>> distance_matrix_;
    sv<N> best_tour_;
    double best_cost_;
};


}
}
}