#ifndef ML_DEEP_LEARNING_AGENTS_BAPO_H
#define ML_DEEP_LEARNING_AGENTS_BAPO_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agents {

/// @brief The bounded attention prefix oracle (BAPO) model.
///
/// Implements Schnabel et al., "Lost in Transmission: When and Why LLMs Fail to
/// Reason Globally", NeurIPS 2025 (arXiv:2505.08140), citation key
/// @c schnabel_lost_in_transmission_2025.
///
/// A BAPO splits an input into a prefix and a suffix and bounds the two ways
/// information can cross that boundary inside a transformer:
///   - the @em prefix @em oracle summarises the whole prefix into @c a bits,
///     modelling the bandwidth of the residual stream, and
///   - the @em attention @em oracle may additionally read @c b individual
///     prefix tokens, modelling what attention heads can retrieve verbatim.
/// A decoder then produces the answer from those bits, those tokens and the
/// suffix. Problems solvable with constant (a, b) are BAPO-easy; problems whose
/// bandwidth grows with the input are BAPO-hard.

using Symbol = int;
using Input = std::vector<Symbol>;
using Output = int;
using TaskFunction = std::function<Output(const Input&)>;

/// A decision problem over fixed-length strings from a finite alphabet.
struct BapoTask {
    std::string name;
    size_t length = 0;         ///< n
    size_t alphabet_size = 2;  ///< |Sigma|
    TaskFunction evaluate;

    /// Total number of inputs, alphabet_size^length.
    size_t input_space_size() const;
};

/// Bandwidth budget (a, b) of a BAPO.
struct BapoBandwidth {
    size_t prefix_bits = 0;      ///< a: bits emitted by the prefix oracle.
    size_t attention_tokens = 0; ///< b: prefix tokens the decoder may read.
};

/// The three components a BAPO is built from.
struct BapoOracles {
    /// Summarises the prefix into at most @c prefix_bits bits.
    std::function<std::vector<bool>(const Input&)> prefix_oracle;
    /// Picks at most @c attention_tokens prefix positions, given the suffix.
    std::function<std::vector<size_t>(const Input&)> attention_oracle;
    /// Produces the answer from the bits, the attended tokens and the suffix.
    std::function<Output(const std::vector<bool>&, const std::vector<Symbol>&, const Input&)> decoder;
};

/// An executable BAPO. Running it enforces the declared bandwidth limits, so a
/// machine that cheats by passing more information simply throws.
class BapoMachine {
public:
    BapoMachine(BapoBandwidth bandwidth, BapoOracles oracles);

    const BapoBandwidth& bandwidth() const { return bandwidth_; }

    /// Evaluates the input, splitting it after @p split symbols.
    Output run(const Input& input, size_t split) const;

    /// True when the machine reproduces @p task on every input at this split.
    bool solves(const BapoTask& task, size_t split) const;

private:
    BapoBandwidth bandwidth_;
    BapoOracles oracles_;
};

/// Exact bandwidth analysis by enumerating the input space.
///
/// With no attention the minimum prefix bandwidth is the one-way deterministic
/// communication complexity of the task: two prefixes may share a code word
/// exactly when no suffix distinguishes them.
class BapoAnalyzer {
public:
    /// @param max_input_space Guard against enumerating an intractable space.
    explicit BapoAnalyzer(BapoTask task, size_t max_input_space = 1u << 22);

    const BapoTask& task() const { return task_; }

    /// Number of distinguishable prefix classes at @p split.
    size_t prefix_classes(size_t split) const;

    /// ceil(log2(prefix_classes(split))): bandwidth needed with b = 0.
    size_t minimum_prefix_bits(size_t split) const;

    /// Minimum prefix bits when the decoder may also read @p attention_tokens
    /// prefix positions, chosen per suffix by the attention oracle.
    ///
    /// The attended positions are picked greedily for each suffix and the
    /// resulting prefix-confusability graph is then coloured, so the result is
    /// an upper bound on the true BAPO bandwidth. It is exact whenever the
    /// colouring bounds meet, which includes the two interesting extremes: a
    /// task attention can solve outright (0 bits) and a task whose confusable
    /// prefixes form a clique.
    size_t minimum_prefix_bits_with_attention(size_t split, size_t attention_tokens) const;

    /// Worst split, which is the bandwidth of the task as a whole.
    size_t bandwidth(size_t attention_tokens = 0) const;
    size_t hardest_split(size_t attention_tokens = 0) const;

    /// A task is BAPO-easy for this input size when its bandwidth fits @p budget_bits.
    bool is_bapo_easy(size_t budget_bits, size_t attention_tokens = 0) const;

private:
    /// Groups prefixes by the vector of answers they induce over all suffixes.
    std::vector<size_t> class_of_each_prefix(size_t split) const;

    BapoTask task_;
    size_t max_input_space_;
};

/// Chain of thought: the model writes intermediate results into the context, so
/// the original task is replaced by a sequence of sub-tasks that each read the
/// tokens emitted so far. The paper proves this can turn a BAPO-hard problem
/// into a BAPO-easy one; @c cot_bandwidth measures that directly.
struct CotDecomposition {
    std::string name;
    /// Sub-tasks in emission order; the last one must produce the final answer.
    std::vector<BapoTask> steps;
};

/// Largest bandwidth required by any single step, with the given attention budget.
size_t cot_bandwidth(const CotDecomposition& decomposition, size_t attention_tokens = 0);

// ----------------------------------------------------------------------------
// Standard task library
// ----------------------------------------------------------------------------

/// Needle in a haystack: the final symbols encode an index, the answer is the
/// symbol stored there. Easy with one attention token, hard without.
BapoTask make_index_task(size_t length);

/// Is the number of ones at least half the length?
BapoTask make_majority_task(size_t length);

/// Do the two halves of the input agree?
BapoTask make_equality_task(size_t half_length);

/// Do the two halves share no position holding a one?
BapoTask make_set_disjointness_task(size_t half_length);

/// Is there a pair of positions whose symbols sum to the target?
BapoTask make_two_sum_task(size_t length, size_t alphabet_size, Symbol target);

/// Is the last node reachable from the first in a directed graph given as an
/// edge list over @p nodes vertices?
BapoTask make_reachability_task(size_t nodes);

/// Parity of the number of ones.
BapoTask make_parity_task(size_t length);

/// Equality decomposed into per-position comparisons, then a conjunction. Each
/// step only has to compare two symbols, which attention can fetch directly.
CotDecomposition make_equality_cot(size_t half_length);

} // namespace agents
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_AGENTS_BAPO_H
