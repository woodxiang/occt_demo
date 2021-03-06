//=============================================================================
//
// SHICHUANG CONFIDENTIAL
// __________________
//
//  [2016] - [2021] SHICHUANG Co., Ltd.
//  All Rights Reserved.
//
//=============================================================================

#include "stl_file.h"

#include <string.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <list>
#include <regex>
#include <tuple>

using namespace std;

bool ExtractPoint3D(string& line, regex& exp, Point3D<float>& newPoint);

StlFile::StlFile() {}

StlFile::~StlFile() {}

bool StlFile::LoadFromStream(std::istream& is) {
  if (!LoadBinaryFormatStream(is)) {
    is.seekg(0);
    if (!LoadAsciiFormatStream(is)) {
      return false;
    }
  }

  return true;
}

void StlFile::SaveAsBinary(ostream& os, string header) {
  char buf[80];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, header.c_str(), min(header.length() + 1, sizeof(buf)));

  os.seekp(0);
  os.write(buf, 80);
  unsigned int nFacet = (int)m_vecFacets.size();
  os.write((char*)&nFacet, 4);

  os.write((char*)m_vecFacets.data(), nFacet * sizeof(Triangle3D<float>));
}

void StlFile::SaveAsAscii(ostream& os, string header) {
  if (header.length() > 80) header = header.substr(0, 80);
  os << header << endl;

  for (auto& facet : m_vecFacets) {
    os << "    facet normal " << facet.Normal.Coords[0] << " "
       << facet.Normal.Coords[1] << " " << facet.Normal.Coords[2] << endl;
    os << "        outer loop" << endl;

    for (int i = 0; i < 3; i++) {
      os << "            vertex " << facet.Vertexes[i].Coords[0] << " "
         << facet.Vertexes[i].Coords[0] << " " << facet.Vertexes[i].Coords[0]
         << endl;
    }
    os << "        endloop" << endl;
    os << "    endfacet" << endl;
  }
}

string StlFile::get_Header() { return string(m_strHeader); }

bool StlFile::LoadBinaryFormatStream(istream& is) {
  char buf[80];
  is.read(buf, sizeof(buf));

  if (is.fail()) {
    return false;
  }

  m_strHeader = buf;

  is.seekg(0, ios_base::end);
  auto nLeft = is.tellg() -
               static_cast<typename std::basic_istream<char>::pos_type>(80 + 4);
  is.seekg(80, ios::beg);

  // Read and check facet count
  //
  int facetCount;
  is.read((char*)&facetCount, 4);
  if (is.fail()) {
    return false;
  }
  auto sizeToRead = facetCount * sizeof(Triangle3D<float>);

  if (facetCount < 0 || nLeft != sizeToRead) {
    return false;
  }

  m_vecFacets.resize(facetCount);

  is.read((char*)m_vecFacets.data(), sizeToRead);

  if (is.fail()) {
    return false;
  }
  return true;
}

bool StlFile::LoadAsciiFormatStream(istream& is) {
  list<Triangle3D<float>> facets;
  char buf[7][64];

  is.getline(buf[0], sizeof(buf[0]));

  m_strHeader = buf[0];

  regex exp(R"([+|-]?\d+(\.\d+)?(e[+|-]?\d+)?)");

  while (true) {
    bool allReady = true;
    for (size_t i = 0; i < 7; i++) {
      is.getline(buf[i], sizeof(buf[i]));
      if (is.eof()) {
        allReady = false;
      }
    }

    if (!allReady) break;

    Triangle3D<float> newFacet;
    string str = buf[0];

    if (!ExtractPoint3D(str, exp, newFacet.Normal)) {
      break;
    }

    str = buf[2];
    if (!ExtractPoint3D(str, exp, newFacet.Vertexes[0])) {
      break;
    }
    str = buf[3];
    if (!ExtractPoint3D(str, exp, newFacet.Vertexes[1])) {
      break;
    }

    str = buf[4];
    if (!ExtractPoint3D(str, exp, newFacet.Vertexes[2])) {
      break;
    }

    facets.push_back(newFacet);
  }

  m_vecFacets.resize(facets.size());

  size_t i = 0;
  for (auto& var : facets) {
    m_vecFacets[i++] = var;
  }

  return true;
}

bool ExtractPoint3D(string& line, regex& exp, Point3D<float>& newPoint) {
  std::smatch match;
  auto beg = line.cbegin();
  auto end = line.cend();

  int i = 0;
  while (regex_search(beg, end, match, exp)) {
    if (!match[0].matched) {
      return false;
    }
    float newValue = (float)atof(match[0].str().data());
    newPoint.Coords[i++] = newValue;
    beg = match.suffix().first;
  }

  return true;
}

void StlFile::ToIndexedData(std::vector<float>& vertexes,
                            std::vector<unsigned int>& indexes) {
  std::vector<size_t> sequence(m_vecFacets.size() * 3);
  for (size_t i = 0; i < sequence.size(); i++) {
    sequence[i] = i;
  }

  auto& faces = m_vecFacets;

  // sort by vertex position
  std::sort(sequence.begin(), sequence.end(),
            [faces](const size_t a, const size_t b) {
              auto& aPos = faces[a / 3].Vertexes[a % 3].Coords;
              auto& bPos = faces[b / 3].Vertexes[b % 3].Coords;

              if (aPos[0] == bPos[0]) {
                if (aPos[1] == bPos[1]) {
                  if (aPos[2] == bPos[2]) {
                    return false;
                  }
                  return aPos[2] - bPos[2] < 0;
                }
                return aPos[1] - bPos[1] < 0;
              }
              return aPos[0] - bPos[0] < 0;
            });

  // look for the overlapped vertexes
  std::vector<size_t> dupMap(sequence.size());
  size_t mapTo = 0;
  size_t vertexNumber = 0;
  float p[] = {std::numeric_limits<float>::min(),
               std::numeric_limits<float>::min(),
               std::numeric_limits<float>::min()};
  auto prevPos = p;
  for (size_t i = 0; i < sequence.size(); i++) {
    auto index = sequence[i];
    auto& curPos = m_vecFacets[index / 3].Vertexes[index % 3].Coords;
    if (curPos[0] != prevPos[0] || curPos[1] != prevPos[1] ||
        curPos[2] != prevPos[2]) {
      mapTo = index;
      vertexNumber++;
      prevPos = curPos;
    }
    dupMap[index] = mapTo;
  }

  sequence.clear();

  vertexes.reserve(vertexNumber * 3);

  indexes.resize(dupMap.size());
  // set the first bit to 1 and other bits change to the index in vertexes;
  // when the vertex was put into vertexes;
  for (size_t i = 0; i < dupMap.size(); i++) {
    if (dupMap[i] & 0x80000000) {
      indexes[i] = dupMap[i] & 0x7fffffff;
      continue;
    }
    auto index = dupMap[dupMap[i]];
    if (index & 0x80000000) {
      indexes[i] = index & 0x7fffffff;
    } else {
      size_t vertexesNumber = vertexes.size() / 3;
      auto pos = m_vecFacets[index / 3].Vertexes[index % 3].Coords;
      vertexes.push_back(pos[0]);
      vertexes.push_back(pos[1]);
      vertexes.push_back(pos[2]);
      dupMap[dupMap[i]] = 0x80000000 | vertexesNumber;
      indexes[i] = static_cast<unsigned int>(vertexesNumber);
    }
  }
}