# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages

__version__ = "0.0.2"

ext_modules = [
    Pybind11Extension(
        "chess_env_rl",
        [
            "src/main.cpp", 
            "src/board.cpp",
            "src/definitions.cpp",
        ],
        define_macros=[("VERSION_INFO", __version__)],
        depends=[
            "src/board.h", 
            "src/definitions.h"
        ]
    ),
]

setup(
    name="chess_env_rl",
    version=__version__,
    author="Nikhil Atkinson",
    author_email="",
    url="https://github.com/natkinson1/chess-env2",
    description="Bitboard chess engine python library for RL.",
    long_description="""
    Python chess environment written in c++ to simulate chess games. Implementation uses bitboards,
    magic numbers, and precomputed attack tables for fast simulations of games.
    The board state and action representation is of shape (119, 64), (73, 64) as described in the
    AlphaZero implementation.""",
    license="MIT",
    license_files=["LICEN[CS]E*"],
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.9",
)