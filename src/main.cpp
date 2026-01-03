#include <iostream>
#include "board.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

PYBIND11_MODULE(chess_env, m, py::mod_gil_not_used()) {
    py::class_<Board>(m, "ChessEnv")
        .def(py::init<>())
        .def("reset_numpy", [](Board &self) {
            return py::array(self.reset());
        })
        .def("reset", &Board::reset)
        .def("get_actions", &Board::get_legal_moves);
}