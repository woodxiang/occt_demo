#pragma once

#include <RWStl_Reader.hxx>

class RWStl_Stream_Reader : public RWStl_Stream_Reader {
 public:
  Standard_EXPORT Standard_Boolean
  Read(Standard_IStream& theStream, const Message_ProgressRange& theProgress);
};