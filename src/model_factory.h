#pragma once

class TopoDS_Shape;
class AIS_InteractiveObject;
class ModelFactory {
private:
  static ModelFactory *instance_;

public:
  ModelFactory(/* args */);
  ~ModelFactory();

  static ModelFactory *GetInstance();

  TopoDS_Shape MakeBottle(const Standard_Real myWidth,
                          const Standard_Real myHeight,
                          const Standard_Real myThickness);

  Handle(AIS_InteractiveObject) LoadFromStl(std::istream &is);
};
