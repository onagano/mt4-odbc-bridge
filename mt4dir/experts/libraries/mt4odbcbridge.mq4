//+------------------------------------------------------------------+
//|                                                mt4odbcbridge.mq4 |
//|                                                     Osamu Nagano |
//|                   http://www.google.com/profiles/onagano.g#about |
//+------------------------------------------------------------------+
#property copyright "Osamu Nagano"
#property link      "http://www.google.com/profiles/onagano.g#about"
#property library

//+------------------------------------------------------------------+
//| Defines                                                          |
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
//| DLL imports                                                      |
//+------------------------------------------------------------------+
#import "MT4ODBCBridge.dll"
int      MOB_create();
int      MOB_open(int conId, string dsn, string username, string password);
int      MOB_close(int conId);
int      MOB_isDead(int conId);
int		MOB_commit(int conId);
int		MOB_rollback(int conId);
int		MOB_getAutoCommit(int conId);
int		MOB_setAutoCommit(int conId, int autoCommit);
int      MOB_execute(int conId, string sql);
int      MOB_getLastErrorNo(int conId);
string   MOB_getLastErrorMesg(int conId);
int      MOB_registerStatement(int conId, string sql);
int      MOB_unregisterStatement(int conId, int stmtId);
int		MOB_bindIntParameter(int conId, int stmtId, int slot, int intp[]);
int		MOB_bindDoubleParameter(int conId, int stmtId, int slot, double dblp[]);
int		MOB_bindStringParameter(int conId, int stmtId, int slot, string strp);
int		MOB_bindDatetimeParameter(int conId, int stmtId, int slot, datetime dtp[]);
int		MOB_bindIntColumn(int conId, int stmtId, int slot, int intp[]);
int		MOB_bindDoubleColumn(int conId, int stmtId, int slot, double dblp[]);
int		MOB_bindStringColumn(int conId, int stmtId, int slot, string strp);
int		MOB_bindDatetimeColumn(int conId, int stmtId, int slot, datetime dtp[]);
int      MOB_executeStatement(int conId, int stmtId);
int      MOB_selectToFetch(int conId, int stmtId);
int      MOB_fetch(int conId, int stmtId);
int      MOB_fetchedAll(int conId, int stmtId);
int      MOB_insertTick(int conId, int stmtId, datetime dt, int fraction, double vals[], int size);
int      MOB_insertBar(int conId, int stmtId, datetime dt, double vals[], int size);
int      MOB_copyRates(int conId, int stmtId, double rates[][6], int size, int start, int end);
double   MOB_selectDouble(int conId, string sql, double defaultVal);
int      MOB_selectInt(int conId, string sql, int defaultVal);
datetime MOB_selectDatetime(int conId, string sql, datetime defaultVal);
datetime MOB_time();
datetime MOB_localtime();
int      MOB_strcpy(string dest, int size, string src);
#import

//+------------------------------------------------------------------+
//| EX4 imports                                                      |
//+------------------------------------------------------------------+
#import "stdlib.ex4"
   string ErrorDescription(int error_code);
#import


//+------------------------------------------------------------------+
//| Global variables                                                 |
//+------------------------------------------------------------------+
int mob_conId;

//+------------------------------------------------------------------+
//| MOB wrapper functions                                            |
//+------------------------------------------------------------------+
void mob_create() {
   mob_conId = MOB_create();
}

int mob_open(string dsn, string username, string password) {
   return (checkMOBError(MOB_open(mob_conId, dsn, username, password)));
}

int mob_close() {
   return (checkMOBError(MOB_close(mob_conId)));
}

int mob_isDead() {
   return (checkMOBError(MOB_isDead(mob_conId)));
}

int mob_commit() {
   return (checkMOBError(MOB_commit(mob_conId)));
}

int mob_rollback() {
   return (checkMOBError(MOB_rollback(mob_conId)));
}

int mob_getAutoCommit() {
   return (checkMOBError(MOB_getAutoCommit(mob_conId)));
}

int mob_setAutoCommit(int autoCommit) {
   return (checkMOBError(MOB_setAutoCommit(mob_conId, autoCommit)));
}

int mob_execute(string sql) {
   return (checkMOBError(MOB_execute(mob_conId, sql)));
}

int mob_registerStatement(string sql) {
   return (checkMOBError(MOB_registerStatement(mob_conId, sql)));
}

int mob_unregisterStatement(int stmtId) {
   return (checkMOBError(MOB_unregisterStatement(mob_conId, stmtId)));
}

int mob_bindIntParameter(int stmtId, int stmtId, int slot, int intp[]) {
   return (checkMOBError(MOB_bindIntParameter(mob_conId, stmtId, slot, intp)));
}

int mob_bindDoubleParameter(int stmtId, int stmtId, int slot, double dblp[]) {
   return (checkMOBError(MOB_bindDoubleParameter(mob_conId, stmtId, slot, dblp)));
}

int mob_bindStringParameter(int stmtId, int stmtId, int slot, string strp) {
   return (checkMOBError(MOB_bindStringParameter(mob_conId, stmtId, slot, strp)));
}

