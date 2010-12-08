//-[stdhead]-------------------------------------------------------
//Project:
//File   :connect.h
//Started:17.09.02 13:52:55
//Updated:20.09.02 15:36:28
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
class gsodbcexp CGOdbcConnect
{
    friend class CGOdbcStmt;
 public:
    //constructor
    CGOdbcConnect();
    //destructor
    ~CGOdbcConnect();

    //establish connect by DSN name, user identifier and password
    void connect(const char *szDSN, const char *szUID, const char *szPWD);
    //establish connect by connection string
    void connect(const char *szString, HWND hWnd = 0);

    //get connection handle
    HDBC getConnect();
    //set shared connection handle
    void setConnect(HDBC);

    CGOdbcStmt *createStatement();
    void freeStatement(CGOdbcStmt *);

    //execute and bind select query
    CGOdbcStmt *executeSelect(const char *);
    //execute query and returns number of affected rows
    int executeUpdate(const char *);

    //select list of datatypes
    CGOdbcStmt *getListOfTypes();
    //select list of tables
    CGOdbcStmt *getListOfTables(const char *szTableMask = 0);
    //select list of tables
    CGOdbcStmt *getListOfColumns(const char *szTableMask = 0);

    //set transaction processing mode
    void setTransMode(bool bAutoCommit);
    //commit transaction
    void commit();
    //rollback transaction
    void rollback();

    //information about DSN
    bool firstDSN(char *pszDSN, int lMax);
    bool nextDSN(char *pszDSN, int lMax);


    //returns last error during connection
    const char *getError();
    //return name of the driver
    const char *getDriver();

    //close connection
    void close();
 protected:
    void _throwError();                 //get error description
    void _getDriver();                  //get database driver name

    HENV m_hEnv;                        //database environment
    HDBC m_hConn;                       //database connection
    bool m_bShared;                     //connection is shared
                                        //
    _bstr_t m_sError;                   //error text
    _bstr_t m_sDriver;                  //driver name
};                                      //
