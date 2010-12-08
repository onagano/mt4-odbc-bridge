//-[stdhead]-------------------------------------------------------
//Project:
//File   :curex.h
//Started:23.09.02 18:25:30
//Updated:25.09.02 14:11:25
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
class gsodbcexp CGCursor
{
    friend class CGCursorIndex;
 public:
    //-------------------------------------------------------------
    //data structures
    //-------------------------------------------------------------
    typedef enum                                    //
    {                                               //
        eInt,                                       //integer value
        eChar,                                      //fixed length character value
        eBinary,                                    //fixed length binary value
        eDouble,                                    //double value
        eDate,                                      //only-date value
        eTimeStamp,                                 //timestamp value
        eGUID,                                      //GUID value
        eCLOB,                                      //large text value
        eBLOB,                                      //large binary value
    }COLUMNTYPE;                                    //
    //----------------------------------------------//
    typedef struct                                  //
    {                                               //
        char szName[MAX_PATH];                      //name of the field
        COLUMNTYPE eType;                           //datatype of the column
        long lSize;                                 //size of the column
        long lScale;                                //scale of the data
    }COLUMN;                                        //
 public:                                            //
    //constructor for statement                     //
    CGCursor(CGOdbcStmt *pStmt,                     //construct cursor driver for statement
             const char *szFileName = 0);           //
    //constructor for persistent file               //
    CGCursor(const char *szFileName);               //construct cursor driver from previously saved file
    //constructor for new persistent file           //
    CGCursor(int iColumn, COLUMN *pColumn,          //construct cursor new driver for new file
             const char *szFileName = 0);           //
    //destructor                                    //
    ~CGCursor();                                    //
    //----------------------------------------------//
    //operations                                    //
    //----------------------------------------------//
    void go(int iRow);                              //go to specified row
    bool isRowDeleted();                            //returns true if row is deleted row
    bool isRowUpdated();                            //returns true if row is updated row
    bool isRowNew();                                //returns true if row is new row
    int getRowCount();                              //returns number of rows
    int getRowNo();                                 //returns number of current row
    //----------------------------------------------//
    int getColCount();                              //
    int findColumn(const char *szName);             //
    const COLUMN *getColumn(int iColumn);           //
    //----------------------------------------------//
    int getInt(int iColumn);                        //returns integer value
    const char *getChar(int iColumn);               //returns character value
    double getNumber(int iColumn);                  //returns numeric value
    const CGOdbcStmt::DATE *getDate(int iColumn);   //returns data value
    const CGOdbcStmt::TIMESTAMP *                   //returns timestamp value
          getTimeStamp(int iColumn);                //
    const void *getPtr(int iColumn);                //returns pointer value
    int getLength(int iColumn);                     //returns length of the value
    const GUID * getGUID(int iColumn);              //returns GUID value
    bool isNull(int iColumn);                       //returns true if value is null value
    //----------------------------------------------//
    int getInt(const char *szName);                 //returns integer value
    const char *getChar(const char *szName);        //returns character value
    double getNumber(const char *szName);           //returns numeric value
    const CGOdbcStmt::DATE *getDate(const char *szName);   //returns data value
    const CGOdbcStmt::TIMESTAMP *                   //returns timestamp value
          getTimeStamp(const char *szName);         //
    const void *getPtr(const char *szName);         //returns pointer value
    int getLength(const char *szName);              //returns length of the value
    const GUID * getGUID(const char *szName);       //returns GUID value
    bool isNull(const char *szName);                //returns true if value is null value
    //----------------------------------------------//
    //update operations                             //
    //----------------------------------------------//
    void deleteRow();                               //marks row as delete
    bool isDeleted();                               //returns true if row is deleted
    void update();                                  //update current row
    void append();                                  //append new row
    //----------------------------------------------//
    void setInt(int iColumn, int iVal);             //set integer value
    void setChar(int iColumn, const char *);        //set character value
    void setNumber(int iColumn, double dblVal);     //set numeric value
    void setDate(int iColumn,                       //set date value
                 const CGOdbcStmt::DATE *);         //
    void setTimeStamp(int iColumn,                  //set timestamp value
                    const CGOdbcStmt::TIMESTAMP *); //
    void setBin(int iColumn, const void *, int);    //set binary value
    void setGUID(int iColumn, const GUID * );       //set GUID value
    void setNull(int iColumn);                      //set value to null
    //----------------------------------------------//
    void setInt(const char *szName, int iVal);             //set integer value
    void setChar(const char *szName, const char *);        //set character value
    void setNumber(const char *szName, double dblVal);     //set numeric value
    void setDate(const char *szName,                       //set date value
                 const CGOdbcStmt::DATE *);                //
    void setTimeStamp(const char *szName,                  //set timestamp value
                    const CGOdbcStmt::TIMESTAMP *);        //
    void setBin(const char *szName, const void *, int);    //set binary value
    void setGUID(const char *szName, const GUID * );       //set GUID value
    void setNull(const char *szName);                      //set value to null
    //----------------------------------------------//
 protected:                                         //
    void _init();                                   //initialize variables
    void _beforeMove();                             //prepare to move operation
    void _loadLOB(int iColumn);                     //load LOB object
    //----------------------------------------------//
    //types                                         //
    //----------------------------------------------//
    typedef struct                                  //
    {                                               //
        unsigned char *pData;                       //pointer to value buffer
        unsigned char *pLOB;                        //pointer to LOB data
        bool bLOBChanged;                           //LOB is changed
    }COLBUF;                                        //
    //----------------------------------------------//
    typedef enum                                    //
    {                                               //row statues
        eMirror = 'M',                              //data is mirror of data on server
        eNew = 'N',                                 //data is new data (only in memory)
        eDeleted = 'D',                             //data was deleted
        eUpdated = 'U',                             //data was updates (only in memory)
    }ROWSTATUS;                                     //
    //----------------------------------------------//
    char m_szFileName[MAX_PATH];                    //name of the file with cursor
    char m_szLobFileName[MAX_PATH];                 //name of the file with cursor
    long m_lLobSize;                                //size of the LOB file
    bool m_bPersist;                                //persistency flag
    FILE *m_pMainFile;                              //handle to main cursor file
    FILE *m_pLobFile;                               //handle to LOB cursor file
    //----------------------------------------------//
    int m_lColCount;                                //quantity of columns
    int m_lRowCount;                                //quantity of rows
    int m_lCurRow;                                  //
    COLUMN *m_pColumns;                             //list of columns
    COLBUF *m_pColBufs;                             //list of column buffers
    unsigned char *m_pData;                         //data buffer
    long m_lBufSize;                                //size of one buffer
    long m_lDataOffset;                             //offset of data
    //----------------------------------------------//
    std::vector<CGCursorIndex *> m_aIndexes;        //list of indexes
    void _addIndex(CGCursorIndex *);                //
    void _removeIndex(CGCursorIndex *);             //
    //----------------------------------------------//
    _bstr_t m_sLastStr;                             //
};                                                  //
