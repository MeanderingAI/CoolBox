#include "cdeepex.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace ml {
namespace deep_learning {

size_t CDeepExExplainer::class_count(const Tensor& output) {
    if (output.shape().empty()) {
        throw std::invalid_argument("model output must have at least one dimension");
    }
    if (output.shape().size() == 1) {
        return output.shape()[0];
    }
    if (output.shape().size() == 2 && output.shape()[0] == 1) {
        return output.shape()[1];
    }
    throw std::invalid_argument("expected model output shape [classes] or [1, classes]");
}

size_t CDeepExExplainer::class_offset(const Tensor& output, size_t class_index) {
    if (output.shape().size() == 1) {
        return class_index;
    }
    return class_index;
}

size_t CDeepExExplainer::pick_default_foil(const Tensor& output, size_t target_class) {
    const size_t classes = class_count(output);
    if (classes < 2) {
        throw std::invalid_argument("contrastive explanation requires at least 2 classes");
    }
    if (target_class >= classes) {
        throw std::invalid_argument("target_class is out of range");
    }

    size_t foil = static_cast<size_t>(-1);
    double best = -std::numeric_limits<double>::infinity();
    for (size_t c = 0; c < classes; ++c) {
        if (c == target_class) {
            continue;
        }
        const double score = output.data()[class_offset(output, c)];
        if (score > best) {
            best = score;
            foil = c;
        }
    }
    return foil;
}

Tensor CDeepExExplainer::seeded_output_gradient(const Tensor& output, size_t class_index) {
    Tensor seed(output.shape(), 0.0);
    seed.data()[class_offset(output, class_index)] = 1.0;
    return seed;
}

Tensor CDeepExExplainer::relu_abs_difference(const Tensor& positive, const Tensor& negative) {
    if (positive.shape() != negative.shape()) {
        throw std::invalid_argument("positive and negative gradients must have matching shapes");
    }

    Tensor out(positive.shape(), 0.0);
    for (size_t i = 0; i < out.size(); ++i) {
        const double delta = std::abs(positive.data()[i]) - std::abs(negative.data()[i]);
        out.data()[i] = std::max(0.0, delta);
    }
    return out;
}

Tensor CDeepExExplainer::min_max_normalize(const Tensor& input) {
    if (input.size() == 0) {
        return input;
    }

    auto minmax = std::minmax_element(input.data().begin(), input.data().end());
    const double min_v = *minmax.first;
    const double max_v = *minmax.second;

    if (max_v <= min_v) {
        return Tensor(input.shape(), 0.0);
    }

    Tensor out = input.clone();
    const double denom = max_v - min_v;
    for (size_t i = 0; i < out.size(); ++i) {
        out.data()[i] = (out.data()[i] - min_v) / denom;
    }
    return out;
}

std::vector<size_t> CDeepExExplainer::top_feature_indices(const Tensor& input, size_t max_features) {
    std::vector<size_t> idx(input.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&input](size_t a, size_t b) {
        return std::abs(input.data()[a]) > std::abs(input.data()[b]);
    });
    if (idx.size() > max_features) {
        idx.resize(max_features);
    }
    return idx;
}

std::vector<double> CDeepExExplainer::solve_weighted_ridge(const std::vector<std::vector<double>>& X,
                                                           const std::vector<double>& y,
                                                           const std::vector<double>& w,
                                                           double ridge) {
    if (X.empty() || X.size() != y.size() || X.size() != w.size()) {
        throw std::invalid_argument("invalid weighted ridge inputs");
    }

    const size_t n = X.size();
    const size_t p = X[0].size();

    std::vector<std::vector<double>> a(p, std::vector<double>(p, 0.0));
    std::vector<double> b(p, 0.0);

    for (size_t i = 0; i < n; ++i) {
        const double wi = std::max(0.0, w[i]);
        for (size_t r = 0; r < p; ++r) {
            b[r] += wi * X[i][r] * y[i];
            for (size_t c = 0; c < p; ++c) {
                a[r][c] += wi * X[i][r] * X[i][c];
            }
        }
    }

    for (size_t j = 1; j < p; ++j) {
        a[j][j] += ridge;
    }

    for (size_t col = 0; col < p; ++col) {
        size_t pivot = col;
        double best = std::abs(a[col][col]);
        for (size_t r = col + 1; r < p; ++r) {
            const double candidate = std::abs(a[r][col]);
            if (candidate > best) {
                best = candidate;
                pivot = r;
            }
        }
        if (best < 1e-12) {
            continue;
        }
        if (pivot != col) {
            std::swap(a[pivot], a[col]);
            std::swap(b[pivot], b[col]);
        }

        const double diag = a[col][col];
        for (size_t c = col; c < p; ++c) {
            a[col][c] /= diag;
        }
        b[col] /= diag;

        for (size_t r = 0; r < p; ++r) {
            if (r == col) {
                continue;
            }
            const double factor = a[r][col];
            if (std::abs(factor) < 1e-16) {
                continue;
            }
            for (size_t c = col; c < p; ++c) {
                a[r][c] -= factor * a[col][c];
            }
            b[r] -= factor * b[col];
        }
    }

    return b;
}

