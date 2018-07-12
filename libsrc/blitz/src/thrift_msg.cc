#include <protocol/TBinaryProtocol.h>
#include "blitz/thrift_msg.h"
#include "blitz/framework.h"

namespace blitz {
char ConvertAsciiPrinter(uint8_t c){
  if(c >= '0' && c <= '9'){
    return (char)c;
  }
  if(c >= 'a' && c <='z'){
    return (char)c;
  }
  if(c >= 'A' && c <='Z'){
    return (char) c;
  }
  return '.';
}
void Dump(const uint8_t* data,uint32_t length){
  using namespace std;
  uint32_t rows = length / 16;
  for(uint32_t i = 0; i < rows; ++ i){
    for(uint32_t j = 0; j < 16; ++ j){
      printf("%02x ",data[i*16+j]);
    }
    fflush(0);
    printf("        ");
    for(uint32_t j = 0; j < 16; ++ j){
      printf("%c",ConvertAsciiPrinter(data[i*16+j]));
    }
    printf("\n");
  }
  uint32_t remains = length - rows * 16;
  for(uint32_t j = 0; j < remains ; ++ j){
    printf("%02x ",data[rows*16+j]);
  }
   printf("        ");
  for(uint32_t j = 0; j < remains ; ++ j){
    printf("%c",ConvertAsciiPrinter(data[rows*16+j]));
  }
  printf("\n");
  
}

} // namespace blitz.
