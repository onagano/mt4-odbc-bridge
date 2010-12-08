//-[stdhead]-------------------------------------------------------
//Project:
//File   :excpt.h
//Started:19.09.02 12:30:50
//Updated:01.10.02 08:06:55
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
class gsodbcexp CGOdbcEx
{
 public:
    typedef enum
    {
        eTypeIsIncompatible = 1,
        eIndexOutOfRange,
        eFieldNotFound,
        eAllParameterAlreadyBound,
    }UsageCode;

    CGOdbcEx(bool bUsage, long lCode, const char *szMsg = 0);
    bool isUsage();
    long getCode();
    const char *getMsg();
 protected:
    bool m_bUsage;
    long m_lCode;
    _bstr_t m_sMsg;
};
