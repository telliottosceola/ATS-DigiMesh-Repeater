#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <FS.h>
class FileSystem{
public:
  bool readFile(fs::FS &fs, const char * path, char* buffer, int len);
  int getFileSize(fs::FS &fs, const char * path);
  void parseBytes(const char* str, char sep, uint8_t* bytes, int maxBytes, int base);
private:

};
#endif
