#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
       The glibc2 version of this function supports additional encryption algorithms.

       If salt is a character string starting with the characters "$id$" followed by a string terminated by "$":

              $id$salt$encrypted

       then instead of using the DES machine, id identifies the encryption method used and this then determines how the rest of the password string  is  inter-
       preted.  The following values of id are supported:

              ID  | Method
              ---------------------------------------------------------
              1   | MD5
              2a  | Blowfish (not in mainline glibc; added in some
                  | Linux distributions)
              5   | SHA-256 (since glibc 2.7)
              6   | SHA-512 (since glibc 2.7)

*/

char * CRYPT_FUNC (const char *, const char *);

int main(void)
{
	char	*enc;
	int	md5,sha;

	md5 = sha = 0;

	enc = CRYPT_FUNC ("abc","$6$");
	if (enc && !strcmp(enc,"$6$$K7EQl9xnonG1x970hnNPqFQKlunsvbFHwzYLnbANzfHjxbphBMjLilW7SKO5EQOidBzcHseqkDOBCSPD8a3CR0"))
		sha = 1;

	enc = CRYPT_FUNC ("password","$1$XX");
	if (enc && !strcmp(enc,"$1$XX$HxaXRcnpWZWDaXxMy1Rfn0"))
		md5 = 1;

	if (sha)
		write(1,"SHA",3);
	if (md5)
		write(1,"MD5",3);
	write(1,"\n",1);
	return(0);
}
