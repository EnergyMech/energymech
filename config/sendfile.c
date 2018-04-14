#include <sys/sendfile.h>

void test(int to, int from, off_t offset, size_t ct)
{
	sendfile(to, from, &offset, ct);
}
