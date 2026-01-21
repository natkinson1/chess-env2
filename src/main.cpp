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
    .def_readwrite("state_pos", &State::state_pos)
    .def_readwrite("total_move_count", &State::total_move_count)
    .def_readwrite("no_progress_count", &State::no_progress_count)
    .def_readwrite("repetition_count", &State::repetition_count)
    .def_readwrite("n_repititions", &State::n_repititions)
    .def_readwrite("prev_n_repititions", &State::prev_n_repititions)
    .def_property_readonly("bitboards", [](const State &s) {
        return py::array_t<U64>(12, s.bitboards);
    })
    .def_property_readonly("occupancies", [](const State &s) {
        return py::array_t<U64>(3, s.occupancies);
    })
    .def("__repr__", [](const State &s) {
            std::ostringstream oss;
            oss << "State(\n"
                << "  side=" << s.side << ",\n"
                << "  enpassant=" << s.enpassant << ",\n"
                << "  castle=" << s.castle << ",\n"
                << "  state_pos=" << s.state_pos << ",\n"
                << "  total_move_count=" << s.total_move_count << ",\n"
                << "  no_progress_count=" << s.no_progress_count << ",\n"
                << "  n_repititions=" << s.n_repititions << ",\n"
                << "  state_history size=" << s.state_history.size() << "\n"
                << ")";
            return oss.str();
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