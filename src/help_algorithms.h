#pragma once

#include <tuple>
#include <vector>
#include <string>

bool file_edges(
    const std::vector<unsigned int> &indexes,
    std::vector<std::tuple<unsigned int, unsigned int>> &edges,
    std::vector<std::tuple<unsigned int, unsigned int, unsigned int>>
        &triangles);

bool test_file_extension(const std::string &targetString,
                         const std::string &subString);