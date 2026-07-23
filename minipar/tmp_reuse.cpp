#include <winsock2.h>
#include <stdio.h>
int main() {
#ifdef SO_REUSEPORT
  printf("SO_REUSEPORT=%d\n", SO_REUSEPORT);
#else
  printf("SO_REUSEPORT undefined\n");
#endif
#ifdef SO_REUSEADDR
  printf("SO_REUSEADDR=%d\n", SO_REUSEADDR);
#endif
  return 0;
}
