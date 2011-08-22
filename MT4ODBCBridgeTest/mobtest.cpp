#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <tchar.h>

#pragma pack(push,1)
struct RateInfo
{
	unsigned int      ctm;
	double            open;
	double            low;
	double            high;
	double            close;
	double            vol;
};
#pragma pack(pop)

struct MqlStr
{
	int               len;
	char             *string;
};

int			__stdcall MOB_create			();
int			__stdcall MOB_open				(const int conId, const TCHAR *dns, const TCHAR *username, const TCHAR *password);
int			__stdcall MOB_close				(const int conId);
int			__stdcall MOB_isDead			(const int conId);
int			__stdcall MOB_commit			(const int conId);
int			__stdcall MOB_rollback			(const int conId);
int			__stdcall MOB_getAutoCommit		(const int conId);
int			__stdcall MOB_setAutoCommit		(const int conId, const int autoCommit);
int			__stdcall MOB_execute			(const int conId, const TCHAR *sql);
int			__stdcall MOB_getLastErrorNo	(const int conId);
const TCHAR* __stdcall MOB_getLastErrorMesg	(const int conId);
int			__stdcall MOB_registerStatement	(const int conId, const TCHAR *sql);
int			__stdcall MOB_unregisterStatement(const int conId, const int stmtId);
int         __stdcall MOB_bindIntParameter  (const int conId, const int stmtId, const int slot, int *intp);
int         __stdcall MOB_bindDoubleParameter(const int conId, const int stmtId, const int slot, double *dblp);
int         __stdcall MOB_bindStringParameter(const int conId, const int stmtId, const int slot, TCHAR *strp);
int         __stdcall MOB_bindDatetimeParameter(const int conId, const int stmtId, const int slot, unsigned int *timep);
int         __stdcall MOB_bindToFetch       (const int conId, const int stmtId, unsigned int *tVals, const int tSize, int *iVals, const int iSize, double *dVals, const int dSize, MqlStr *sVals, const int sSize);
int         __stdcall MOB_bindIntColumn     (const int conId, const int stmtId, const int slot, int *intp);
int         __stdcall MOB_bindDoubleColumn  (const int conId, const int stmtId, const int slot, double *dblp);
int         __stdcall MOB_bindStringColumn  (const int conId, const int stmtId, const int slot, TCHAR *strp);
int         __stdcall MOB_bindDatetimeColumn   (const int conId, const int stmtId, const int slot, unsigned int *timep);
int         __stdcall MOB_executeStatement  (const int conId, const int stmtId);
int         __stdcall MOB_selectToFetch     (const int conId, const int stmtId);
int         __stdcall MOB_fetch             (const int conId, const int stmtId);
int         __stdcall MOB_fetchedAll        (const int conId, const int stmtId);
int			__stdcall MOB_insertTick		(const int conId, const int stmtId, unsigned int datetime, int fraction, double *vals, const int size);
int			__stdcall MOB_insertBar			(const int conId, const int stmtId, unsigned int datetime, double *vals, const int size);
int			__stdcall MOB_copyRates 		(const int conId, const int stmtId, RateInfo *rates, const int size, const int start, const int end);
double		__stdcall MOB_selectDouble		(const int conId, const TCHAR *sql, const double defaultVal);
int			__stdcall MOB_selectInt			(const int conId, const TCHAR *sql, const int defaultVal);
unsigned int __stdcall MOB_selectDatetime	(const int conId, const TCHAR *sql, unsigned int defaultVal);


static void logIfError(int conId, int rc)
{
	if (rc < 0)
	{
		_tprintf(_T("ERROR(%d):%s\n"), MOB_getLastErrorNo(conId), MOB_getLastErrorMesg(conId));
	}
}


int testTick()
{
	int rc = 0;
	_tprintf(_T("=== MT4-ODBC Bridge Test Tick Begin\n"));

	int conId;
	conId = MOB_create();

	rc = MOB_open(conId, _T("testpostgresql"), _T("testuser"), _T("password"));
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_isDead(conId);
	if (!rc) _tprintf(_T("Connection is alive.\n"));

	rc = MOB_execute(conId, _T("drop table if exists ticks"));
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_execute(conId, _T("create table ticks (time timestamp, fraction smallint, ask decimal(8, 5), bid decimal(8, 5), primary key (time, fraction))"));
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_selectInt(conId, _T("select fraction from ticks"), 0);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	int insertTick;
	insertTick = MOB_registerStatement(conId, _T("insert into ticks values (?, ?, ?, ?)"));
	rc = insertTick;
	if (rc < 0) { logIfError(conId, rc); return rc; }

	time_t t;
	int fraction;
	double ab[2];
	int nab;

	time(&t);
	fraction = 123;
	ab[0] = 123.12345;
	ab[1] = 321.54321;
	nab = 2;

	rc = MOB_insertTick(conId, insertTick, (unsigned int) t, fraction, ab, nab);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	t += 60;
	fraction = 444;
	ab[0] = 123.00001;
	ab[1] = 321.00009;
	nab = 2;

	rc = MOB_insertTick(conId, insertTick, (unsigned int) t, fraction, ab, nab);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_unregisterStatement(conId, insertTick);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	int ti = MOB_selectInt(conId, _T("select count(*) from ticks"), -1);

	double td = MOB_selectDouble(conId, _T("select ask from ticks order by time, fraction"), -1.0);

	time_t tt = MOB_selectDatetime(conId, _T("select time from ticks order by time desc, fraction desc"), 0);
	if (tt == t)
		_tprintf(_T("Got the same time %d\n"), tt);

	rc = MOB_close(conId);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	_tprintf(_T("=== MT4-ODBC Bridge Test Tick End\n"));
	return rc;
}

