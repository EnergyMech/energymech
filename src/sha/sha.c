#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "sha1.h"

int main(int argc, char **argv)
{
    SHA1_CTX sha;
 uint8_t results[20];
 char *buf;
 int n;

    buf = "abc";
 n = strlen(buf);
 SHA1Init(&sha);
 SHA1Update(&sha, (uint8_t *)buf, n);
 SHA1Final(results, &sha);

    /* Print the digest as one long hex value */
printf("a9993e364706816aba3e25717850c26c9cd0d89d <-- expected result\n");
for (n = 0; n < 20; n++)
        printf("%02x", results[n]);

    putchar('\n');

	return(0);
}
