cmake_minimum_required(VERSION 3.5...3.18)
project(chess_env)
add_subdirectory(pybind11)
pybind11_add_module(chess_env bitboard.cpp)