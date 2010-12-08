//-[stdhead]-------------------------------------------------------
//Project:
//File   :curidx.h
//Started:24.09.02 16:45:40
//Updated:25.09.02 11:50:32
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
class gsodbcexp CGCursorIndex                       //
{                                                   //
    friend class CGCursor;                          //
 public:                                            //
    CGCursorIndex(CGCursor *pCursor, int iColumn,   //
          bool (_cdecl *pCallback)(long, long) = 0);//construct and create index
    CGCursorIndex(CGCursor *pCursor,                //load index from file
                  const char *szFileName);          //
    ~CGCursorIndex();                               //destructor
    //----------------------------------------------//
    void save(const char *szFileName);              //save index to file
    void go(int iIndexRow);                         //go to row by index
    int seekForAbsRow(int iAbsRow);                 //seek for absolute row number
    int seekFor(int iVal);                          //seek for integer value
    int seekFor(double dblVal);                     //seek for double value
    int seekFor(const char *szValue, bool bExact);  //seek for character value
    int seekFor(const CGOdbcStmt::DATE *pVal);      //seek for date value
    int seekFor(const CGOdbcStmt::TIMESTAMP *pVal); //seek for timestamp value
    int seekFor(const GUID *pVal);                  //seek for timestamp value
 protected:                                         //
    void _addRow(int iRow);                         //add row to index
    void _removeRow(int iRow);                      //remove row from index
    int _compareRows(int iRow1, int iRow2);         //compare two rows of cursor
    int _compareWith(int iValue, int iRow);         //compare value with row
    int _compareWith(double iValue, int iRow);      //compare value with row
    int _compareWith(const char *szValue, int iRow, bool);        //compare value with row
    int _compareWith(const CGOdbcStmt::DATE *, int iRow);         //compare value with row
    int _compareWith(const CGOdbcStmt::TIMESTAMP *, int iRow);    //compare value with row
    int _compareWith(const GUID *, int iRow);                     //compare value with row
    int _cmpDate(const CGOdbcStmt::DATE *,          //compare two dates
                 const CGOdbcStmt::DATE *);         //
    int _cmpTimeStamp(const CGOdbcStmt::TIMESTAMP *,    //compare two timestamps
                      const CGOdbcStmt::TIMESTAMP *);   //
    int _cmpGUID(const GUID *pGUID1,                    //compare two GUIDs
                 const GUID *pGUID2);                   //
    //----------------------------------------------//
    CGCursor *m_pCursor;                            //reference to cursor
    CGCursor::COLUMNTYPE m_eType;                   //datatype of the column
    int m_iColumn;                                  //index of column
    std::vector<int> m_aIndex;                      //index itself
};
