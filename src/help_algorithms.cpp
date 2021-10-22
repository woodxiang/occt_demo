#include "help_algorithms.h"

#include <algorithm>
#include <cassert>

/*
 * indexes is the index of vertexes
 * edges and triangles are the outputs.
 * edges indicate by the vertex indexes
 * triangles indicate by the edges indexes
 */
bool findEdges(
    std::vector<unsigned int>& indexes,
    std::vector<std::tuple<unsigned int, unsigned int>>& edges,
    std::vector<std::tuple<unsigned int, unsigned int, unsigned int>>&
        triangles) {
  // too large to process
  if (indexes.size() > 0x07fffffff) {
    return false;
  }

  std::vector<unsigned long long> edgesTmp;

  // get all the edges
  for (size_t i = 0; i < indexes.size() / 3; i++) {
    for (size_t j = 0; j < 3; j++) {
      auto p1 = indexes[i * 3 + j];
      auto p2 = indexes[i * 3 + (j + 1) % 3];
      assert(p1 != p2);
      if (p1 > p2) {
        edgesTmp.push_back(((unsigned long long)p2) << 32 | p1);
      } else {
        edgesTmp.push_back(((unsigned long long)p1) << 32 | p2);
      }
    }
  }

  // remove dupplicated edges
  std::vector<unsigned int> sequence(indexes.size());
  for (unsigned int i = 0; i < sequence.size(); i++) {
    sequence[i] = i;
  }

  std::reverse(sequence.begin(), sequence.end());

  std::sort(sequence.begin(), sequence.end(),
            [edgesTmp](unsigned int a, unsigned int b) {
              return edgesTmp[a] < edgesTmp[b];
            });

  std::vector<unsigned int> dupMap(sequence.size());
  unsigned long long prevPos = std::numeric_limits<unsigned long long>::max();
  unsigned int mapTo = 0;
  unsigned int targetSize = 0;
  for (size_t i = 0; i < sequence.size(); i++) {
    auto index = sequence[i];
    auto& curPos = edgesTmp[index];
    if (curPos != prevPos) {
      mapTo = index;
      targetSize++;
      prevPos = curPos;
    }
    dupMap[index] = mapTo;
  }

  sequence.clear();
  edges.reserve(targetSize);

  // set the first bit to 1 and other bits change to the index in vertexes;
  // when the vertex was put into vertexes;
  for (size_t i = 0; i < dupMap.size(); i++) {
    if (dupMap[i] & 0x80000000) {
      continue;
    }

    auto index = dupMap[dupMap[i]];
    if ((index < 0x80000000)) {
      auto number = edges.size();
      edges.push_back(std::make_tuple(
          static_cast<unsigned int>(edgesTmp[index] >> 32),
          static_cast<unsigned int>(edgesTmp[index] & 0x7fffffff)));
      dupMap[dupMap[i]] = static_cast<unsigned int>(0x80000000 | number);
    }
  }

  for (size_t i = 0; i < dupMap.size(); i++) {
    if ((dupMap[i] & 0x80000000) == 0) {
      dupMap[i] = dupMap[dupMap[i]] & 0x7fffffff;
    } else {
      dupMap[i] = dupMap[i] & 0x7fffffff;
    }
  }

  triangles.reserve(indexes.size() / 3);
  for (size_t i = 0; i < indexes.size() / 3; i++) {
    triangles.push_back(
        std::make_tuple(dupMap[i * 3], dupMap[i * 3 + 1], dupMap[i * 3 + 2]));
  }

  return true;
}