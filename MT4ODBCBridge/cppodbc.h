#include <Windows.h>
#include <tchar.h>
#include <sqlext.h>
#include <time.h>
#include <vector>

#define EX_SQLSTATE_LEN     6
#define EX_ERRMESG_LEN      SQL_MAX_MESSAGE_LENGTH

/**
 * CPPODBC: a plain object-oriented C++ wrapper for ODBC API.
 *
 * This provides read/write access for the most basic primitive data types;
 * integer, floating point number, string, and timestamp.
 * Stream types such as CLOB and BLOB are not and will not be supported.
 *
 * Parameter binding and prepared statements, and column binding and row
 * fetching are also suppored.
 * Those parameters and columns are bound to C variables through their
 * pointers.
 *
 * Classes in this namespace basically don't hide original SQL types,
 * included by sql.h and sqlext.h.
 * Those just reconstruct the original C API into an object-oriented way,
 * using constructor/destructor, inheritance, exception handling, etc.
 * A user is intended to rewrap those classes to suit your domain.
 * 
 * You should consult the original ODBC API for any details.
 * http://msdn.microsoft.com/en-us/library/ms714562(v=vs.85).aspx
 *
 * ODBC Appendixes, which includes like the C-SQL type mapping, are also
 * helpful.
 * http://msdn.microsoft.com/en-us/library/ms710927(v=VS.85).aspx
 *
 * [Converting Data from C to SQL Data Types]
 * http://msdn.microsoft.com/en-us/library/ms716298(v=VS.85).aspx
 */
namespace cppodbc
{
    /**
     * Exception class that contains SQL error message.
     */
    class ODBCException
    {
        int     nativeError;
        TCHAR   sqlState[EX_SQLSTATE_LEN];
        TCHAR   errorMesg[EX_ERRMESG_LEN];
    public:
        ODBCException(int nativeError, const TCHAR *sqlState, const TCHAR *errorMesg);
        ~ODBCException();
        inline int      getNativeError() { return nativeError; };
        inline TCHAR*   getSQLState()    { return sqlState; };
        inline TCHAR*   getErrorMesg()   { return errorMesg; };
    };

    /*
     * Just declares in the following use.
     */
    class Environment;
    class Connection;
    class Statement;

    /**
     * This represents the common resource used in ODBC API.
     * Allocated in the constructor and be freed in the destructor.
     *
     * It also manages exception handling process because the handler
     * plays a central role in ODBC API.
     */
    class Handler
    {
        friend class Environment;
        friend class Connection;
        friend class Statement;
        SQLRETURN       retcode;
        SQLSMALLINT     hType;
        SQLHANDLE       handle;
    protected:
        Handler(SQLSMALLINT hType, SQLHANDLE parent);
        ~Handler();
        SQLRETURN       process(SQLRETURN rc);
        ODBCException*  makeException();
        //TODO: extract [get|set]Attribute methods into this super class
    };

    /**
     * Environment handle which generates a Connection handle.
     */
    class Environment : public Handler
    {
    public:
        Environment();
        ~Environment();
        void    setAttribute(int aType, int value);
        int     getAttribute(int aType);
    };

    /**
     * Connection handle which generates a Statement handle.
     */
    class Connection : public Handler
    {
    public:
        Connection(Environment *env);
        ~Connection();
        void    setAttribute(int aType, int value);
        int     getAttribute(int aType);
        void    connect(const TCHAR *dsn, const TCHAR *username, const TCHAR *password);
        void    disconnect();
        int     isDead();
        void    commit();
        void    rollback();
        int     getAutoCommit();
        void    setAutoCommit(int autoCommit);
    };

    /*
     * Just declares in the following use.
     */
    class DataPointer;
    class UIntTimetPointer;

