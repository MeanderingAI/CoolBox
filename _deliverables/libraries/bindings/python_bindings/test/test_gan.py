import math

import pytest

from ml_toolbox import GAN
from ml_toolbox import ml_core


def test_gan_trains_both_networks_and_generates_samples():
    dl = ml_core.deep_learning
    gan = GAN(latent_dim=3, sample_dim=2, hidden_dims=(4,), learning_rate=0.01)
    initial_generator_weights = list(gan._generator[0].weights().data())

    real_batch = dl.Tensor([2, 2], [0.2, -0.3, -0.4, 0.5])
    discriminator_loss, generator_loss = gan.train_batch(real_batch)
    generated = gan.sample(5)

    assert math.isfinite(discriminator_loss)
    assert math.isfinite(generator_loss)
    assert generated.shape() == [5, 2]
    assert list(gan._generator[0].weights().data()) != initial_generator_weights


def test_gan_rejects_real_batch_with_wrong_feature_count():
    gan = GAN(latent_dim=3, sample_dim=2, hidden_dims=(4,))
    wrong_shape = ml_core.deep_learning.Tensor([2, 1], [0.0, 0.5])

    with pytest.raises(ValueError, match="real_samples must have shape"):
        gan.train_batch(wrong_shape)


def test_gan_validates_configuration():
    with pytest.raises(ValueError, match="hidden_dims"):
        GAN(latent_dim=3, sample_dim=2, hidden_dims=())
