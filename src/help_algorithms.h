#pragma once

#include <tuple>
#include <vector>

bool findEdges(
    std::vector<unsigned int>& indexes,
    std::vector<std::tuple<unsigned int, unsigned int>>& edges,
    std::vector<std::tuple<unsigned int, unsigned int, unsigned int>>&
        triangles);