double CDeepExExplainer::target_score(const Tensor& output, size_t class_index) {
    const size_t classes = class_count(output);
    if (class_index >= classes) {
        throw std::invalid_argument("target_class is out of range");
    }
    return output.data()[class_offset(output, class_index)];
}

double CDeepExExplainer::binomial_coeff(size_t n, size_t k) {
    if (k > n) {
        return 0.0;
    }
    if (k == 0 || k == n) {
        return 1.0;
    }
    k = std::min(k, n - k);
    double c = 1.0;
    for (size_t i = 1; i <= k; ++i) {
        c *= static_cast<double>(n - (k - i));
        c /= static_cast<double>(i);
    }
    return c;
}

ContrastiveExplanation CDeepExExplainer::explain(const Tensor& input,
                                                 size_t target_class,
                                                 size_t foil_class,
                                                 bool normalize) {
    Tensor output = network_.forward(input);
    const size_t classes = class_count(output);

    if (target_class >= classes) {
        throw std::invalid_argument("target_class is out of range");
    }

    if (foil_class == static_cast<size_t>(-1)) {
        foil_class = pick_default_foil(output, target_class);
    }
    if (foil_class >= classes) {
        throw std::invalid_argument("foil_class is out of range");
    }
    if (foil_class == target_class) {
        throw std::invalid_argument("foil_class must be different from target_class");
    }

    const Tensor target_seed = seeded_output_gradient(output, target_class);
    const Tensor foil_seed = seeded_output_gradient(output, foil_class);

    Tensor positive_grad = network_.input_gradient(input, target_seed);
    Tensor negative_grad = network_.input_gradient(input, foil_seed);

    Tensor contrastive = relu_abs_difference(positive_grad, negative_grad);
    if (normalize) {
        contrastive = min_max_normalize(contrastive);
    }

    return ContrastiveExplanation{
        output,
        positive_grad,
        negative_grad,
        contrastive,
        target_class,
        foil_class
    };
}

LimeExplanation CDeepExExplainer::explain_lime(const Tensor& input,
                                               size_t target_class,
                                               size_t num_samples,
                                               size_t max_features,
                                               double kernel_width,
                                               double ridge,
                                               unsigned int seed) {
    if (num_samples < 8) {
        throw std::invalid_argument("num_samples must be >= 8");
    }
    if (max_features == 0) {
        throw std::invalid_argument("max_features must be > 0");
    }
    if (kernel_width <= 0.0) {
        throw std::invalid_argument("kernel_width must be > 0");
    }

    const Tensor base_output = network_.forward(input);
    const size_t classes = class_count(base_output);
    if (target_class >= classes) {
        throw std::invalid_argument("target_class is out of range");
    }

    const std::vector<size_t> selected = top_feature_indices(input, max_features);
    const size_t m = selected.size();

    Tensor baseline(input.shape(), 0.0);

    std::mt19937 gen(seed);
    std::bernoulli_distribution onoff(0.5);

    std::vector<std::vector<double>> X;
    std::vector<double> y;
    std::vector<double> weights;
    X.reserve(num_samples + 1);
    y.reserve(num_samples + 1);
    weights.reserve(num_samples + 1);

    auto add_sample = [&](const std::vector<double>& mask) {
        Tensor perturbed = input.clone();
        for (size_t j = 0; j < m; ++j) {
            if (mask[j] < 0.5) {
                const size_t idx = selected[j];
                perturbed.data()[idx] = baseline.data()[idx];
            }
        }

        const Tensor out = network_.forward(perturbed);
        const double score = target_score(out, target_class);

        double dist2 = 0.0;
        for (size_t j = 0; j < m; ++j) {
            if (mask[j] < 0.5) {
                const size_t idx = selected[j];
                const double d = input.data()[idx] - baseline.data()[idx];
                dist2 += d * d;
            }
        }
        const double weight = std::exp(-dist2 / (kernel_width * kernel_width + 1e-12));

        std::vector<double> row(m + 1, 1.0);
        for (size_t j = 0; j < m; ++j) {
            row[j + 1] = mask[j];
        }
        X.push_back(std::move(row));
        y.push_back(score);
        weights.push_back(weight);
    };

    std::vector<double> full_mask(m, 1.0);
    add_sample(full_mask);

    for (size_t i = 0; i < num_samples; ++i) {
        std::vector<double> mask(m, 0.0);
        for (size_t j = 0; j < m; ++j) {
            mask[j] = onoff(gen) ? 1.0 : 0.0;
        }
        add_sample(mask);
    }

    const std::vector<double> beta = solve_weighted_ridge(X, y, weights, ridge);

    Tensor importance(input.shape(), 0.0);
    for (size_t j = 0; j < m; ++j) {
        importance.data()[selected[j]] = beta[j + 1];
    }

    const double intercept = beta[0];
    double local_prediction = intercept;
    for (size_t j = 0; j < m; ++j) {
        local_prediction += beta[j + 1];
    }

    return LimeExplanation{
        base_output,
        importance,
        intercept,
        local_prediction,
        target_score(base_output, target_class),
        target_class,
        m
    };
}

