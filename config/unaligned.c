#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int buserror(char *data)
{
  int *x;
  x = (int*)(data+1);
  return(*x);
}
int main(int argc, char **argv)
{
  char busdata[6];
  int x;
  strcpy(busdata,"Xyes\n");
  x = buserror(busdata);
  write(1,&x,4);
  return(0);
}
