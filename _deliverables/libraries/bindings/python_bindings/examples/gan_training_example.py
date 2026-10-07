"""Train a small CoolBox GAN and plot its discriminator loss."""

import random

import matplotlib.pyplot as plt

import ml_toolbox as ml


def make_real_batch(batch_size, dl):
    # A simple two-mode distribution, already scaled to the generator's [-1, 1].
    samples = []
    for _ in range(batch_size):
        center = random.choice((-0.5, 0.5))
        samples.append(max(-1.0, min(1.0, random.gauss(center, 0.12))))
    return dl.Tensor([batch_size, 1], samples)


def main():
    random.seed(7)
    dl = ml.ml_core.deep_learning
    gan = ml.GAN(latent_dim=8, sample_dim=1, hidden_dims=(32, 32))

    discriminator_losses = []
    generator_losses = []
    steps = 1000
    batch_size = 32

    for step in range(steps):
        real_batch = make_real_batch(batch_size, dl)
        d_loss, g_loss = gan.train_batch(real_batch)
        discriminator_losses.append(d_loss)
        generator_losses.append(g_loss)

        if (step + 1) % 100 == 0:
            print(
                f"Step {step + 1}/{steps} | "
                f"Discriminator loss: {d_loss:.4f} | "
                f"Generator loss: {g_loss:.4f}"
            )

    plt.plot(discriminator_losses, label="Discriminator loss")
    plt.plot(generator_losses, label="Generator loss")
    plt.xlabel("Training step")
    plt.ylabel("Binary cross-entropy loss")
    plt.title("CoolBox GAN training losses")
    plt.legend()
    plt.tight_layout()
    plt.savefig("gan_losses.png", dpi=150)
    plt.show()


if __name__ == "__main__":
    main()