    /**
     * Statement handle which represents a SQL statement.
     * This also holds bound parameters for the prepared statement,
     * and bound columns for SELECT statement.
     * Those paramters and columns are represented by DataPointer class.
     * The lifetime of a DataPointer instance is the same with the
     * statement it belongs to.
     * This means paramters and columns are supposed to bound once per
     * statement, otherwise they leak.
     */
    class Statement : public Handler
    {
        //TODO: rewrite to use parameterHolder and columnHolder
        DataPointer*    parameters;
        int             paramNum;
        DataPointer*    columns;
        int             colNum;
        //TODO: there may be better way to roll out, like a stop condition
        void    (*rowHandler)(int rowCnt, DataPointer *dps, int size, void *anyPointer);
        void*   anyPointer;
        std::vector<DataPointer*>   parameterHolder;
        std::vector<DataPointer*>   columnHolder;
        std::vector<UIntTimetPointer*>  indirectDataPointers; //TODO: use polymorphism!
    public:
        Statement(Connection *dbc);
        ~Statement();
        void    setAttribute(int aType, int value);
        int     getAttribute(int aType);
        void    free();
        int     execute(const TCHAR *sql);
        void    prepare(const TCHAR *sql);

        void bindIntParameter(int slot, int *intp);
        void bindDoubleParameter(int slot, double *dblp);
        void bindStringParameter(int slot, TCHAR *strp);
        void bindTimestampParameter(int slot, SQL_TIMESTAMP_STRUCT *tsp);
        //void bindTimetParameter(int slot, time_t *tp);
        void bindUIntTimetParameter(int slot, unsigned int *tp);
        void bindParameter(int slot, DataPointer *dp);
        void bindParameters(DataPointer *dps, int size);

        void bindIntColumn(int slot, int *intp);
        void bindDoubleColumn(int slot, double *dblp);
        void bindStringColumn(int slot, TCHAR *strp);
        void bindTimestampColumn(int slot, SQL_TIMESTAMP_STRUCT *tsp);
        //void bindTimetColumn(int slot, time_t *tp);
        void bindUIntTimetColumn(int slot, unsigned int *tp);
        void bindColumn(int slot, DataPointer *dp);
        void bindColumns(DataPointer *dps, int size);

        void    setRowHandler(void (*rowHandler)(int rowCnt, DataPointer *dps, int size, void *anyPointer), void *anyPointer);
        int     execute();
        int     execute(bool fetchImmediatly);
        int     fetch();
        void    fetchedAll();
    private:
        int     afterExecution();
    };

    /**
     * This represents a general data to be processed.
     * It is used in parameter binding and column binding too.
     * This is subclassed to each specific type.
     * Each subclass hides the tedious setting of the original API,
     * such as SQLBindParameter and SQLBindCol.
     */
    class DataPointer
    {
        SQLSMALLINT     cType;
        SQLSMALLINT     sqlType;
        SQLULEN         columnSize;
        SQLSMALLINT     scale;
        SQLPOINTER      dataPointer;
        SQLLEN          dataSize;
        SQLLEN*         lenOrInd;
    public:
        DataPointer(SQLSMALLINT cType, SQLSMALLINT sqlType,     SQLULEN columnSize,
            SQLSMALLINT scale, SQLPOINTER  dataPointer, SQLLEN dataSize);
        ~DataPointer();
        inline SQLSMALLINT  getCType()          { return cType; };
        inline SQLSMALLINT  getSQLType()        { return sqlType; };
        inline SQLULEN      getColumnSize()     { return columnSize; };
        inline SQLSMALLINT  getScale()          { return scale; };
        inline SQLPOINTER   getDataPointer()    { return dataPointer; }
        inline SQLLEN       getDataSize()       { return dataSize; };
        inline SQLLEN*      getLenOrInd()       { return lenOrInd; };
        void    setNull();
        int     isNull();
        int     length();
        //TODO: extract more methods like value() from subclasses
    protected:
        /**
         * DataPointer contains 2 pointers.
         * One is dataPointer and another is lenOrInd.
         * The former points the data you want to process.
         * The latter points the auxiliary information about the data,
         * such as the length of the data for array types, or whether
         * the data is null or not.
         * The latter can be omitted in typical use.
         * So this field is provided by this class, not by user.
         * Note that pointed space is needed to exist for all time you
         * process.
         */
        SQLLEN  lengthOrIndicator;
    };

