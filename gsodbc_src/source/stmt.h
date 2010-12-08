//-[stdhead]-------------------------------------------------------
//Project:
//File   :stmt.h
//Started:17.09.02 14:28:43
//Updated:24.09.02 16:41:44
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#ifndef SQL_TYPE_ORACLE_BLOB
    #define SQL_TYPE_ORACLE_BLOB    -402
#endif
#ifndef SQL_TYPE_ORACLE_CLOB
    #define SQL_TYPE_ORACLE_CLOB    -401
#endif

class gsodbcexp CGOdbcStmt
{
 protected:
    friend class CGOdbcConnect;                         //only connection class can create Stmt
    //--------------------------------------------------//
    CGOdbcStmt(HDBC hDbc);                              //stmt constructor
    ~CGOdbcStmt();                                      //destructor
 public:                                                //
    //--------------------------------------------------//
    //structures                                        //
    //--------------------------------------------------//
    typedef struct                                      //ODBC data
    {                                                   //
        _int16 iYear;                                   //year
        _int16 iMonth;                                  //month
        _int16 iDay;                                    //day
    }DATE;                                              //
                                                        //
    typedef struct                                      //ODBC timestamp
    {                                                   //
        _int16 iYear;                                   //year
        _int16 iMonth;                                  //month
        _int16 iDay;                                    //day
        _int16 iHour;                                   //hour
        _int16 iMin;                                    //minutes
        _int16 iSec;                                    //seconds
        _int32 iFract;                                  //fractions of second
    }TIMESTAMP;                                         //
                                                        //
    typedef struct                                      //description of one field
    {                                                   //
        friend class CGOdbcStmt;                        //
        char szName[MAX_PATH];                          //field name
        short iSqlType;                                 //SQL datatype
        short iSqlCType;                                //SQL-C datatype
        int iLen;                                       //length of data buffer
        unsigned long iSqlSize;                         //SQL size of datatype
        short iScale;                                    //precision of value
        bool bLOB;                                      //data is LOB data
    protected:                                          //
        long lOffset;                                   //offset in buffer
        unsigned char *pData;                           //point to item data
        long cbData;                                    //count of data
    }COLUMN;                                            //
                                                        //
    typedef struct                                      //description of one parameter
    {                                                   //
        friend class CGOdbcStmt;                        //
        short iSqlType;                                 //SQL datatype
        short iSqlCType;                                //SQL-C datatype
        int iLen;                                       //length of data buffer
        unsigned long iSqlSize;                         //SQL size of datatype
        short iScale;                                   //precision of value
        bool bLOB;                                      //parameter is LOB object
    protected:                                          //
        long lOffset;                                   //offset in buffer
        unsigned char *pData;                           //point to item data
        long cbData;                                    //count of data
    }PARAM;                                             //
    //--------------------------------------------------//
    //execution methods                                 //
    void setTimeOut(int iMSec);                         //set timeout for next execute operation
    void execute(const char *);                         //execute query
    void prepare(const char *);                         //execute prepared query
    void execute();                                     //execute prepared query
    bool moreResults();                                 //switch to next result
    long getRowCount();                                 //returns number of rows
    bool bindAuto();                                    //bind data automatically
    bool bindParamsAuto();                              //bind param automatically
    void setParamNumber(int);                           //set number of parameters manually
    void bindParam(int, short, short, int,              //
                   unsigned long, short);               //
    const char *getError();                             //returns last error
    void cancel();                                      //cancel statement
    void toNative(const char *, char *, int);           //convert to native SQL
    //--------------------------------------------------//
    //moving methods                                    //
    //--------------------------------------------------//
    bool first();                                       //go to first row
    bool last();                                        //go to last row
    bool next();                                        //go to next row
    bool prev();                                        //go to previous row
    bool go(int);                                       //go to row
    bool skip(int);                                     //skip number of rows
    int getRowNo();                                     //returns current row index
    //--------------------------------------------------//
    //data access methods                               //
    //--------------------------------------------------//
    int getInt(int iCol);                               //returns integer value
    const char *getChar(int iCol);                      //returns zero-terminated character value
    int getLength(int iCol);                            //returns length of value
    const void *getPtr(int iCol);                       //returns pointer to value
    bool isNull(int iCol);                              //returns true if value is null
    double getNumber(int iCol);                         //returns number value
    const DATE *getDate(int iCol);                      //returns date-only value
    const TIMESTAMP *getTimeStamp(int iCol);            //returns timestamp value
    const GUID *getGUID(int iCol);                      //returns value as GUID
    //--------------------------------------------------//
    //work with fields                                  //
    //--------------------------------------------------//
    int getColCount();                                  //returns number of columns
    int findColumn(const char *);                       //finds column by name
    const COLUMN *getColumn(int iCol);                  //return columns description
    int getInt(const char *szCol);                      //returns integer value
    const char *getChar(const char *szCol);             //returns zero-terminated character value
    int getLength(const char *szCol);                   //returns length of value
    const void *getPtr(const char *szCol);              //returns pointer to value
    bool isNull(const char *szCol);                     //returns true if value is null
    double getNumber(const char *szCol);                //returns number value
    const DATE *getDate(const char *szCol);             //returns date-only value
    const TIMESTAMP *getTimeStamp(const char *szCol);   //returns timestamp value
    const GUID *getGUID(const char *szCol);             //returns value as GUID
    //--------------------------------------------------//
    //parameter set method                              //
    //--------------------------------------------------//
    void setInt(int iParam, int iValue);                //set integer value
    void setChar(int iParam, const char *pParam);       //set character value
    void setBin(int iParam, const void *pBin, int iLen);//set binary value
    void setNumber(int iParam, double dblVal);          //set double value
    void setDate(int iParam, const DATE *pDate);        //set date value
    void setTimeStamp(int iParam,                       //set timestamp value
                      const TIMESTAMP *pDate);          //
    void setGUID(int iParam, const GUID *);             //set guid value
    void setNull(int iParam);                           //set parameter value to null
    //--------------------------------------------------//
    HSTMT getStmt();                                    //returns ODBC statement
 protected:                                             //
    void _throwError();                                 //get description of error
    void _beforeMove();                                 //clear buffers before move
    void _free();                                       //free internal buffer
    void _loadBlob(int iCol);                           //load blob data
    //--------------------------------------------------//
    HSTMT m_hStmt;                                      //statement handle
    _bstr_t m_sError;                                   //error handle
    _bstr_t m_sLastStr;                                 //latest returned string
    //--------------------------------------------------//
    short m_iColCount;                                  //number of columns
    COLUMN *m_pColumns;                                 //array of columns
    unsigned char *m_pData;                             //data memory for non-blob data
    //--------------------------------------------------//
    short m_iParamsCount;                               //number of parameters
    PARAM *m_pParams;                                   //parameter list
    unsigned char *m_pParamData;                        //parameter's data
    int m_iBinded;                                      //
    //--------------------------------------------------//
    int m_iTimeOut;                                     //
};                                                      //
