#include <stdio.h>
#include <time.h>


int		__stdcall MOB_create();
void	__stdcall MOB_open(int conId, const char *dns, const char *username, const char *password);
void	__stdcall MOB_close(int conId);
void	__stdcall MOB_execute(int conId, const char *sql);
int		__stdcall MOB_getLastErrorNo(int conId);
char*	__stdcall MOB_getLastErrorMesg(int conId);
int		__stdcall MOB_prepareStatement(int conId, const char *sql);
int		__stdcall MOB_executeWithTickData(int conId, int stmtId, int datetime, int millis, double *vals, int size);
int		__stdcall MOB_executeWithCandleData(int conId, int stmtId, int datetime, double *vals, int size);


void checkError(int conId) {
	int ret = MOB_getLastErrorNo(conId);
	if (ret != 0) {
		printf("ErrNo: %d, ErrMesg:\n%s\n", ret, MOB_getLastErrorMesg(conId));
	}
}


int main(int argc, char *argv[])
{
	int ret = 0;
	int conId = 0;
	int stmtId = 0;
	time_t t;

	printf("MT4-ODBC Bridge Test\n");

	conId = MOB_create();

	MOB_open(conId, "testhoge", "onagano", "password");
	checkError(conId);

	checkError(conId);

	MOB_open(conId, "testmysql", "onagano", "password");
	checkError(conId);

	MOB_execute(conId, "insert into pet (name, owner) values ('aaa', 'hogefoobar')");
	checkError(conId);

	MOB_execute(conId, "insert into pet2 (name, owner) values ('aaa', 'hogefoobar')");
	checkError(conId);

	MOB_execute(conId, "create table if not exists ticks ("
						"time datetime,"
						"msec int,"
						"open double,"
						"high double,"
						"low double,"
						"close double,"
						"vol double"
						")");
	checkError(conId);

	MOB_execute(conId, "insert into ticks values ('2001-01-01 13:00:01', 333, 110.32, 111.55, 90.90, 101.01, 64)");
	checkError(conId);

	stmtId = MOB_prepareStatement(conId, "insert into ticks values (?, ?, ?, ?, ?, ?, ?)");
	checkError(conId);

	time(&t);
	double vals1[5] = {111.32, 115.55, 70.90, 100.01, 44.24};
	ret = MOB_executeWithTickData(conId, stmtId, (int) t, 123, vals1, 5);
	checkError(conId);

	MOB_execute(conId, "create table if not exists candles ("
						"time datetime,"
						"open double,"
						"high double,"
						"low double,"
						"close double,"
						"vol double"
						")");
	checkError(conId);

	stmtId = MOB_prepareStatement(conId, "insert into candles values (?, ?, ?, ?, ?, ?)");
	checkError(conId);

	time(&t);
	double vals2[5] = {1.3245, 1.5532, 0.9000, 1.0001, 1044.24};
	ret = MOB_executeWithCandleData(conId, stmtId, (int) t, vals2, 5);
	checkError(conId);

	time(&t);
	double vals3[5] = {1.11114, 1.11115, 0.999999, 0.000001, 1044.24};
	ret = MOB_executeWithCandleData(conId, stmtId, (int) t, vals3, 5);
	checkError(conId);

	MOB_close(conId);
	checkError(conId);

	getchar();
	return ret;
}
