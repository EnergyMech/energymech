#include <netinet/in.h>
#include <arpa/inet.h>
int main(int argc, char **argv)   
{
  struct in_addr ia;
  return(inet_aton("0.0.0.0",&ia));
}

