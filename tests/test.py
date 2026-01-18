import chess_env_rl as chess_env

env = chess_env.ChessEnv()
player, state, reward, terminal = env.reset()
actions = env.get_actions()
