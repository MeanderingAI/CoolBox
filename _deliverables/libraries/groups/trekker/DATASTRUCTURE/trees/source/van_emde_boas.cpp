#include "../headers/van_emde_boas.h"


namespace data_structures {

// ── Helpers ───────────────────────────────────────────────────────────────

uint64_t VanEmdeBoasTree::round_up_pow2(uint64_t n) {
    if (n <= 2) return 2;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

// Count the number of bits needed to represent (u_ - 1).
// i.e. floor(log2(u_)) when u_ is a power of 2.
static uint64_t bit_length(uint64_t u) {
    uint64_t bits = 0;
    uint64_t tmp  = u - 1;
    while (tmp > 0) { tmp >>= 1; ++bits; }
    return bits;
}

uint64_t VanEmdeBoasTree::sqrt_upper() const {
    // 2^ceil(b/2) where b = bit_length(u_)
    uint64_t b = bit_length(u_);
    return uint64_t(1) << ((b + 1) / 2);
}

uint64_t VanEmdeBoasTree::sqrt_lower() const {
    // 2^floor(b/2)
    uint64_t b = bit_length(u_);
    return uint64_t(1) << (b / 2);
}

// ── Construction ──────────────────────────────────────────────────────────

VanEmdeBoasTree::VanEmdeBoasTree(uint64_t universe_size)
    : u_(round_up_pow2(universe_size))
    , min_(NONE)
    , max_(NONE)
{
    if (u_ > 2) {
        const uint64_t sq_up  = sqrt_upper();
        const uint64_t sq_lo  = sqrt_lower();
        summary_ = std::make_unique<VanEmdeBoasTree>(sq_up);
        clusters_.resize(sq_up);
        for (uint64_t i = 0; i < sq_up; ++i) {
            clusters_[i] = std::make_unique<VanEmdeBoasTree>(sq_lo);
        }
    }
}

// ── Insert ────────────────────────────────────────────────────────────────

void VanEmdeBoasTree::insert(uint64_t x) {
    if (x >= u_) return;

    if (min_ == NONE) {
        // Tree was empty — store as both min and max (not propagated further)
        min_ = max_ = x;
        return;
    }

    if (x == min_ || x == max_) return; // already present (base check)

    if (x < min_) std::swap(x, min_);   // keep min_ as the "lazy" minimum

    if (u_ > 2) {
        const uint64_t h = high(x);
        const uint64_t l = low(x);
        if (clusters_[h]->empty()) {
            summary_->insert(h);
        }
        clusters_[h]->insert(l);
    }

    if (x > max_) max_ = x;
}

// ── Remove ────────────────────────────────────────────────────────────────

void VanEmdeBoasTree::remove(uint64_t x) {
    if (min_ == NONE) return;   // empty

    if (min_ == max_) {
        // Only one element
        if (x == min_) min_ = max_ = NONE;
        return;
    }

    if (u_ == 2) {
        // Two possible elements: 0 and 1
        min_ = max_ = (x == 0) ? 1u : 0u;
        return;
    }

    if (x == min_) {
        // Replace min_ with the smallest element stored in the clusters
        const uint64_t first = summary_->minimum();
        x     = compose(first, clusters_[first]->minimum());
        min_  = x;
    }

    const uint64_t h = high(x);
    const uint64_t l = low(x);
    clusters_[h]->remove(l);

    if (clusters_[h]->empty()) {
        summary_->remove(h);
        if (x == max_) {
            const uint64_t sum_max = summary_->maximum();
            max_ = (sum_max == NONE) ? min_ : compose(sum_max, clusters_[sum_max]->maximum());
        }
    } else if (x == max_) {
        max_ = compose(h, clusters_[h]->maximum());
    }
}

// ── Contains ──────────────────────────────────────────────────────────────

bool VanEmdeBoasTree::contains(uint64_t x) const {
    if (x >= u_)   return false;
    if (x == min_ || x == max_) return true;
    if (u_ == 2)   return false;
    return clusters_[high(x)]->contains(low(x));
}

// ── Successor ─────────────────────────────────────────────────────────────

uint64_t VanEmdeBoasTree::successor(uint64_t x) const {
    if (u_ == 2) {
        if (x == 0 && max_ == 1) return 1;
        return NONE;
    }

    // If x is less than the minimum, the minimum is its successor
    if (min_ != NONE && x < min_) return min_;

    const uint64_t h       = high(x);
    const uint64_t l       = low(x);
    const uint64_t max_low = clusters_[h]->maximum();

    if (max_low != NONE && l < max_low) {
        // Successor is within the same cluster
        return compose(h, clusters_[h]->successor(l));
    }

    // Successor is the minimum of the next non-empty cluster
    const uint64_t next_cluster = summary_->successor(h);
    if (next_cluster == NONE) return NONE;
    return compose(next_cluster, clusters_[next_cluster]->minimum());
}

// ── Predecessor ───────────────────────────────────────────────────────────

uint64_t VanEmdeBoasTree::predecessor(uint64_t x) const {
    if (u_ == 2) {
        if (x == 1 && min_ == 0) return 0;
        return NONE;
    }

    // If x is greater than the maximum, the maximum is its predecessor
    if (max_ != NONE && x > max_) return max_;

    const uint64_t h       = high(x);
    const uint64_t l       = low(x);
    const uint64_t min_low = clusters_[h]->minimum();

    if (min_low != NONE && l > min_low) {
        // Predecessor is within the same cluster
        return compose(h, clusters_[h]->predecessor(l));
    }

    // Predecessor is the maximum of the previous non-empty cluster
    const uint64_t prev_cluster = summary_->predecessor(h);
    if (prev_cluster == NONE) {
        // The only remaining candidate is min_ (stored lazily)
        if (min_ != NONE && x > min_) return min_;
        return NONE;
    }
    return compose(prev_cluster, clusters_[prev_cluster]->maximum());
}

} // namespace data_structures
