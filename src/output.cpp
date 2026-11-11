#include <string>
#include <vector>
#include <cstdint> 
#include <cstddef>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <algorithm>
#include <cctype>
#include "output.hpp"

std::string output(const std::vector<uint8_t>& data) {
    std::ostringstream out;
    for (size_t line = 0; line < data.size(); line += 16) {
        size_t end = std::min(line + 16, data.size());

        for (size_t i = line; i < end; i++){
            out << std::bitset<8>(data[i]) << " ";
        }

        for (size_t i = line; i < end; i++){
            out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
        }

        for (size_t i = end; i < line + 16; i++){
            out << std::string(9, ' ') << "   ";
        }

        out << " |";

        for (size_t i = line; i < end; i++){
            out << (std::isprint(data[i]) ? static_cast<char>(data[i]) : '.');
        }

        out << "|\n";
    }

    return out.str();
}