    /**
     * DataPointer for integers.
     */
    class IntPointer : public DataPointer
    {
    public:
        IntPointer(int *intp);
        ~IntPointer();
        inline int value() { return *((int *) getDataPointer()); };
    };

    /**
     * DataPointer for floating point numbers.
     */
    class DoublePointer : public DataPointer
    {
    public:
        DoublePointer(double *doublep);
        ~DoublePointer();
        inline double value() { return *((double *) getDataPointer()); };
    };

    /**
     * DataPointer for strings.
     */
    class StringPointer : public DataPointer
    {
    public:
        StringPointer(TCHAR *stringp, int size);
        ~StringPointer();
        inline TCHAR* value() { return (TCHAR *) getDataPointer(); };
    };

    /**
     * DataPointer for timestamps.
     * SQL_TIMESTAMP_STRUCT is declared in sql.h.
     * fractionScale represents milli or micro or nano second, but most
     * implementation like MySQL and PostgreSQL seem to ignore it.
     * Maybe recompilation with specific flag is needed to use it.
     */
    class TimestampPointer : public DataPointer
    {
    public:
        TimestampPointer(SQL_TIMESTAMP_STRUCT *timestamp);
        TimestampPointer(SQL_TIMESTAMP_STRUCT *timestamp, int fractionScale);
        ~TimestampPointer();
        inline SQL_TIMESTAMP_STRUCT *value() { return (SQL_TIMESTAMP_STRUCT *) getDataPointer(); };
        // converting methods
        static void structtm2timestamp(struct tm *in, SQL_TIMESTAMP_STRUCT *out);
        static void timestamp2structtm(SQL_TIMESTAMP_STRUCT *in, struct tm *out);
        static void timet2timestamp(time_t *in, SQL_TIMESTAMP_STRUCT *out);
        static void timestamp2timet(SQL_TIMESTAMP_STRUCT *in, time_t *out);
    };

    /**
     * Another DataPointer for timestamp type, using time_t type.
     * ODBC API recognizes only SQL_TIMESTAMP_STRUCT.
     * So if you reflect the newly binded or newly fetched value,
     * call the corresponding helper method.
     * Statement class does so through indirectDataPointers field.
     */
    class TimetPointer : public TimestampPointer
    {
        time_t*                 timeType;
        SQL_TIMESTAMP_STRUCT    timestamp;
    public:
        TimetPointer(time_t *timet);
        ~TimetPointer();
        inline time_t *value() { return timeType; };
        void rebind();
        void refetch();
    };

    /**
     * For time_t which is actually a unsigned int.
     */
    class UIntTimetPointer : public TimetPointer
    {
        unsigned int*   uintp;
        time_t          timet;
    public:
        UIntTimetPointer(unsigned int *uintTimet);
        ~UIntTimetPointer();
        inline unsigned int *value() { return uintp; };
        void rebind();
        void refetch();
    };

    /**
     * DataPointer for dates.
     */
    class DatePointer : public DataPointer
    {
    public:
        DatePointer(SQL_DATE_STRUCT *datep);
        ~DatePointer();
        inline SQL_DATE_STRUCT *value() { return (SQL_DATE_STRUCT *) getDataPointer(); };
    };

    /**
     * DataPointer for times.
     */
    class TimePointer : public DataPointer
    {
    public:
        TimePointer(SQL_TIME_STRUCT *timep);
        ~TimePointer();
        inline SQL_TIME_STRUCT *value() { return (SQL_TIME_STRUCT *) getDataPointer(); };
    };

    /**
     * DataPointer for generic numbers.
     * This uses SQL_NUMERIC_STRUCT declared in sql.h.
     * This is for text representations of numbers, especially fixed point
     * numbers and very large numbers int or double cannot handle.
     */
    class NumericPointer : public DataPointer
    {
    public:
        NumericPointer(SQL_NUMERIC_STRUCT *numericp, int precision, int scale);
        ~NumericPointer();
        inline SQL_NUMERIC_STRUCT *value() { return (SQL_NUMERIC_STRUCT *) getDataPointer(); };
        // helper methods
        static void numstruct2double(SQL_NUMERIC_STRUCT *nums, double *dbl);
        double doubleValue();
    };
}
