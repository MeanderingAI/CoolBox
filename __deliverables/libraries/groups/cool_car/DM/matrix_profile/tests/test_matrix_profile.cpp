#include "matrix_profile.hpp"

#include <tyst_framework.hpp>

#include <cmath>
#include <numeric>

namespace {

constexpr double kPi = 3.14159265358979323846;

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Generate a sine wave time series of length n with period p samples.
std::vector<double> sine_wave(std::size_t n, double period) {
    std::vector<double> ts(n);
    for (std::size_t i = 0; i < n; ++i) {
        ts[i] = std::sin(2.0 * kPi * static_cast<double>(i) / period);
    }
    return ts;
}

// Generate a constant time series (trivially normalises to zero).
std::vector<double> constant_signal(std::size_t n, double value = 1.0) {
    return std::vector<double>(n, value);
}

// ─── z_normalize ──────────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, ZNormalizeZeroMeanUnitVariance) {
    const std::vector<double> s = {1.0, 2.0, 3.0, 4.0, 5.0};
    const auto z = dm::matrix_profile::z_normalize(s);
    TYST_ASSERT_EQ(z.size(), s.size());

    double mean = 0.0;
    double var  = 0.0;
    for (double v : z) { mean += v; }
    mean /= static_cast<double>(z.size());
    for (double v : z) { var += (v - mean) * (v - mean); }
    var /= static_cast<double>(z.size());

    TYST_EXPECT_NEAR(mean, 0.0, 1e-10);
    TYST_EXPECT_NEAR(var,  1.0, 1e-10);
}

TYST_TEST(MatrixProfileTest, ZNormalizeConstantReturnsZeroVector) {
    const auto z = dm::matrix_profile::z_normalize({3.0, 3.0, 3.0, 3.0});
    for (double v : z) {
        TYST_EXPECT_NEAR(v, 0.0, 1e-12);
    }
}

// ─── sliding_statistics ───────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, SlidingStatisticsCorrectCount) {
    const std::vector<double> ts = {1.0, 2.0, 3.0, 4.0, 5.0};
    const std::size_t m = 3U;
    std::vector<double> means, stds;
    dm::matrix_profile::sliding_statistics(ts, m, means, stds);
    TYST_ASSERT_EQ(means.size(), 3U);
    TYST_ASSERT_EQ(stds.size(),  3U);
    TYST_EXPECT_NEAR(means[0], 2.0, 1e-12);  // (1+2+3)/3
    TYST_EXPECT_NEAR(means[1], 3.0, 1e-12);  // (2+3+4)/3
    TYST_EXPECT_NEAR(means[2], 4.0, 1e-12);  // (3+4+5)/3
}

TYST_TEST(MatrixProfileTest, SlidingStatisticsStdIsPositive) {
    const auto ts = sine_wave(32U, 8.0);
    std::vector<double> means, stds;
    dm::matrix_profile::sliding_statistics(ts, 8U, means, stds);
    for (double s : stds) {
        TYST_EXPECT_GE(s, 0.0);
    }
}

// ─── distance_profile ─────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, DistanceProfileSelfDistanceIsZero) {
    // A query matched against itself should have distance 0.
    const auto ts = sine_wave(32U, 8.0);
    const std::size_t m = 8U;
    std::vector<double> means, stds;
    dm::matrix_profile::sliding_statistics(ts, m, means, stds);
    const auto dp = dm::matrix_profile::distance_profile(ts, 0U, m, means, stds);
    TYST_ASSERT_EQ(dp.size(), ts.size() - m + 1U);
    TYST_EXPECT_NEAR(dp[0], 0.0, 1e-6);
}

TYST_TEST(MatrixProfileTest, DistanceProfileNonNegative) {
    const auto ts = sine_wave(40U, 10.0);
    const std::size_t m = 10U;
    std::vector<double> means, stds;
    dm::matrix_profile::sliding_statistics(ts, m, means, stds);
    const auto dp = dm::matrix_profile::distance_profile(ts, 2U, m, means, stds);
    for (double d : dp) {
        TYST_EXPECT_GE(d, 0.0);
    }
}

