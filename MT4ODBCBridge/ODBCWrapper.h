#ifndef gsodbcexp
#include "MT4ODBCBridge.h"
#endif
#include <map>

class ODBCWrapper {

	int conId;
	CGOdbcConnect *connection;
	int errorNo;
	char *errorMesg;
	int nextStmtId;
	std::map<int, CGOdbcStmt*> stmts;

	void handleException(CGOdbcEx *e);

public:
	ODBCWrapper(int conId);
	~ODBCWrapper();

	inline int getConnectionId() {
		return conId;
	};

	inline CGOdbcConnect *getConnection() {
		return connection;
	};

	inline int getLastErrorNo() {
		int temp = errorNo;
		errorNo = 0;
		return temp;
	};

	inline char *getLastErrorMesg() {
		char *temp = errorMesg;
		errorMesg = "";
		return temp;
	};

	void open(const char *dns, const char *username, const char *password);
	void close();

	void execute(const char *sql);

	int prepareStatement(const char *sql);
	int executeWithTickData(int stmtId, int datetime, int millis, double *vals, int size);
	int executeWithCandleData(int stmtId, int datetime, double *vals, int size);
};
