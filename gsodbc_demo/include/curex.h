//-[stdhead]-------------------------------------------------------
//Project:
//File   :curex.h
//Started:24.09.02 15:42:26
//Updated:25.09.02 12:40:52
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
class gsodbcexp CGCursorEx
{
 public:
    typedef enum
    {
        eFileOpen = 1,              //%s
        eNoColumns,                 //
        eUnknownType,               //%i
        eWriteError,                //
        eInvalidRow,                //%i
        eInvalidColumn,             //%i
        eIncompatibleType,          //
        eReadError,                 //
        eBroken,                    //
        eInvalidColumnName,         //%s
    }eCode;
    //-------------------------------------------------//
    CGCursorEx(eCode);
    CGCursorEx(eCode, int);
    CGCursorEx(eCode, const char *);

    long getCode();
    const char *getMsg();
 protected:
    long m_lCode;
    _bstr_t m_sMsg;
};
