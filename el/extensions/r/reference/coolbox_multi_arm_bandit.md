# Run a multi-arm bandit simulation

Run a multi-arm bandit simulation

## Usage

``` r
coolbox_multi_arm_bandit(
  true_probs,
  strategy = c("epsilon_greedy", "ucb", "thompson_sampling", "decaying_epsilon"),
  steps = 1000L,
  epsilon = 0.1,
  c = 2,
  decay_rate = 0.99,
  seed = 0L
)
```