// ─── self_join ────────────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, SelfJoinProfileLength) {
    const auto ts = sine_wave(64U, 16.0);
    const std::size_t m = 16U;
    const auto result = dm::matrix_profile::self_join(ts, m);
    TYST_ASSERT_EQ(result.profile.size(), ts.size() - m + 1U);
    TYST_ASSERT_EQ(result.index.size(),   ts.size() - m + 1U);
    TYST_EXPECT_EQ(result.subsequence_length, m);
}

TYST_TEST(MatrixProfileTest, SelfJoinProfileIsNonNegative) {
    const auto ts = sine_wave(64U, 16.0);
    const auto result = dm::matrix_profile::self_join(ts, 16U);
    for (double d : result.profile) {
        TYST_EXPECT_GE(d, 0.0);
    }
}

TYST_TEST(MatrixProfileTest, SelfJoinFindsPeriodMotifInSineWave) {
    // A sine wave with period p should have a strong motif at offset p.
    const std::size_t period  = 16U;
    const std::size_t repeats = 4U;
    const auto ts = sine_wave(period * repeats, static_cast<double>(period));
    const auto result = dm::matrix_profile::self_join(ts, period);

    // At least one index should point period samples away (modulo exclusion zone)
    bool found = false;
    for (std::size_t i = 0; i < result.index.size(); ++i) {
        const std::size_t j = result.index[i];
        const std::size_t diff = (i >= j) ? (i - j) : (j - i);
        if (diff == period) {
            found = true;
            break;
        }
    }
    TYST_EXPECT_TRUE(found);
}

TYST_TEST(MatrixProfileTest, SelfJoinMinProfileNearZeroForRepeatingSeries) {
    const std::size_t period  = 16U;
    const auto ts = sine_wave(period * 4U, static_cast<double>(period));
    const auto result = dm::matrix_profile::self_join(ts, period);

    const double min_dist = *std::min_element(result.profile.begin(), result.profile.end());
    TYST_EXPECT_LT(min_dist, 0.1);
}

TYST_TEST(MatrixProfileTest, SelfJoinShortSeriesThrows) {
    TYST_EXPECT_THROW(dm::matrix_profile::self_join({1.0, 2.0}, 5U), std::invalid_argument);
    TYST_EXPECT_THROW(dm::matrix_profile::self_join({1.0, 2.0, 3.0}, 0U), std::invalid_argument);
}

// ─── ab_join ──────────────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, AbJoinResultLength) {
    const auto ts_a = sine_wave(32U, 8.0);
    const auto ts_b = sine_wave(48U, 8.0);
    const auto result = dm::matrix_profile::ab_join(ts_a, ts_b, 8U);
    TYST_ASSERT_EQ(result.profile.size(), ts_a.size() - 8U + 1U);
}

TYST_TEST(MatrixProfileTest, AbJoinIdenticalSeriesHasLowProfile) {
    const auto ts = sine_wave(48U, 12.0);
    const auto result = dm::matrix_profile::ab_join(ts, ts, 12U);
    const double min_dist = *std::min_element(result.profile.begin(), result.profile.end());
    TYST_EXPECT_LT(min_dist, 1e-9);  // identical subsequences at same position = 0
}

// ─── top_motifs ───────────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, TopMotifsReturnCorrectCount) {
    const auto ts = sine_wave(64U, 16U);
    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto motifs = dm::matrix_profile::top_motifs(mp, 2U);
    TYST_EXPECT_LE(motifs.size(), 2U);
    TYST_EXPECT_GE(motifs.size(), 1U);
}

TYST_TEST(MatrixProfileTest, TopMotifsSortedByDistance) {
    const auto ts = sine_wave(64U, 16U);
    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto motifs = dm::matrix_profile::top_motifs(mp, 3U);
    for (std::size_t i = 1; i < motifs.size(); ++i) {
        TYST_EXPECT_GE(motifs[i].distance, motifs[i - 1U].distance - 1e-12);
    }
}

