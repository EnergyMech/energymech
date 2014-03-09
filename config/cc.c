#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char **argv)
{
#if (__GNUC__ >= 3) && (__GNUC_MINOR__ >= 3)
  write(1,"gnucc33\n",9);
  exit(0);
#endif
#if (__GNUC__ >= 2) && (__GNUC_MINOR__ >= 95)
  write(1,"gnucc95\n",8);
  exit(0);
#endif
#ifdef __GNUC__
  write(1,"gnucc\n",6);
#else
  write(1,"yes\n",4);
#endif
  return(0);
}
