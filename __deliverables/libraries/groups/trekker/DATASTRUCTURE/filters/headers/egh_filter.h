#ifndef DATA_STRUCTURES_EGH_FILTER_H
#define DATA_STRUCTURES_EGH_FILTER_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "probabilistic_filter_utils.h"

namespace data_structures {

template<typename T>
class EGHFilter {
public:
    explicit EGHFilter(std::size_t expected_insertions = 1024, double false_positive_rate = 0.01)
        : expected_insertions_(detail::sanitize_expected_insertions(expected_insertions)),
          false_positive_rate_(detail::sanitize_false_positive_rate(false_positive_rate)),
          moduli_(build_moduli(expected_insertions_, false_positive_rate_)),
          layers_(build_layers(moduli_)),
          inserted_items_(0) {
    }

    void insert(const T& value) {
        const std::uint64_t base_hash = detail::murmur_primary_hash(value);
        for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
            layers_[layer][slot_for(base_hash, layer)] = true;
        }
        ++inserted_items_;
    }

    bool contains(const T& value) const {
        const std::uint64_t base_hash = detail::murmur_primary_hash(value);
        for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
            if (!layers_[layer][slot_for(base_hash, layer)]) {
                return false;
            }
        }
        return true;
    }

    void clear() {
        for (auto& layer : layers_) {
            std::fill(layer.begin(), layer.end(), false);
        }
        inserted_items_ = 0;
    }

    std::size_t layer_count() const {
        return layers_.size();
    }

    const std::vector<std::size_t>& moduli() const {
        return moduli_;
    }

    std::size_t total_bucket_count() const {
        return std::accumulate(moduli_.begin(), moduli_.end(), static_cast<std::size_t>(0));
    }

    std::size_t insert_count() const {
        return inserted_items_;
    }

    std::size_t approximate_count() const {
        if (layers_.empty()) {
            return 0;
        }

        double total_estimate = 0.0;
        std::size_t contributing_layers = 0;

        for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
            const double modulus = static_cast<double>(moduli_[layer]);
            const double occupied = static_cast<double>(std::count(layers_[layer].begin(), layers_[layer].end(), true));

            if (occupied <= 0.0) {
                continue;
            }
            if (occupied >= modulus) {
                total_estimate += static_cast<double>(inserted_items_);
                ++contributing_layers;
                continue;
            }

            total_estimate += -modulus * std::log(1.0 - (occupied / modulus));
            ++contributing_layers;
        }

        if (contributing_layers == 0) {
            return 0;
        }

        return static_cast<std::size_t>(std::llround(total_estimate / static_cast<double>(contributing_layers)));
    }

    double estimated_false_positive_rate() const {
        double probability = 1.0;
        for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
            const double modulus = static_cast<double>(moduli_[layer]);
            const double occupied = static_cast<double>(std::count(layers_[layer].begin(), layers_[layer].end(), true));
            probability *= modulus == 0.0 ? 1.0 : occupied / modulus;
        }
        return probability;
    }

private:
    std::size_t expected_insertions_;
    double false_positive_rate_;
    std::vector<std::size_t> moduli_;
    std::vector<std::vector<bool>> layers_;
    std::size_t inserted_items_;

    static std::vector<std::size_t> build_moduli(std::size_t expected_insertions, double false_positive_rate) {
        const std::size_t layer_count = std::max<std::size_t>(2, static_cast<std::size_t>(std::ceil(std::log2(1.0 / false_positive_rate))));
        const double per_layer_probability = std::min(0.95, std::pow(false_positive_rate, 1.0 / static_cast<double>(layer_count)));
        const double denominator = std::log(1.0 - per_layer_probability);
        const std::size_t minimum_modulus = std::max<std::size_t>(3,
            static_cast<std::size_t>(std::ceil(-static_cast<double>(expected_insertions) / denominator)));

        std::vector<std::size_t> moduli;
        moduli.reserve(layer_count);

        std::size_t candidate = minimum_modulus;
        for (std::size_t i = 0; i < layer_count; ++i) {
            candidate = detail::next_prime(candidate + i);
            moduli.push_back(candidate);
            candidate += 2;
        }

        return moduli;
    }

    static std::vector<std::vector<bool>> build_layers(const std::vector<std::size_t>& moduli) {
        std::vector<std::vector<bool>> layers;
        layers.reserve(moduli.size());
        for (const auto modulus : moduli) {
            layers.emplace_back(modulus, false);
        }
        return layers;
    }

    std::size_t slot_for(std::uint64_t base_hash, std::size_t layer) const {
        const auto mixed = detail::mix64(base_hash + (layer + 1) * 0x9e3779b97f4a7c15ULL);
        return static_cast<std::size_t>(mixed % moduli_[layer]);
    }
};

} // namespace data_structures

#endif // DATA_STRUCTURES_EGH_FILTER_H
