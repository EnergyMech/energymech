#include <netinet/in.h>
#include <arpa/inet.h>
int main(int argc, char **argv)   
{
  struct in_addr ia;
  ia.s_addr = inet_addr("0.0.0.0");
  return(0);
}