int mob_bindDatetimeParameter(int stmtId, int stmtId, int slot, datetime dtp[]) {
   return (checkMOBError(MOB_bindDatetimeParameter(mob_conId, stmtId, slot, dtp)));
}

int mob_bindIntColumn(int stmtId, int stmtId, int slot, int intp[]) {
   return (checkMOBError(MOB_bindIntColumn(mob_conId, stmtId, slot, intp)));
}

int mob_bindDoubleColumn(int stmtId, int stmtId, int slot, double dblp[]) {
   return (checkMOBError(MOB_bindDoubleColumn(mob_conId, stmtId, slot, dblp)));
}

int mob_bindStringColumn(int stmtId, int stmtId, int slot, string strp) {
   return (checkMOBError(MOB_bindStringColumn(mob_conId, stmtId, slot, strp)));
}

int mob_bindDatetimeColumn(int stmtId, int stmtId, int slot, datetime dtp[]) {
   return (checkMOBError(MOB_bindDatetimeColumn(mob_conId, stmtId, slot, dtp)));
}

int mob_executeStatement(int stmtId) {
   return (checkMOBError(MOB_executeStatement(mob_conId, stmtId)));
}

int mob_selectToFetch(int stmtId) {
   return (checkMOBError(MOB_selectToFetch(mob_conId, stmtId)));
}

int mob_fetch(int stmtId) {
   return (checkMOBError(MOB_fetch(mob_conId, stmtId)));
}

int mob_fetchedAll(int stmtId) {
   return (checkMOBError(MOB_fetchedAll(mob_conId, stmtId)));
}

int mob_insertTick(int stmtId, datetime dt, int fraction, double vals[]) {
   return (checkMOBError(MOB_insertTick(mob_conId, stmtId, dt, fraction, vals, ArraySize(vals))));
}

int mob_insertBar(int stmtId, datetime dt, double vals[]) {
   return (checkMOBError(MOB_insertBar(mob_conId, stmtId, dt, vals, ArraySize(vals))));
}

int mob_copyRates(int stmtId, double rates[][6], int size, int start, int end) {
   return (checkMOBError(MOB_copyRates(mob_conId, stmtId, rates, size, start, end)));
}

int mob_getLastErrorNo() {
   return (MOB_getLastErrorNo(mob_conId));
}

string mob_getLastErrorMesg() {
   return (MOB_getLastErrorMesg(mob_conId));
}

double mob_selectDouble(string sql, double defaultVal) {
   return (MOB_selectDouble(mob_conId, sql, defaultVal));
}

int mob_selectInt(string sql, int defaultVal) {
   return (MOB_selectInt(mob_conId, sql, defaultVal));
}

datetime mob_selectDatetime(string sql, datetime defaultVal) {
   return (MOB_selectDatetime(mob_conId, sql, defaultVal));
}

datetime mob_time() {
   return (MOB_time());
}

datetime mob_localtime() {
   return (MOB_localtime());
}

int mob_strcpy(string dest, int size, string src) {
   return (MOB_strcpy(dest, size, src));
}


//+------------------------------------------------------------------+
//| Stateful helper functions                                        |
//+------------------------------------------------------------------+
int checkMOBError(int rc, bool rollback = false, bool disconnect = false) {
   if (rc < 0) {
      int eno = mob_getLastErrorNo();
      string emesg = mob_getLastErrorMesg();
      Print("MOB error #", eno, ": ", emesg);
      if (rollback) mob_rollback();
      if (disconnect) mob_close();
   }
   return (rc);
}

int createTableIfNotExists(string tableName, string sql, bool drop = false) {
   int rc = 0;

   // MySQL has 'CREATE TABLE IF NOT EXISTS ...'
   // while PostgreSQL has 'DROP TABLE IF EXISTS ...'
 
   if (drop) rc = mob_execute("drop table " + tableName);

   rc = mob_selectInt("select count(*) from " + tableName, -1);
   bool exist = 0 <= rc;
   
   if (!exist) {
      rc = mob_execute(sql);
   }
   
   return (rc);
}

//+------------------------------------------------------------------+
//| Stateless helper functions                                       |
//+------------------------------------------------------------------+
string getPeriodSymbol(int period_XX) {
   switch (period_XX) {
   case PERIOD_M1:
      return ("M1");
      break;
   case PERIOD_M5:
      return ("M5");
      break;
   case PERIOD_M15:
      return ("M15");
      break;
   case PERIOD_M30:
      return ("M30");
      break;
   case PERIOD_H1:
      return ("H1");
      break;
   case PERIOD_H4:
      return ("H4");
      break;
   case PERIOD_D1:
      return ("D1");
      break;
   case PERIOD_W1:
      return ("W1");
      break;
   case PERIOD_MN1:
      return ("MN1");
      break;
   default:
      return ("UNKNOWN");
   }
}

double getPipSize(string symbol) {
   double pip    = 0.0;
   double point  = MarketInfo(symbol, MODE_POINT);
   int    digits = MarketInfo(symbol, MODE_DIGITS);
   switch (digits) {
      case 5:
      case 3:
         pip = NormalizeDouble(point * 10, digits);
         break;
      case 4:
      case 2:
         pip = NormalizeDouble(point, digits);
         break;
   }
   return (pip);
}
//+------------------------------------------------------------------+

