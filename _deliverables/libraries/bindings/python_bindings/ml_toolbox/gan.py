"""A small trainable GAN built from CoolBox deep-learning layers."""


class GAN:
    """Train a fully connected GAN using CoolBox tensors and layers.

    ``train_batch`` expects a two-dimensional tensor with samples normalized to
    ``[-1, 1]`` and returns ``(discriminator_loss, generator_loss)``.
    """

    def __init__(
        self,
        latent_dim,
        sample_dim,
        hidden_dims=(64, 32),
        learning_rate=0.001,
    ):
        if latent_dim <= 0 or sample_dim <= 0:
            raise ValueError("latent_dim and sample_dim must be positive")
        if not hidden_dims or any(width <= 0 for width in hidden_dims):
            raise ValueError("hidden_dims must contain positive layer widths")
        if learning_rate <= 0:
            raise ValueError("learning_rate must be positive")

        from . import ml_core

        self._dl = ml_core.deep_learning
        self.latent_dim = latent_dim
        self.sample_dim = sample_dim
        self.learning_rate = learning_rate
        self._loss = self._dl.BCELoss()

        self._generator = []
        previous_width = latent_dim
        for width in hidden_dims:
            self._generator.extend(
                [self._dl.DenseLayer(previous_width, width), self._dl.ReLULayer()]
            )
            previous_width = width
        self._generator.extend(
            [self._dl.DenseLayer(previous_width, sample_dim), self._dl.TanhLayer()]
        )

        self._discriminator = []
        previous_width = sample_dim
        for width in hidden_dims:
            self._discriminator.extend(
                [self._dl.DenseLayer(previous_width, width), self._dl.ReLULayer()]
            )
            previous_width = width
        self._discriminator.extend(
            [self._dl.DenseLayer(previous_width, 1), self._dl.SigmoidLayer()]
        )

    @staticmethod
    def _forward(layers, tensor):
        for layer in layers:
            tensor = layer.forward(tensor)
        return tensor

    @staticmethod
    def _backward(layers, gradient):
        for layer in reversed(layers):
            gradient = layer.backward(gradient)
        return gradient

    def _update(self, layers):
        for layer in layers:
            if layer.has_parameters():
                layer.update_parameters(self.learning_rate)

    def sample(self, count):
        """Generate ``count`` samples and return a CoolBox Tensor."""
        if count <= 0:
            raise ValueError("count must be positive")
        noise = self._dl.Tensor([count, self.latent_dim])
        noise.randomize(-1.0, 1.0)
        return self._forward(self._generator, noise)

    def train_batch(self, real_samples):
        """Train on one real-data batch and return discriminator/generator loss."""
        shape = real_samples.shape()
        if len(shape) != 2 or shape[1] != self.sample_dim or shape[0] == 0:
            raise ValueError(
                f"real_samples must have shape [batch, {self.sample_dim}]"
            )

        batch_size = shape[0]
        noise = self._dl.Tensor([batch_size, self.latent_dim])
        noise.randomize(-1.0, 1.0)
        fake_samples = self._forward(self._generator, noise)

        combined_samples = self._dl.Tensor(
            [batch_size * 2, self.sample_dim],
            list(real_samples.data()) + list(fake_samples.data()),
        )
        labels = self._dl.Tensor(
            [batch_size * 2, 1],
            [1.0] * batch_size + [0.0] * batch_size,
        )

        discriminator_predictions = self._forward(
            self._discriminator, combined_samples
        )
        discriminator_loss = self._loss.compute(discriminator_predictions, labels)
        discriminator_gradient = self._loss.gradient(
            discriminator_predictions, labels
        )
        self._backward(self._discriminator, discriminator_gradient)
        self._update(self._discriminator)

        noise = self._dl.Tensor([batch_size, self.latent_dim])
        noise.randomize(-1.0, 1.0)
        fake_samples = self._forward(self._generator, noise)
        discriminator_predictions = self._forward(
            self._discriminator, fake_samples
        )
        generator_labels = self._dl.Tensor([batch_size, 1], [1.0] * batch_size)
        generator_loss = self._loss.compute(
            discriminator_predictions, generator_labels
        )
        discriminator_gradient = self._loss.gradient(
            discriminator_predictions, generator_labels
        )
        generator_gradient = self._backward(
            self._discriminator, discriminator_gradient
        )
        self._backward(self._generator, generator_gradient)
        self._update(self._generator)

        return discriminator_loss, generator_loss
