#include "bapo.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace ml {
namespace deep_learning {
namespace agents {
namespace {

size_t integer_pow(size_t base, size_t exponent, size_t cap) {
    size_t result = 1;
    for (size_t i = 0; i < exponent; ++i) {
        if (base != 0 && result > cap / base) {
            return cap + 1;
        }
        result *= base;
    }
    return result;
}

/// Position 0 is the most significant digit.
Input decode(size_t index, size_t length, size_t alphabet_size) {
    Input symbols(length, 0);
    for (size_t i = length; i-- > 0;) {
        symbols[i] = static_cast<Symbol>(index % alphabet_size);
        index /= alphabet_size;
    }
    return symbols;
}

size_t bits_for(size_t distinct_values) {
    if (distinct_values <= 1) {
        return 0;
    }
    size_t bits = 0;
    size_t capacity = 1;
    while (capacity < distinct_values) {
        capacity *= 2;
        ++bits;
    }
    return bits;
}

using Adjacency = std::vector<std::vector<char>>;

/// DSATUR upper bound on the chromatic number.
size_t dsatur_colors(const Adjacency& adjacency) {
    const size_t n = adjacency.size();
    std::vector<int> color(n, -1);
    std::vector<size_t> degree(n, 0);
    for (size_t v = 0; v < n; ++v) {
        degree[v] = static_cast<size_t>(std::count(adjacency[v].begin(), adjacency[v].end(), 1));
    }

    size_t used = 0;
    for (size_t step = 0; step < n; ++step) {
        size_t best = n;
        size_t best_saturation = 0;
        for (size_t v = 0; v < n; ++v) {
            if (color[v] >= 0) {
                continue;
            }
            std::vector<char> seen(used + 1, 0);
            size_t saturation = 0;
            for (size_t u = 0; u < n; ++u) {
                if (adjacency[v][u] && color[u] >= 0 && !seen[static_cast<size_t>(color[u])]) {
                    seen[static_cast<size_t>(color[u])] = 1;
                    ++saturation;
                }
            }
            if (best == n || saturation > best_saturation ||
                (saturation == best_saturation && degree[v] > degree[best])) {
                best = v;
                best_saturation = saturation;
            }
        }

        std::vector<char> blocked(used + 1, 0);
        for (size_t u = 0; u < n; ++u) {
            if (adjacency[best][u] && color[u] >= 0) {
                blocked[static_cast<size_t>(color[u])] = 1;
            }
        }
        size_t chosen = 0;
        while (chosen < used && blocked[chosen]) {
            ++chosen;
        }
        color[best] = static_cast<int>(chosen);
        used = std::max(used, chosen + 1);
    }
    return used;
}

/// Greedy clique, a lower bound on the chromatic number.
size_t greedy_clique_size(const Adjacency& adjacency) {
    const size_t n = adjacency.size();
    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return std::count(adjacency[a].begin(), adjacency[a].end(), 1) >
               std::count(adjacency[b].begin(), adjacency[b].end(), 1);
    });