int testBar()
{
	int rc = 0;
	_tprintf(_T("=== MT4-ODBC Bridge Test Bar Begin\n"));

	int conId;
	conId = MOB_create();

	rc = MOB_open(conId, _T("testpostgresql"), NULL, NULL);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_isDead(conId);
	if (!rc) _tprintf(_T("Connection is alive.\n"));

	rc = MOB_execute(conId, _T("drop table if exists bars"));
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_execute(conId, _T("create table bars (time timestamp, open decimal(8, 5), low decimal(8, 5), high decimal(8, 5), close decimal(8, 5), vol double precision, primary key (time))"));
	if (rc < 0) { logIfError(conId, rc); return rc; }

	double dblNull = MOB_selectDouble(conId, _T("select max(open) from bars"), -1);
	_tprintf(_T("DblNull is %f\n"), dblNull);

	int insertBar;
	insertBar = MOB_registerStatement(conId, _T("insert into bars values (?, ?, ?, ?, ?, ?)"));
	rc = insertBar;
	if (rc < 0) { logIfError(conId, rc); return rc; }

/*
	double rates[][6] = {
		{0,		0.1, 0.1, 0.1, 0.1,		1},
		{60,	0.2, 0.2, 0.2, 0.2,		2},
		{120,	0.3, 0.3, 0.3, 0.3,		3}
	};
*/
	RateInfo r1;
	RateInfo r2;
	RateInfo r3;
	r1.ctm		= 0;
	r1.open		= 0.1;
	r1.low		= 0.1;
	r1.high		= 0.1;
	r1.close	= 0.1;
	r1.vol		= 1;
	r2.ctm		= 60;
	r2.open		= 0.2;
	r2.low		= 0.2;
	r2.high		= 0.2;
	r2.close	= 0.2;
	r2.vol		= 2;
	r3.ctm		= 120;
	r3.open		= 0.3;
	r3.low		= 0.3;
	r3.high		= 0.3;
	r3.close	= 0.3;
	r3.vol		= 3;
	RateInfo rates[] = {r1, r2, r3};

	rc = MOB_copyRates(conId, insertBar, rates, 3, 0, 3);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_close(conId);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	_tprintf(_T("=== MT4-ODBC Bridge Test Bar End\n"));
	return rc;
}

int testGeneric()
{
	int rc = 0;
	_tprintf(_T("=== MT4-ODBC Bridge Test Generic Begin\n"));

	int conId;
	conId = MOB_create();

	rc = MOB_open(conId, _T("testpostgresql"), NULL, NULL);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_isDead(conId);
	if (!rc) _tprintf(_T("Connection is alive.\n"));

	rc = MOB_execute(conId, _T("drop table if exists generic"));
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_execute(conId, _T("create table generic (time1 timestamp, int1 integer, dbl1 double precision, str1 varchar(10))"));
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_registerStatement(conId, _T("insert into generic (time1, int1, dbl1, str1) values (?, ?, ?, ?)"));
	if (rc < 0) { logIfError(conId, rc); return rc; }
	int insertStmt = rc;

	time_t t1;
	time(&t1);

	int i1 = 123;

	double d1 = 1.23;

	TCHAR *s1 = (TCHAR *) malloc(sizeof(TCHAR) * 10);
	_tcsncpy_s(s1, 10, _T("fugafuga"), _TRUNCATE);

	rc = MOB_bindDatetimeParameter(conId, insertStmt, 1, (unsigned int *) &t1);
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_bindIntParameter(conId, insertStmt, 2, &i1);
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_bindDoubleParameter(conId, insertStmt, 3, &d1);
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_bindStringParameter(conId, insertStmt, 4, s1);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_executeStatement(conId, insertStmt);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	i1 = 222;

	d1 = 4.56;

	_tcsncpy_s(s1, 10, _T("oeoeoeoehh"), _TRUNCATE);

	t1 += 60;
	
	rc = MOB_executeStatement(conId, insertStmt);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	int i2 = -1;
	double d2 = -1.2;
	TCHAR *s2 = (TCHAR *) malloc(sizeof(TCHAR) * 10);
	time_t t2;

	_tcsncpy_s(s2, 10, _T("1234567890"), _TRUNCATE);

	rc = MOB_registerStatement(conId, _T("select time1, int1, dbl1, str1 from generic"));
	if (rc < 0) { logIfError(conId, rc); return rc; }
	int selectStmt = rc;

	rc = MOB_bindDatetimeColumn(conId, selectStmt, 1, (unsigned int *) &t2);
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_bindIntColumn(conId, selectStmt, 2, &i2);
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_bindDoubleColumn(conId, selectStmt, 3, &d2);
	if (rc < 0) { logIfError(conId, rc); return rc; }
	rc = MOB_bindStringColumn(conId, selectStmt, 4, s2);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_selectToFetch(conId, selectStmt);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	while (MOB_fetch(conId, selectStmt)) {
		//_tprintf(_T("%i %i %f %s\n"), t2, i2, d2, s2);
		_tprintf(_T("%i\n"), t2);
		_tprintf(_T("%i\n"), i2);
		_tprintf(_T("%f\n"), d2);
		_tprintf(_T("%s\n"), s2);
	}

	MOB_fetchedAll(conId, selectStmt);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	rc = MOB_close(conId);
	if (rc < 0) { logIfError(conId, rc); return rc; }

	_tprintf(_T("=== MT4-ODBC Bridge Test Generic End\n"));
	return rc;
}

int mobtest(int argc, TCHAR *argv[])
{
	int rc = 0;
	rc += testTick();
	rc += testBar();
	rc += testGeneric();
	return rc;
}
