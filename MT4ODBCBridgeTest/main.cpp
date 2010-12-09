#include <stdio.h>


int	 __stdcall MOB_create();
void __stdcall MOB_open(int conId, const char *dns, const char *username, const char *password);
void __stdcall MOB_close(int conId);
void __stdcall MOB_execute(int conId, const char *sql);


int main(int argc, char *argv[])
{
	int ret = 0;
	printf("MT4-ODBC Bridge Test\n");

	int conId = MOB_create();
	MOB_open(conId, "testmysql", "onagano", "password");
	MOB_execute(conId, "insert into pet (name, owner) values ('aaa', 'hogefoobar')");
	MOB_close(conId);

	getchar();
	return ret;
}
