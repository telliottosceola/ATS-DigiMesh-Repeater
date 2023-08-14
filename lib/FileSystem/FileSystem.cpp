#include <FileSystem.h>

bool FileSystem::readFile(fs::FS &fs, const char * path, char* buffer, int len){
  char internalBuffer[len];
  memset(internalBuffer, 0, len);
  // Serial.printf("Reading file: %s\r\n", path);
  delay(300);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    #ifdef DEBUG
    Serial.println("- failed to open file for reading(readFile)");
    Serial.println(path);
    #endif
    return false;
  }
  char loadedString[file.available()];
  for(int i = 0; i < len; i++){
    internalBuffer[i] = file.read();
  }
  memcpy(buffer, internalBuffer, len);
  return true;
}

int FileSystem::getFileSize(fs::FS &fs, const char * path){
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    #ifdef DEBUG
    Serial.println("- failed to open file for reading(getFileSize)");
    Serial.println(path);
    #endif
    return 0;
  }else{
    return file.available();
  }
}

void FileSystem::parseBytes(const char* str, char sep, uint8_t* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                            // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}
