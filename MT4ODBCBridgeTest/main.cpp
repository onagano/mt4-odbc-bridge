#include <stdio.h>


void __stdcall MOB_create(const char *name);
void __stdcall MOB_open(const char *name, const char *dns, const char *username, const char *password);
void __stdcall MOB_close(const char *name);
void __stdcall MOB_execute(const char *name, const char *sql);


int main(int argc, char *argv[])
{
	int ret = 0;
	printf("MT4-ODBC Bridge Test\n");

	const char *name = "hogeid";
	MOB_create(name);
	MOB_open(name, "testmysql", "onagano", "password");
	MOB_execute(name, "insert into pet (name, owner) values ('aaa', 'hogefoobar')");
	MOB_close(name);

	getchar();
	return ret;
}
