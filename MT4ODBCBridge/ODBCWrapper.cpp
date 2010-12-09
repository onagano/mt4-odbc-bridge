#include "ODBCWrapper.h"

ODBCWrapper::ODBCWrapper(const char *name) {
	this->name = name;
}

ODBCWrapper::~ODBCWrapper() {
	if (connection != NULL)
		close();
}

void ODBCWrapper::open(const char *dns, const char *username, const char *password) {
	connection = new CGOdbcConnect();
	connection->connect(dns, username, password);
}

void ODBCWrapper::close() {
	connection->close();
	delete connection;
}

void ODBCWrapper::execute(const char *sql) {
	CGOdbcStmt *pCur;
	pCur = connection->createStatement();
	try
	{
		pCur->execute(sql);
	}
	catch(CGOdbcEx *pE)
	{
		printf("execute error\n%s\n", pE->getMsg());
		return;
	}
	connection->freeStatement(pCur);
}
