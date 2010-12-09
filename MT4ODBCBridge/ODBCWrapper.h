#ifndef gsodbcexp
#include "MT4ODBCBridge.h"
#endif
#include <map>

class ODBCWrapper {

	const char *name;
	CGOdbcConnect *connection;

public:
	ODBCWrapper(const char *name);
	~ODBCWrapper();

	inline const char *getName() {
		return name;
	};

	inline CGOdbcConnect *getConnection() {
		return connection;
	};

	void open(const char *dns, const char *username, const char *password);
	void close();

	void execute(const char *sql);
};
