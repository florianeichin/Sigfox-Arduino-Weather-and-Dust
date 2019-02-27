#include<stdio.h>
char *toHex( __uint128_t c )
{
  const static char *hex="0123456789ABCDEF";
  static char toHexBuf[3];
  toHexBuf[0]= hex[c>>4];
  toHexBuf[1]= hex[c & 0x0f];
  toHexBuf[2]=0;
  return toHexBuf;
}

int main(){
   int value = 4095;
      char *a ;
      sprintf (a,"%04x", value);
      printf(a);
}