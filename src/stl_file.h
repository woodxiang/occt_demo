#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <iostream>

template <typename T>
struct Point3D {
  T Coords[3] = {};

  Point3D() {}

  Point3D(T x, T y, T z) {
    Coords[0] = x;
    Coords[1] = y;
    Coords[2] = z;
  }

 public:
  T operator[](size_t index) { return Coords[index]; }

  inline T dot(Point3D<T> &pt) {
    return Coords[0] * pt[0] + Coords[1] * pt[1] + Coords[2] * pt[2];
  }

  inline Point3D<T> Cross(Point3D<T> &pt) {
    return Point3D(Coords[1] * pt[2] - pt[1] * Coords[2],
                   Coords[2] * pt[0] - pt[2] * Coords[0],
                   Coords[0] * pt[1] - pt[0] * Coords[1]);
  }

  inline Point3D<T> operator-(Point3D<T> &p) {
    return Point3D<T>(Coords[0] - p[0], Coords[1] - p[1], Coords[2] - p[2]);
  }

  inline Point3D<T> operator+(Point3D<T> &p) {
    return Point3D<T>(Coords[0] + p[0], Coords[1] + p[1], Coords[2] + p[2]);
  }

  inline Point3D<T> operator/(T div) {
    return Point3D<T>(Coords[0] / div, Coords[1] / div, Coords[2] / div);
  }
};

#pragma pack(push)
#pragma pack(2)
template <typename T>
struct Triangle3D {
  Point3D<T> Normal;
  Point3D<T> Vertexes[3];
  short dummy = 0;

 public:
  Point3D<T> &operator[](size_t index) { return Vertexes[index]; }
};
#pragma pack(pop)

class StlFile {
 public:
  StlFile();
  virtual ~StlFile();

 public:
  bool LoadFromStream(std::istream &is);

  void SaveAsBinary(std::ostream& os, std::string header);
  void SaveAsAscii(std::ostream& os, std::string header);

  size_t get_TriangleCount() { return m_vecFacets.size(); }
  Triangle3D<float> &get_Triangle(size_t index) {
    assert(index < m_vecFacets.size());
    return m_vecFacets[index];
  }

  std::string get_Header();

 private:
  bool LoadBinaryFormatStream(std::istream &is);
  bool LoadAsciiFormatStream(std::istream &is);

 private:
  std::string m_strHeader;
  std::vector<Triangle3D<float>> m_vecFacets;
};