#ifndef gsodbcexp
#include "MT4ODBCBridge.h"
#endif
#include <map>

class ODBCWrapper {

	CGOdbcConnect *connection;

public:
	ODBCWrapper();
	~ODBCWrapper();

	void open(const char *dns, const char *username, const char *password);
	void close();

	void execute(const char *sql);
};
