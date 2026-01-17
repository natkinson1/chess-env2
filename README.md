![PyPI - Version](https://img.shields.io/pypi/v/chess-env-rl)

# chess_env

Python chess environment written in c++ to simulate chess games. Implementation uses bitboards, magic numbers, and precomputed attack tables for fast simulations of games.

# Install

To install, run:

```bash
pip install chess-env-rl
```


# Implementation

The board state and action representation is of shape (119, 64), (73, 64) as described in the [AlphaZero representation](https://arxiv.org/pdf/1712.01815). The board is orientated to the perspective of the current player.

An action is chosen by selecting the index value of the flattened (73, 64) action space. An example is shown below:

```python
import chess_env_rl as chess_env
import numpy as np

env = chess_env.ChessEnv()
state, reward, terminal = env.reset()

actions = env.get_actions()
actions = np.array(actions).reshape(-1)
indices = np.argwhere(actions == 1).flatten()
random_action = np.random.choice(indices)
state, reward, terminal = env.step(random_action)
env.render()
```

```
 8 |  ♜  ♞  ♝  ♛  ♚  ♝  ♞  ♜

  7 |  ♟  ♟  ♟  ♟  ♟  ♟  ♟  ♟

  6 |   .   .   .   .   .   .   .   .

  5 |   .   .   .   .   .   .   .   .

  4 |   .   .   .   .   .   .   .   .

  3 |   .   .   .   .   .   .   .  ♙

  2 |  ♙  ♙  ♙  ♙  ♙  ♙  ♙   .

  1 |  ♖  ♘  ♗  ♕  ♔  ♗  ♘  ♖

```

# Benchmarks

```python
import chess_env

env = chess_env.ChessEnv()
state, reward, terminal = env.reset()

%%timeit
actions = env.get_actions()
# 15.4 μs ± 164 ns per loop (mean ± std. dev. of 7 runs, 100,000 loops each)

```

# Credits

The implentation of the bitboards engine was made possible with the help of [bbc chess](https://github.com/maksimKorzh/bbc) and the fantastic YouTube tutorial series.

[pybind11](https://pybind11.readthedocs.io/en/stable/index.html) to convert c++ into python package and also [this example](https://github.com/pybind/python_example/blob/master/setup.py) for help setting up pypi package.

