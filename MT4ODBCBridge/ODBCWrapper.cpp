#include "ODBCWrapper.h"

ODBCWrapper::ODBCWrapper(int conId) {
	this->conId = conId;
	this->errorNo = 0;
	this->errorMesg = "";
}

ODBCWrapper::~ODBCWrapper() {
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
	CGOdbcStmt *pCur = connection->createStatement();
	try	{
		pCur->execute(sql);
	}
	catch(CGOdbcEx *e) {
		handleException(e);
	}
	connection->freeStatement(pCur);
}
