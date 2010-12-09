#include <stdio.h>


int		__stdcall MOB_create();
void	__stdcall MOB_open(int conId, const char *dns, const char *username, const char *password);
void	__stdcall MOB_close(int conId);
void	__stdcall MOB_execute(int conId, const char *sql);
int		__stdcall MOB_getLastErrorNo(int conId);
char*	__stdcall MOB_getLastErrorMesg(int conId);


void checkError(int conId) {
	int ret = MOB_getLastErrorNo(conId);
	if (ret != 0) {
		printf("ErrNo: %d, ErrMesg:\n%s\n", ret, MOB_getLastErrorMesg(conId));
	}
}


int main(int argc, char *argv[])
{
	int ret = 0;
	printf("MT4-ODBC Bridge Test\n");

	int conId = MOB_create();

	MOB_open(conId, "testhoge", "onagano", "password");
	checkError(conId);

	MOB_open(conId, "testmysql", "onagano", "password");
	checkError(conId);

	MOB_execute(conId, "insert into pet (name, owner) values ('aaa', 'hogefoobar')");
	checkError(conId);

	MOB_execute(conId, "insert into pet2 (name, owner) values ('aaa', 'hogefoobar')");
	checkError(conId);

	MOB_close(conId);
	checkError(conId);

	getchar();
	return ret;
}
