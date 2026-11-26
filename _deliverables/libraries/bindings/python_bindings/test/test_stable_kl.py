import numpy as np
import pytest

from ml_toolbox import ml_core

stable_kl = ml_core.stable_kl


def test_expm1_minus_x_is_zero_at_zero():
    value = stable_kl.expm1_minus_x(np.array([0.0, 1e-8, -1e-8]))
    assert value[0] == 0.0
    assert np.max(np.abs(value)) < 1e-15


def test_normal_kl_is_zero_for_identical_distributions():
    mu = np.array([1.0, -2.0])
    logvar = np.array([0.25, -0.5])
    np.testing.assert_array_equal(
        stable_kl.normal_kl(mu, logvar, mu, logvar),
        np.zeros(2),
    )


def test_logits_kl_is_exactly_zero_for_shifted_identical_logits():
    logits = np.array([[[1.0, 2.0, 3.0], [4.0, 4.0, 4.0]]])
    shifted = logits + 100.0
    np.testing.assert_array_equal(
        stable_kl.kl_from_logits(logits, shifted),
        np.zeros((1, 2)),
    )


@pytest.mark.parametrize("log_target", [False, True])
def test_kl_divergence_reduction(log_target):
    target = np.array([[0.25, 0.75], [0.5, 0.5]])
    input = np.log(target)
    if log_target:
        target = np.log(target)

    value = stable_kl.kl_divergence(
        target,
        input,
        reduction="batchmean",
        log_target=log_target,
    )
    assert value == 0.0
