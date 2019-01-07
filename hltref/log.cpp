#include "log.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>


int ___bot_id;
void hlt::log::open(int bot_id) {
    ___bot_id = bot_id;
}

void hlt::log::log(const std::string& message) {
  static std::ofstream file("bot-" + std::to_string(___bot_id) + ".log", std::ios::trunc | std::ios::out);
  file << message << std::endl;
}