// ─── top_discords ─────────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, TopDiscordsReturnHighDistances) {
    // Insert a spike anomaly into an otherwise periodic signal
    auto ts = sine_wave(64U, 16.0);
    ts[32] = 10.0;  // anomaly

    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto discords = dm::matrix_profile::top_discords(mp, 1U);
    TYST_ASSERT_EQ(discords.size(), 1U);
    // The discord should be at or near the spike position
    TYST_EXPECT_GT(discords[0].distance, 0.0);
}

// ─── arc_curve ────────────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, ArcCurveHasCorrectLength) {
    const auto ts = sine_wave(64U, 16.0);
    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto ac = dm::matrix_profile::arc_curve(mp);
    TYST_ASSERT_EQ(ac.size(), mp.profile.size());
}

TYST_TEST(MatrixProfileTest, ArcCurveValuesNonNegative) {
    const auto ts = sine_wave(64U, 16.0);
    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto ac = dm::matrix_profile::arc_curve(mp);
    for (double v : ac) {
        TYST_EXPECT_GE(v, 0.0);
    }
}

// ─── segmentation_points ──────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, SegmentationPointsCountIsNumSegmentsMinusOne) {
    const auto ts = sine_wave(64U, 16.0);
    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto ac = dm::matrix_profile::arc_curve(mp);
    const auto pts = dm::matrix_profile::segmentation_points(ac, 3U);
    TYST_EXPECT_LE(pts.size(), 2U);
}

TYST_TEST(MatrixProfileTest, SegmentationPointsAreWithinRange) {
    const auto ts = sine_wave(64U, 16.0);
    const auto mp = dm::matrix_profile::self_join(ts, 16U);
    const auto ac = dm::matrix_profile::arc_curve(mp);
    const auto pts = dm::matrix_profile::segmentation_points(ac, 3U);
    for (std::size_t p : pts) {
        TYST_EXPECT_LT(p, ac.size());
    }
}

TYST_TEST(MatrixProfileTest, SegmentationOneSegmentReturnsEmpty) {
    const std::vector<double> arc(32U, 0.5);
    const auto pts = dm::matrix_profile::segmentation_points(arc, 1U);
    TYST_EXPECT_TRUE(pts.empty());
}

// ─── extract_subsequence ──────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, ExtractSubsequenceCorrectContent) {
    const std::vector<double> ts = {10.0, 20.0, 30.0, 40.0, 50.0};
    const auto sub = dm::matrix_profile::extract_subsequence(ts, 1U, 3U);
    TYST_ASSERT_EQ(sub.size(), 3U);
    TYST_EXPECT_NEAR(sub[0], 20.0, 1e-12);
    TYST_EXPECT_NEAR(sub[1], 30.0, 1e-12);
    TYST_EXPECT_NEAR(sub[2], 40.0, 1e-12);
}

TYST_TEST(MatrixProfileTest, ExtractSubsequenceOutOfRangeThrows) {
    const std::vector<double> ts = {1.0, 2.0, 3.0};
    TYST_EXPECT_THROW(dm::matrix_profile::extract_subsequence(ts, 2U, 5U),
                      std::out_of_range);
}

// ─── batch_self_join ──────────────────────────────────────────────────────────

TYST_TEST(MatrixProfileTest, BatchSelfJoinRowCount) {
    const std::size_t rows    = 3U;
    const std::size_t cols    = 48U;
    const std::size_t m       = 12U;
    mytrix::DenseMatrix mat(static_cast<int>(rows), static_cast<int>(cols));
    for (std::size_t r = 0; r < rows; ++r) {
        const auto row = sine_wave(cols, 12.0);
        for (std::size_t c = 0; c < cols; ++c) {
            mat.at(r, c) = row[c];
        }
    }
    const auto results = dm::matrix_profile::batch_self_join(mat, m);
    TYST_ASSERT_EQ(results.size(), rows);
    for (const auto& mp : results) {
        TYST_EXPECT_EQ(mp.profile.size(), cols - m + 1U);
    }
}

}  // namespace

// Main entry point for tyst framework
#if defined(__APPLE__) && defined(__aarch64__)
#endif
