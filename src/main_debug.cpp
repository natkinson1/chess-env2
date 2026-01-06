#include <iostream>
#include "board_debug.h"

int main() {

    Board env;

    auto [state, reward, terminal] = env.reset();

    std::vector<std::vector<int>> actions = env.get_legal_moves();
    return 0;
}