#include <unistd.h>

int main(void)
{
	Py_Initialize();
	Py_Finalize();
	return 0;
} 
