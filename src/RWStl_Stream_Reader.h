#pragma once

#include <Message_ProgressScope.hxx>
#include <RWStl_Reader.hxx>

class RWStl_Stream_Reader : public RWStl_Reader {
public:
  Standard_EXPORT Standard_Boolean
  Read(Standard_IStream &inputStream,
       const Message_ProgressRange &readProgress = Message_ProgressRange());
};