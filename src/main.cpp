#include <iostream>
#include "board.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

PYBIND11_MODULE(chess_env_rl, m, py::mod_gil_not_used()) {
    py::class_<State>(m, "State")
    .def(py::init<>())
    .def_readonly("side", &State::side)
    .def_readonly("enpassant", &State::enpassant)
    .def_readonly("castle", &State::castle)
    .def_readwrite("state_history", &State::state_history)
    .def_property_readonly("bitboards", [](const State &s) {
        return py::array_t<U64>(12, s.bitboards);
    })
    .def_property_readonly("occupancies", [](const State &s) {
        return py::array_t<U64>(3, s.occupancies);
    });
    py::class_<Board>(m, "ChessEnv")
        .def(py::init<>())
        .def("reset", &Board::reset)
        .def("get_actions", &Board::get_legal_moves)
        .def("render", &Board::print_board)
        .def("parse_fen", &Board::parse_fen)
        .def("step", &Board::step)
        .def("save_state", &Board::save_state)
        .def("restore_state", &Board::restore_state)
        .def("hash_state", &Board::hash_game_state)
        .def("roll_out", &Board::roll_out);
}