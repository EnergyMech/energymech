#include <stdio.h>
#include <unistd.h>

#if defined(__GNUC__) && defined(__ELF__)
#define __page(x)               __attribute__ (( __section__ (x) ))
#else
#define __page(x)               /* nothing */
#endif

#define	S(x)	x,sizeof(x)

__page(".text.e")
int function1(int a)
{
	return a + 1;
}

__page(".text.c")
int function2(int a)
{
	return a + 2;
}

__page(".text.d")
int main(int argc, char **argv)
{
	if (((void*)main < (void*)function1) && ((void*)function1 < (void*)function2))
		write(1,S("yes\n"));
	else
		write(1,S("no\n"));
	return(0);
}