    size_t best = n == 0 ? 0 : 1;
    for (size_t start : order) {
        std::vector<size_t> clique = {start};
        for (size_t v : order) {
            if (v == start) {
                continue;
            }
            bool connected = true;
            for (size_t member : clique) {
                if (!adjacency[v][member]) {
                    connected = false;
                    break;
                }
            }
            if (connected) {
                clique.push_back(v);
            }
        }
        best = std::max(best, clique.size());
    }
    return best;
}

bool can_color(const Adjacency& adjacency, size_t colors, std::vector<int>& color, size_t vertex) {
    const size_t n = adjacency.size();
    if (vertex == n) {
        return true;
    }
    size_t ceiling = 0;
    for (size_t v = 0; v < vertex; ++v) {
        ceiling = std::max(ceiling, static_cast<size_t>(color[v]) + 1);
    }
    // Symmetry breaking: never open more than one fresh colour at a time.
    const size_t limit = std::min(colors, ceiling + 1);
    for (size_t c = 0; c < limit; ++c) {
        bool ok = true;
        for (size_t u = 0; u < vertex; ++u) {
            if (adjacency[vertex][u] && color[u] == static_cast<int>(c)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            color[vertex] = static_cast<int>(c);
            if (can_color(adjacency, colors, color, vertex + 1)) {
                return true;
            }
            color[vertex] = -1;
        }
    }
    return false;
}

size_t chromatic_number(const Adjacency& adjacency, size_t exact_node_limit = 28) {
    const size_t n = adjacency.size();
    if (n == 0) {
        return 0;
    }
    bool has_edge = false;
    for (size_t v = 0; v < n && !has_edge; ++v) {
        for (size_t u = v + 1; u < n; ++u) {
            if (adjacency[v][u]) {
                has_edge = true;
                break;
            }
        }
    }
    if (!has_edge) {
        return 1;
    }

    const size_t upper = dsatur_colors(adjacency);
    const size_t lower = greedy_clique_size(adjacency);
    if (lower >= upper || n > exact_node_limit) {
        return upper;
    }

    for (size_t k = lower; k < upper; ++k) {
        std::vector<int> color(n, -1);
        if (can_color(adjacency, k, color, 0)) {
            return k;
        }
    }
    return upper;
}

} // namespace

size_t BapoTask::input_space_size() const {
    return integer_pow(alphabet_size, length, static_cast<size_t>(-1) / 2);
}

// ----------------------------------------------------------------------------
// BapoMachine
// ----------------------------------------------------------------------------

BapoMachine::BapoMachine(BapoBandwidth bandwidth, BapoOracles oracles)
    : bandwidth_(bandwidth), oracles_(std::move(oracles)) {
    if (!oracles_.decoder) {
        throw std::invalid_argument("BapoMachine: a decoder is required");
    }
}

Output BapoMachine::run(const Input& input, size_t split) const {
    if (split > input.size()) {
        throw std::out_of_range("BapoMachine::run: split is past the end of the input");
    }
    const Input prefix(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(split));
    const Input suffix(input.begin() + static_cast<std::ptrdiff_t>(split), input.end());

    std::vector<bool> bits;
    if (oracles_.prefix_oracle) {
        bits = oracles_.prefix_oracle(prefix);
    }
    if (bits.size() > bandwidth_.prefix_bits) {
        throw std::runtime_error("BapoMachine: prefix oracle exceeded its bit budget");
    }

    std::vector<size_t> positions;
    if (oracles_.attention_oracle) {
        positions = oracles_.attention_oracle(suffix);
    }
    if (positions.size() > bandwidth_.attention_tokens) {
        throw std::runtime_error("BapoMachine: attention oracle exceeded its token budget");
    }

    std::vector<Symbol> attended;
    attended.reserve(positions.size());
    for (size_t position : positions) {
        if (position >= prefix.size()) {
            throw std::out_of_range("BapoMachine: attended position is outside the prefix");
        }
        attended.push_back(prefix[position]);
    }

    return oracles_.decoder(bits, attended, suffix);
}

bool BapoMachine::solves(const BapoTask& task, size_t split) const {
    const size_t total = task.input_space_size();
    for (size_t index = 0; index < total; ++index) {
        const Input input = decode(index, task.length, task.alphabet_size);
        if (run(input, split) != task.evaluate(input)) {
            return false;
        }
    }
    return true;
}

// ----------------------------------------------------------------------------
// BapoAnalyzer
// ----------------------------------------------------------------------------

BapoAnalyzer::BapoAnalyzer(BapoTask task, size_t max_input_space)
    : task_(std::move(task)), max_input_space_(max_input_space) {
    if (!task_.evaluate) {
        throw std::invalid_argument("BapoAnalyzer: the task needs an evaluate function");
    }
    if (task_.alphabet_size < 2) {
        throw std::invalid_argument("BapoAnalyzer: alphabet_size must be at least 2");
    }
    if (task_.input_space_size() > max_input_space_) {
        throw std::invalid_argument("BapoAnalyzer: input space is larger than the configured limit");
    }
}

std::vector<size_t> BapoAnalyzer::class_of_each_prefix(size_t split) const {
    const size_t prefix_count = integer_pow(task_.alphabet_size, split, max_input_space_);
    const size_t suffix_count =
        integer_pow(task_.alphabet_size, task_.length - split, max_input_space_);

    std::map<std::vector<Output>, size_t> classes;
    std::vector<size_t> assignment(prefix_count, 0);

    for (size_t p = 0; p < prefix_count; ++p) {
        const Input prefix = decode(p, split, task_.alphabet_size);
        std::vector<Output> behaviour(suffix_count);
        for (size_t s = 0; s < suffix_count; ++s) {
            Input input = prefix;
            const Input suffix = decode(s, task_.length - split, task_.alphabet_size);
            input.insert(input.end(), suffix.begin(), suffix.end());
            behaviour[s] = task_.evaluate(input);
        }
        const auto inserted = classes.emplace(std::move(behaviour), classes.size());
        assignment[p] = inserted.first->second;
    }
    return assignment;
}

size_t BapoAnalyzer::prefix_classes(size_t split) const {
    if (split > task_.length) {
        throw std::out_of_range("BapoAnalyzer: split is past the end of the input");
    }
    const std::vector<size_t> assignment = class_of_each_prefix(split);
    if (assignment.empty()) {
        return 1;
    }
    return static_cast<size_t>(*std::max_element(assignment.begin(), assignment.end())) + 1;
}

size_t BapoAnalyzer::minimum_prefix_bits(size_t split) const {
    return bits_for(prefix_classes(split));
}

size_t BapoAnalyzer::minimum_prefix_bits_with_attention(size_t split,
                                                        size_t attention_tokens) const {
    if (attention_tokens == 0) {
        return minimum_prefix_bits(split);
    }
    if (split > task_.length) {
        throw std::out_of_range("BapoAnalyzer: split is past the end of the input");
    }
    if (attention_tokens >= split) {
        return 0; // Attention can simply read the whole prefix.
    }

    const size_t prefix_count = integer_pow(task_.alphabet_size, split, max_input_space_);
    const size_t suffix_count =
        integer_pow(task_.alphabet_size, task_.length - split, max_input_space_);

    std::vector<Input> prefixes(prefix_count);
    for (size_t p = 0; p < prefix_count; ++p) {
        prefixes[p] = decode(p, split, task_.alphabet_size);
    }

    // answers[s][p]
    std::vector<std::vector<Output>> answers(suffix_count, std::vector<Output>(prefix_count));
    for (size_t s = 0; s < suffix_count; ++s) {
        const Input suffix = decode(s, task_.length - split, task_.alphabet_size);
        for (size_t p = 0; p < prefix_count; ++p) {
            Input input = prefixes[p];
            input.insert(input.end(), suffix.begin(), suffix.end());
            answers[s][p] = task_.evaluate(input);
        }
    }

    // Every size-b set of prefix positions the attention oracle may pick.
    std::vector<std::vector<size_t>> candidates;
    std::vector<size_t> current;
    std::function<void(size_t)> build = [&](size_t start) {
        if (current.size() == attention_tokens) {
            candidates.push_back(current);
            return;
        }
        for (size_t position = start; position < split; ++position) {
            current.push_back(position);
            build(position + 1);
            current.pop_back();
        }
    };
    build(0);

    Adjacency adjacency(prefix_count, std::vector<char>(prefix_count, 0));
    for (size_t s = 0; s < suffix_count; ++s) {
        // Pick the attended positions that leave the fewest confusable pairs.
        const std::vector<size_t>* best = nullptr;
        size_t best_conflicts = static_cast<size_t>(-1);
        for (const auto& candidate : candidates) {
            std::map<std::pair<std::vector<Symbol>, Output>, size_t> buckets;
            std::map<std::vector<Symbol>, size_t> totals;
            for (size_t p = 0; p < prefix_count; ++p) {
                std::vector<Symbol> key;
                key.reserve(candidate.size());
                for (size_t position : candidate) {
                    key.push_back(prefixes[p][position]);
                }
                ++totals[key];
                ++buckets[{key, answers[s][p]}];
            }
            size_t conflicts = 0;
            for (const auto& entry : totals) {
                conflicts += entry.second * entry.second;
            }
            for (const auto& entry : buckets) {
                conflicts -= entry.second * entry.second;
            }
            if (conflicts < best_conflicts) {
                best_conflicts = conflicts;
                best = &candidate;
            }
        }
        if (best == nullptr) {
            continue;
        }

        for (size_t p = 0; p < prefix_count; ++p) {
            for (size_t q = p + 1; q < prefix_count; ++q) {
                if (adjacency[p][q] || answers[s][p] == answers[s][q]) {
                    continue;
                }
                bool same_view = true;
                for (size_t position : *best) {
                    if (prefixes[p][position] != prefixes[q][position]) {
                        same_view = false;
                        break;
                    }
                }
                if (same_view) {
                    adjacency[p][q] = 1;
                    adjacency[q][p] = 1;
                }
            }
        }
    }

    return bits_for(chromatic_number(adjacency));
}

size_t BapoAnalyzer::bandwidth(size_t attention_tokens) const {
    size_t worst = 0;
    for (size_t split = 0; split <= task_.length; ++split) {
        worst = std::max(worst, minimum_prefix_bits_with_attention(split, attention_tokens));
    }
    return worst;
}

size_t BapoAnalyzer::hardest_split(size_t attention_tokens) const {
    size_t worst = 0;
    size_t argworst = 0;
    for (size_t split = 0; split <= task_.length; ++split) {
        const size_t bits = minimum_prefix_bits_with_attention(split, attention_tokens);
        if (bits > worst) {
            worst = bits;
            argworst = split;
        }
    }
    return argworst;
}

bool BapoAnalyzer::is_bapo_easy(size_t budget_bits, size_t attention_tokens) const {
    return bandwidth(attention_tokens) <= budget_bits;
}

size_t cot_bandwidth(const CotDecomposition& decomposition, size_t attention_tokens) {
    size_t worst = 0;
    for (const BapoTask& step : decomposition.steps) {
        worst = std::max(worst, BapoAnalyzer(step).bandwidth(attention_tokens));
    }
    return worst;
}

// ----------------------------------------------------------------------------
// Task library
// ----------------------------------------------------------------------------

BapoTask make_index_task(size_t length) {
    if (length < 2) {
        throw std::invalid_argument("make_index_task: length must be at least 2");
    }
    size_t index_bits = 1;
    while ((static_cast<size_t>(1) << index_bits) < length) {
        ++index_bits;
    }

    BapoTask task;
    task.name = "index";
    task.alphabet_size = 2;
    task.length = length + index_bits;
    task.evaluate = [length, index_bits](const Input& x) {
        size_t index = 0;
        for (size_t i = 0; i < index_bits; ++i) {
            index = index * 2 + static_cast<size_t>(x[length + i]);
        }
        return x[index % length];
    };
    return task;
}

BapoTask make_majority_task(size_t length) {
    BapoTask task;
    task.name = "majority";
    task.alphabet_size = 2;
    task.length = length;
    task.evaluate = [length](const Input& x) {
        const size_t ones = static_cast<size_t>(std::count(x.begin(), x.end(), 1));
        return static_cast<Output>(2 * ones >= length);
    };
    return task;
}

BapoTask make_equality_task(size_t half_length) {
    BapoTask task;
    task.name = "equality";
    task.alphabet_size = 2;
    task.length = 2 * half_length;
    task.evaluate = [half_length](const Input& x) {
        for (size_t i = 0; i < half_length; ++i) {
            if (x[i] != x[half_length + i]) {
                return 0;
            }
        }
        return 1;
    };
    return task;
}

BapoTask make_set_disjointness_task(size_t half_length) {
    BapoTask task;
    task.name = "set_disjointness";
    task.alphabet_size = 2;
    task.length = 2 * half_length;
    task.evaluate = [half_length](const Input& x) {
        for (size_t i = 0; i < half_length; ++i) {
            if (x[i] == 1 && x[half_length + i] == 1) {
                return 0;
            }
        }
        return 1;
    };
    return task;
}

BapoTask make_two_sum_task(size_t length, size_t alphabet_size, Symbol target) {
    BapoTask task;
    task.name = "two_sum";
    task.alphabet_size = alphabet_size;
    task.length = length;
    task.evaluate = [target](const Input& x) {
        for (size_t i = 0; i < x.size(); ++i) {
            for (size_t j = i + 1; j < x.size(); ++j) {
                if (x[i] + x[j] == target) {
                    return 1;
                }
            }
        }
        return 0;
    };
    return task;
}

BapoTask make_reachability_task(size_t nodes) {
    if (nodes < 2) {
        throw std::invalid_argument("make_reachability_task: need at least 2 nodes");
    }
    BapoTask task;
    task.name = "reachability";
    task.alphabet_size = 2;
    task.length = nodes * nodes;
    task.evaluate = [nodes](const Input& x) {
        std::vector<char> seen(nodes, 0);
        std::vector<size_t> stack = {0};
        seen[0] = 1;
        while (!stack.empty()) {
            const size_t node = stack.back();
            stack.pop_back();
            for (size_t next = 0; next < nodes; ++next) {
                if (!seen[next] && x[node * nodes + next] == 1) {
                    seen[next] = 1;
                    stack.push_back(next);
                }
            }
        }
        return static_cast<Output>(seen[nodes - 1]);
    };
    return task;
}

BapoTask make_parity_task(size_t length) {
    BapoTask task;
    task.name = "parity";
    task.alphabet_size = 2;
    task.length = length;
    task.evaluate = [](const Input& x) {
        int parity = 0;
        for (Symbol symbol : x) {
            parity ^= (symbol & 1);
        }
        return parity;
    };
    return task;
}

CotDecomposition make_equality_cot(size_t half_length) {
    CotDecomposition decomposition;
    decomposition.name = "equality_cot";

    const size_t base_length = 2 * half_length;
    for (size_t step = 0; step < half_length; ++step) {
        BapoTask task;
        task.name = "equality_cot.compare" + std::to_string(step);
        task.alphabet_size = 2;
        // The input carries the original string plus the bits emitted so far.
        task.length = base_length + step;
        task.evaluate = [half_length, step](const Input& x) {
            return static_cast<Output>(x[step] == x[half_length + step]);
        };
        decomposition.steps.push_back(std::move(task));
    }

    BapoTask conjunction;
    conjunction.name = "equality_cot.conjoin";
    conjunction.alphabet_size = 2;
    conjunction.length = base_length + half_length;
    conjunction.evaluate = [base_length, half_length](const Input& x) {
        for (size_t i = 0; i < half_length; ++i) {
            if (x[base_length + i] == 0) {
                return 0;
            }
        }
        return 1;
    };
    decomposition.steps.push_back(std::move(conjunction));

    return decomposition;
}

} // namespace agents
} // namespace deep_learning
} // namespace ml
