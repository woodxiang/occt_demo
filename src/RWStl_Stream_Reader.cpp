#include "RWStl_Stream_Reader.h"
namespace {

// Binary STL sizes
static const size_t THE_STL_HEADER_SIZE = 84;
static const size_t THE_STL_SIZEOF_FACET = 50;
static const size_t THE_STL_MIN_FILE_SIZE =
    THE_STL_HEADER_SIZE + THE_STL_SIZEOF_FACET;

// The length of buffer to read (in bytes)
static const size_t THE_BUFFER_SIZE = 1024;

} // namespace

Standard_EXPORT Standard_Boolean RWStl_Stream_Reader::Read(
    Standard_IStream &inputStream, const Message_ProgressRange &readProgress) {
  inputStream.seekg(0, std::ios_base::end);
  auto end = inputStream.tellg();
  inputStream.seekg(0, std::ios_base::beg);
  bool isAscii =
      (((size_t)end < THE_STL_MIN_FILE_SIZE) || IsAscii(inputStream, true));

  Standard_ReadLineBuffer aBuffer(THE_BUFFER_SIZE);

  // Note: here we are trying to handle rare but realistic case of
  // STL files which are composed of several STL data blocks
  // running translation in cycle.
  // For this reason use infinite (logarithmic) progress scale,
  // but in special mode so that the first cycle will take ~ 70% of it
  Message_ProgressScope aPS(readProgress, NULL, 1, true);
  while (inputStream.good()) {
    if (isAscii) {
      if (!ReadAscii(inputStream, aBuffer, end, aPS.Next(2))) {
        break;
      }
    } else {
      if (!ReadBinary(inputStream, aPS.Next(2))) {
        break;
      }
    }
    inputStream >> std::ws; // skip any white spaces
  }
  return inputStream.fail();
}