#pragma once

#include <Message_ProgressScope.hxx>
#include <NCollection_Vector.hxx>
#include <Poly_Triangulation.hxx>
#include <RWStl_Reader.hxx>

class RWStl_Stream_Reader : public RWStl_Reader {
public:
  DEFINE_STANDARD_RTTIEXT(RWStl_Stream_Reader, RWStl_Reader)
  Standard_EXPORT Standard_Boolean
  Read(Standard_IStream &inputStream,
       const Message_ProgressRange &readProgress = Message_ProgressRange());

public:
  //! Add new node
  virtual Standard_Integer AddNode(const gp_XYZ &thePnt) Standard_OVERRIDE;

  //! Add new triangle
  virtual void AddTriangle(Standard_Integer theNode1, Standard_Integer theNode2,
                           Standard_Integer theNode3) Standard_OVERRIDE;

  //! Creates Poly_Triangulation from collected data
  Handle(Poly_Triangulation) GetTriangulation();

private:
  NCollection_Vector<gp_XYZ> myNodes;
  NCollection_Vector<Poly_Triangle> myTriangles;
};