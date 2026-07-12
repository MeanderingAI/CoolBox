#pragma once

/**
 * trekker::algorithm::scheduling
 * ──────────────────────────────────────────────────────────────────────────
 * Header-only scheduling helpers for common CPU/task dispatch strategies.
 *
 * Provided techniques:
 *   - round_robin_sequence                deterministic cyclic dispatch
 *   - stride_sequence                     proportional-share scheduling
 *   - lottery_sequence                    randomized proportional-share scheduling
 *   - priority_order                      static priority ranking
 *   - earliest_deadline_first_order       deadline-driven ordering
 *   - shortest_job_first_order            non-preemptive shortest-job-first
 *   - shortest_remaining_time_sequence    preemptive shortest-remaining-time-first
 *
 * The helpers intentionally operate on simple vector inputs so they can be
 * used in tests, simulations, and lightweight policy experiments without
 * pulling in a larger runtime.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace trekker {
namespace algorithm {
namespace scheduling {

using job_id_t = std::size_t;

namespace detail {

template <typename ScoreFn>
job_id_t select_best_index(std::size_t count, ScoreFn score_fn) {
    if (count == 0) {
        return 0;
    }

    job_id_t best_index = 0;
    auto best_score = score_fn(0);
    for (job_id_t index = 1; index < count; ++index) {
        auto score = score_fn(index);
        if (score < best_score || (score == best_score && index < best_index)) {
            best_score = score;
            best_index = index;
        }
    }
    return best_index;
}

template <typename ScoreFn>
std::vector<job_id_t> order_by_score(std::size_t count, ScoreFn score_fn) {
    std::vector<job_id_t> order;
    order.reserve(count);
    std::vector<bool> used(count, false);

    for (std::size_t emitted = 0; emitted < count; ++emitted) {
        job_id_t best_index = count;
        auto best_score = std::numeric_limits<long double>::max();

        for (job_id_t index = 0; index < count; ++index) {
            if (used[index]) {
                continue;
            }
            auto score = score_fn(index);
            if (best_index == count || score < best_score || (score == best_score && index < best_index)) {
                best_score = score;
                best_index = index;
            }
        }

        if (best_index == count) {
            break;
        }

        used[best_index] = true;
        order.push_back(best_index);
    }

    return order;
}

} // namespace detail

inline std::vector<job_id_t> round_robin_sequence(
    std::size_t job_count,
    std::size_t quanta,
    std::size_t start_index = 0
) {
    std::vector<job_id_t> sequence;
    if (job_count == 0 || quanta == 0) {
        return sequence;
    }

    sequence.reserve(quanta);
    for (std::size_t step = 0; step < quanta; ++step) {
        sequence.push_back((start_index + step) % job_count);
    }
    return sequence;
}

inline std::vector<job_id_t> stride_sequence(
    const std::vector<std::size_t>& tickets,
    std::size_t quanta,
    long double scale = 1000000.0L
) {
    std::vector<job_id_t> sequence;
    if (quanta == 0 || tickets.empty()) {
        return sequence;
    }

    const std::size_t total_tickets = std::accumulate(tickets.begin(), tickets.end(), std::size_t{0});
    if (total_tickets == 0) {
        throw std::invalid_argument("stride_sequence requires at least one positive ticket count");
    }

    std::vector<long double> stride(tickets.size(), std::numeric_limits<long double>::infinity());
    std::vector<long double> pass(tickets.size(), 0.0L);
    for (std::size_t index = 0; index < tickets.size(); ++index) {
        if (tickets[index] > 0) {
            stride[index] = scale / static_cast<long double>(tickets[index]);
        }
    }

    sequence.reserve(quanta);
    for (std::size_t step = 0; step < quanta; ++step) {
        job_id_t selected = tickets.size();
        long double selected_pass = std::numeric_limits<long double>::infinity();
        for (job_id_t index = 0; index < tickets.size(); ++index) {
            if (tickets[index] == 0) {
                continue;
            }
            if (selected == tickets.size() || pass[index] < selected_pass || (pass[index] == selected_pass && index < selected)) {
                selected = index;
                selected_pass = pass[index];
            }
        }

        if (selected == tickets.size()) {
            break;
        }

        sequence.push_back(selected);
        pass[selected] += stride[selected];
    }

    return sequence;
}

inline std::vector<job_id_t> lottery_sequence(
    const std::vector<std::size_t>& tickets,
    std::size_t quanta,
    std::uint64_t seed = 42
) {
    std::vector<job_id_t> sequence;
    if (quanta == 0 || tickets.empty()) {
        return sequence;
    }

    const std::size_t total_tickets = std::accumulate(tickets.begin(), tickets.end(), std::size_t{0});
    if (total_tickets == 0) {
        throw std::invalid_argument("lottery_sequence requires at least one positive ticket count");
    }

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<std::size_t> draw(0, total_tickets - 1);

    sequence.reserve(quanta);
    for (std::size_t step = 0; step < quanta; ++step) {
        const std::size_t winning_ticket = draw(rng);
        std::size_t cumulative = 0;
        for (job_id_t index = 0; index < tickets.size(); ++index) {
            cumulative += tickets[index];
            if (winning_ticket < cumulative) {
                sequence.push_back(index);
                break;
            }
        }
    }

    return sequence;
}

inline std::vector<job_id_t> priority_order(const std::vector<int>& priorities) {
    return detail::order_by_score(priorities.size(), [&priorities](job_id_t index) {
        return -static_cast<long double>(priorities[index]);
    });
}

inline std::vector<job_id_t> earliest_deadline_first_order(const std::vector<std::size_t>& deadlines) {
    return detail::order_by_score(deadlines.size(), [&deadlines](job_id_t index) {
        return static_cast<long double>(deadlines[index]);
    });
}

inline std::vector<job_id_t> shortest_job_first_order(const std::vector<std::size_t>& bursts) {
    return detail::order_by_score(bursts.size(), [&bursts](job_id_t index) {
        return static_cast<long double>(bursts[index]);
    });
}

inline std::vector<job_id_t> shortest_remaining_time_sequence(std::vector<std::size_t> bursts) {
    std::vector<job_id_t> sequence;
    if (bursts.empty()) {
        return sequence;
    }

    const std::size_t total_time = std::accumulate(bursts.begin(), bursts.end(), std::size_t{0});
    sequence.reserve(total_time);

    while (true) {
        job_id_t selected = bursts.size();
        std::size_t best_remaining = std::numeric_limits<std::size_t>::max();
        for (job_id_t index = 0; index < bursts.size(); ++index) {
            if (bursts[index] == 0) {
                continue;
            }
            if (selected == bursts.size() || bursts[index] < best_remaining || (bursts[index] == best_remaining && index < selected)) {
                selected = index;
                best_remaining = bursts[index];
            }
        }

        if (selected == bursts.size()) {
            break;
        }

        sequence.push_back(selected);
        --bursts[selected];
    }

    return sequence;
}

} // namespace scheduling
} // namespace algorithm
} // namespace trekker
