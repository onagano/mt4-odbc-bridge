#include <stdio.h>
#include <tchar.h>

int mobtest(int argc, TCHAR *argv[]);
int maptest(int argc, TCHAR *argv[]);


int _tmain(int argc, TCHAR *argv[])
{
	int rc = 0;

	rc += mobtest(argc, argv);
	rc += maptest(argc, argv);

	printf("Press enter to exit: ");
	getchar();
	return rc;
}
