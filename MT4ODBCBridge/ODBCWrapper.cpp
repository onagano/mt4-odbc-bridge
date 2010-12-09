#include "ODBCWrapper.h"
#include <time.h>

ODBCWrapper::ODBCWrapper(int conId) {
	this->conId = conId;
	this->errorNo = 0;
	this->errorMesg = "";
	this->nextStmtId = 1;
	//this->stmts = new std::map<int, CGOdbcStmt*>();
}

ODBCWrapper::~ODBCWrapper() {
	//delete stmts;
	if (connection) close();
}

void ODBCWrapper::handleException(CGOdbcEx *e) {
	long lcode = e->getCode();
	errorNo = lcode ? (int) lcode : -1;
	errorMesg = (char *) e->getMsg();
}

void ODBCWrapper::open(const char *dns, const char *username, const char *password) {
	connection = new CGOdbcConnect();
	try {
		connection->connect(dns, username, password);
	}
	catch (CGOdbcEx *e) {
		handleException(e);
		delete connection;
		connection = NULL;
	}
}

void ODBCWrapper::close() {
	if (!connection) return;
	try {
		connection->close();
	}
	catch (CGOdbcEx *e) {
		handleException(e);
	}
	delete connection;
}

void ODBCWrapper::execute(const char *sql) {
	if (!connection) return;
	CGOdbcStmt *stmt = connection->createStatement();
	try	{
		stmt->execute(sql);
	}
	catch(CGOdbcEx *e) {
		handleException(e);
	}
	connection->freeStatement(stmt);
}

int ODBCWrapper::prepareStatement(const char *sql) {
	if (!connection) return -1; //TODO: define error enums

	int ret = -2;
	CGOdbcStmt *stmt = connection->createStatement();
	try	{
		stmt->prepare(sql);
		ret = nextStmtId++;
		stmts[ret] = stmt;
	}
	catch(CGOdbcEx *e) {
		handleException(e);
		connection->freeStatement(stmt);
	}

	return ret;
}

void inttimet2timestamp(int inttimet, int millis, CGOdbcStmt::TIMESTAMP *ts) {
	time_t t = (time_t) inttimet;
	struct tm *dt = localtime(&t);
	ts->iYear	= dt->tm_year + 1900;
	ts->iMonth	= dt->tm_mon + 1;
	ts->iDay	= dt->tm_mday;
	ts->iHour	= dt->tm_hour;
	ts->iMin	= dt->tm_min;
	ts->iSec	= dt->tm_sec;
	ts->iFract	= millis;
}

// SQLRowCount
// http://msdn.microsoft.com/en-us/library/ms711835(v=VS.85).aspx
// SQLBindParameter
// http://msdn.microsoft.com/en-us/library/ms710963(v=VS.85).aspx

int ODBCWrapper::executeWithTickData(int stmtId, int datetime, int millis, double *vals, int size) {
	if (!connection) return -1;
	int ret = -3;
	CGOdbcStmt *stmt = stmts[stmtId];
	try {
		const int offset = 2;
		const int n = offset + size;

		//if (!stmt->bindParamsAuto()) {
			//printf("Binding...\n");
			stmt->setParamNumber(n);
			stmt->bindParam(0, SQL_TIMESTAMP, SQL_C_TIMESTAMP, 23, 23, 3);
			stmt->bindParam(1, SQL_INTEGER, SQL_C_LONG, 4, 0, 0);
			for (int i = offset; i < n; i++) {
				stmt->bindParam(i, SQL_DOUBLE, SQL_C_DOUBLE, 0, 8, 5);
			}
		//}

		CGOdbcStmt::TIMESTAMP ts;
		inttimet2timestamp(datetime, millis, &ts);
		stmt->setTimeStamp(0, &ts);
		stmt->setInt(1, millis);
		for (int i = offset; i < n; i++) {
			stmt->setNumber(i, vals[i - offset]);
		}
		
		stmt->execute();
		ret = (int) stmt->getRowCount();
	}
	catch(CGOdbcEx *e) {
		handleException(e);
	}
	return ret;
}

int ODBCWrapper::executeWithCandleData(int stmtId, int datetime, double *vals, int size) {
	if (!connection) return -1;
	int ret = -4;
	CGOdbcStmt *stmt = stmts[stmtId];
	try {
		const int offset = 1;
		const int n = offset + size;

		//if (!stmt->bindParamsAuto()) {
			//printf("Binding...\n");
			stmt->setParamNumber(n);
			stmt->bindParam(0, SQL_TIMESTAMP, SQL_C_TIMESTAMP, 23, 23, 3);
			for (int i = offset; i < n; i++) {
				stmt->bindParam(i, SQL_DOUBLE, SQL_C_DOUBLE, 0, 8, 5);
			}
		//}

		CGOdbcStmt::TIMESTAMP ts;
		inttimet2timestamp(datetime, 0, &ts);
		stmt->setTimeStamp(0, &ts);
		for (int i = offset; i < n; i++) {
			stmt->setNumber(i, vals[i - offset]);
		}
		
		stmt->execute();
		ret = (int) stmt->getRowCount();
	}
	catch(CGOdbcEx *e) {
		handleException(e);
	}
	return ret;
}

