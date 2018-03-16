#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char **argv)
{
  char x[4];
  *((int*)&x) = 1;
  printf("%i\n",x[0]);
  return(0);
}

