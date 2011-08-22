#include "cppodbc.h"
#include <map>

#define MOB_OK              0
#define MOB_NG              -1
#define MOB_UNDEFINED       -2
#define MOB_INVALID_CONID   -3
#define MOB_INVALID_STMTID  -4

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

using namespace cppodbc;

class ODBCWrapper
{
    Environment*    environment;
    Connection*     connection;
    /**
     * Cache for prepared statements.
     */
    std::map<int, Statement*> stmts;
    int             nextStmtId;
    int             lastErrorNo;
    TCHAR           lastErrorMesg[EX_SQLSTATE_LEN + EX_ERRMESG_LEN + 10];
public:
    ODBCWrapper();
    ~ODBCWrapper();
    int     open(const TCHAR *dsn, const TCHAR *username, const TCHAR *password);
    void    close();
    int     isDead();
    int     commit();
    int     rollback();
    int     getAutoCommit();
    int     setAutoCommit(int autoCommit);
    int     execute(const TCHAR *sql);
    int     getLastErrorNo();
    TCHAR*  getLastErrorMesg();
    int     registerStatement(const TCHAR *sql);
    int     unregisterStatement(int stmtId);

    int     bindIntParameter(int stmtId, int slot, int *intp);
    int     bindDoubleParameter(int stmtId, int slot, double *dblp);
    int     bindStringParameter(int stmtId, int slot, TCHAR *strp);
    int     bindDatetimeParameter(int stmtId, int slot, unsigned int *timep);

    int     bindIntColumn(int stmtId, int slot, int *intp);
    int     bindDoubleColumn(int stmtId, int slot, double *dblp);
    int     bindStringColumn(int stmtId, int slot, TCHAR *strp);
    int     bindDatetimeColumn(int stmtId, int slot, unsigned int *timep);

    int     executeStatement(int stmtId);
    int     selectToFetch(int stmtId);
    int     fetch(int stmtId);
    int     fetchedAll(int stmtId);

    // Custom methods
    int     insertTick(const int stmtId, unsigned int datetime, int fraction, double *vals, const int size);
    int     insertBar(const int stmtId, unsigned int datetime, double *vals, const int size);
    int     copyRates(const int stmtId, RateInfo *rates, const int size, const int start, const int end);
    double  selectDouble(const TCHAR *sql, const double defaultVal);
    int     selectInt(const TCHAR *sql, const int defaultVal);
    //TODO: implement selectString method
    unsigned int selectDatetime(const TCHAR *sql, const unsigned int defaultVal);
private:
    void    handleException(ODBCException *e);
};
