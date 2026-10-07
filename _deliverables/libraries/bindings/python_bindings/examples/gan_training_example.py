"""Train a small CoolBox GAN and plot its discriminator loss."""

import random

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
    graphics = ml.ml_core.graphics
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

    steps_x = list(range(1, steps + 1))
    graph = graphics.Graph(900, 600, graphics.GraphType.LINE)
    graph.set_title("CoolBox GAN training losses")
    graph.set_x_label("Training step")
    graph.set_y_label("Binary cross-entropy loss")
    graph.add_series(
        graphics.DataSeries(
            "Discriminator loss", steps_x, discriminator_losses, graphics.BLUE
        )
    )
    graph.add_series(
        graphics.DataSeries("Generator loss", steps_x, generator_losses, graphics.RED)
    )
    canvas = graph.render()
    if not canvas.save_png("gan_losses.png"):
        raise RuntimeError("CoolBox failed to save gan_losses.png")
    print("Saved loss plot to gan_losses.png")


if __name__ == "__main__":
    main()
