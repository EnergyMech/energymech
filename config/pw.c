#include <stdio.h>
#include <string.h>
#include <unistd.h>
char * CRYPT_FUNC (const char *, const char *);
int main(void)
{
	char	*enc;
	int	md5,des;

	des = md5 = 0;
	enc = CRYPT_FUNC ("password","$1$XX");
	if (enc && !strcmp(enc,"$1$XX$HxaXRcnpWZWDaXxMy1Rfn0"))
		md5 = 1;
	enc = CRYPT_FUNC ("password","XX");
	if (enc && !strcmp(enc,"XXq2wKiyI43A2"))
		des = 1;

	if (md5 && des)
		write(1,"DESaMD5\n",8);
	else
	if (des)
		write(1,"DES\n",4);
	else
	if (md5)
		write(1,"MD5\n",4);
	return(0);
}
