#ifndef gsodbcexp
#include "MT4ODBCBridge.h"
#endif
#include <map>

class ODBCWrapper {

	int conId;
	CGOdbcConnect *connection;

public:
	ODBCWrapper(int conId);
	~ODBCWrapper();

	inline int getConnectionId() {
		return conId;
	};

	inline CGOdbcConnect *getConnection() {
		return connection;
	};

	void open(const char *dns, const char *username, const char *password);
	void close();

	void execute(const char *sql);
};
