import chess_env_rl as chess_env
import numpy as np

env = chess_env.ChessEnv()
state = env.reset()
state = np.array(state)
assert state.shape == (119, 64)