IntegratedGradientsExplanation CDeepExExplainer::explain_integrated_gradients(
    const Tensor& input,
    size_t target_class,
    size_t steps,
    bool normalize) {
    if (steps == 0) {
        throw std::invalid_argument("steps must be > 0");
    }

    const Tensor output = network_.forward(input);
    const size_t classes = class_count(output);
    if (target_class >= classes) {
        throw std::invalid_argument("target_class is out of range");
    }

    Tensor baseline(input.shape(), 0.0);
    Tensor delta = input - baseline;
    Tensor avg_grad(input.shape(), 0.0);

    for (size_t s = 1; s <= steps; ++s) {
        const double alpha = static_cast<double>(s) / static_cast<double>(steps);
        Tensor step_input = baseline + (delta * alpha);
        const Tensor step_output = network_.forward(step_input);
        const Tensor seed = seeded_output_gradient(step_output, target_class);
        Tensor grad = network_.input_gradient(step_input, seed);
        avg_grad += grad;
    }
    avg_grad = avg_grad / static_cast<double>(steps);

    Tensor attribution = delta * avg_grad;
    if (normalize) {
        attribution = min_max_normalize(attribution);
    }

    return IntegratedGradientsExplanation{
        output,
        attribution,
        attribution.sum(),
        target_class
    };
}

ShapExplanation CDeepExExplainer::explain_kernel_shap(const Tensor& input,
                                                      size_t target_class,
                                                      size_t num_samples,
                                                      size_t max_features,
                                                      double ridge,
                                                      unsigned int seed) {
    if (num_samples < 16) {
        throw std::invalid_argument("num_samples must be >= 16");
    }
    if (max_features == 0) {
        throw std::invalid_argument("max_features must be > 0");
    }

    const Tensor base_output = network_.forward(input);
    const size_t classes = class_count(base_output);
    if (target_class >= classes) {
        throw std::invalid_argument("target_class is out of range");
    }

    const std::vector<size_t> selected = top_feature_indices(input, max_features);
    const size_t m = selected.size();
    if (m < 2) {
        throw std::invalid_argument("KernelSHAP requires at least 2 selected features");
    }

    Tensor baseline(input.shape(), 0.0);

    std::mt19937 gen(seed);
    std::bernoulli_distribution onoff(0.5);

    std::vector<std::vector<double>> X;
    std::vector<double> y;
    std::vector<double> weights;
    X.reserve(num_samples + 2);
    y.reserve(num_samples + 2);
    weights.reserve(num_samples + 2);

    auto add_masked_sample = [&](const std::vector<double>& mask, double weight) {
        Tensor perturbed = input.clone();
        for (size_t j = 0; j < m; ++j) {
            if (mask[j] < 0.5) {
                const size_t idx = selected[j];
                perturbed.data()[idx] = baseline.data()[idx];
            }
        }

        const Tensor out = network_.forward(perturbed);
        const double score = target_score(out, target_class);

        std::vector<double> row(m + 1, 1.0);
        for (size_t j = 0; j < m; ++j) {
            row[j + 1] = mask[j];
        }
        X.push_back(std::move(row));
        y.push_back(score);
        weights.push_back(weight);
    };

    std::vector<double> zeros(m, 0.0);
    std::vector<double> ones(m, 1.0);
    add_masked_sample(zeros, 1e6);
    add_masked_sample(ones, 1e6);

    for (size_t i = 0; i < num_samples; ++i) {
        std::vector<double> mask(m, 0.0);
        size_t s = 0;
        for (size_t j = 0; j < m; ++j) {
            mask[j] = onoff(gen) ? 1.0 : 0.0;
            if (mask[j] > 0.5) {
                ++s;
            }
        }
        if (s == 0 || s == m) {
            continue;
        }

        const double denom = binomial_coeff(m, s) * static_cast<double>(s) * static_cast<double>(m - s);
        const double w = (static_cast<double>(m - 1) / std::max(1e-12, denom));
        add_masked_sample(mask, w);
    }

    const std::vector<double> beta = solve_weighted_ridge(X, y, weights, ridge);

    Tensor shap_values(input.shape(), 0.0);
    for (size_t j = 0; j < m; ++j) {
        shap_values.data()[selected[j]] = beta[j + 1];
    }

    const double base_value = beta[0];
    double local_prediction = base_value;
    for (size_t j = 0; j < m; ++j) {
        local_prediction += beta[j + 1];
    }

    return ShapExplanation{
        base_output,
        shap_values,
        base_value,
        local_prediction,
        target_score(base_output, target_class),
        target_class,
        m
    };
}

} // namespace deep_learning
} // namespace ml
