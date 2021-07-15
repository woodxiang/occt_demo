#pragma once

#include <TopoDS_Shape.hxx>

class ModelFactory
{
private:
  static ModelFactory *instance_;

public:
  ModelFactory(/* args */);
  ~ModelFactory();

  static ModelFactory *GetInstance();

  TopoDS_Shape MakeBottle(const Standard_Real myWidth, const Standard_Real myHeight,
                          const Standard_Real myThickness);
};
