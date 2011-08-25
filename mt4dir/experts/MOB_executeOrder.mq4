//+------------------------------------------------------------------+
//|                                             MOB_executeOrder.mq4 |
//|                                                     Osamu Nagano |
//|                      https://profiles.google.com/onagano.g/about |
//+------------------------------------------------------------------+
#property copyright "Osamu Nagano"
#property link      "https://profiles.google.com/onagano.g/about"

#import "mt4odbcbridge.ex4"
void     mob_create();
int      mob_open(string dsn, string username, string password);
int      mob_close();
int      mob_isDead();
int		mob_commit();
int		mob_rollback();
int		mob_getAutoCommit();
int		mob_setAutoCommit(int autoCommit);
int      mob_execute(string sql);
int      mob_getLastErrorNo();
string   mob_getLastErrorMesg();
int      mob_registerStatement(string sql);
int      mob_unregisterStatement(int stmtId);
int		mob_bindIntParameter(int stmtId, int slot, int intp[]);
int		mob_bindDoubleParameter(int stmtId, int slot, double dblp[]);
int		mob_bindStringParameter(int stmtId, int slot, string strp);
int		mob_bindDatetimeParameter(int stmtId, int slot, datetime dtp[]);
int		mob_bindIntColumn(int stmtId, int slot, int intp[]);
int		mob_bindDoubleColumn(int stmtId, int slot, double dblp[]);
int		mob_bindStringColumn(int stmtId, int slot, string strp);
int		mob_bindDatetimeColumn(int stmtId, int slot, datetime dtp[]);
int      mob_executeStatement(int stmtId);
int      mob_selectToFetch(int stmtId);
int      mob_fetch(int stmtId);
int      mob_fetchedAll(int stmtId);
int      mob_insertTick(int stmtId, datetime dt, int fraction, double vals[]);
int      mob_insertBar(int stmtId, datetime dt, double vals[]);
int      mob_copyRates(int stmtId, double rates[][6], int size, int start, int end);
double   mob_selectDouble(string sql, double defaultVal);
int      mob_selectInt(string sql, int defaultVal);
datetime mob_selectDatetime(string sql, datetime defaultVal);
int      createTableIfNotExists(string tableName, string sql);
string   getPeriodSymbol(int period_XX);
double   getPipSize(string symbol);
datetime mob_time();
int      mob_strcpy(string dest, int size, string src);
#import "stdlib.ex4"
string   ErrorDescription(int error_code);
#import

#define OPE_SEND           1
#define OPE_CLOSE          2
#define OPE_CLOSEBY        3
#define OPE_DELETE         4
#define OPE_MODIFY         5
#define OPE_DUMP_TRADES    6
#define OPE_DUMP_HISTORY   7
#define OPE_GET_ACCOUNT    8
#define OPE_GET_TRADE      9
#define OPE_COPY_RATES     10

#define ORD_DEFAULT_SLIPPAGE  3
#define MAX_ORDER_SIZE        100

//--- input parameters
extern string    ExtDataSourceName = "fxddx";
extern string    ExtUsername       = "";
extern string    ExtPassword       = "";
extern string    ExtTablePrefix    = "MOB_";
extern bool      ExtDebugPrint     = false;

//--- global variables
string orderTable       = "ORDER";
int    selectOrderStmt  = 0;
int    updateOrderStmt  = 0;

int      ordId[1];
int      ordOperation[1];
int      ordTicket[1];
int      ordOppositeTicket[1];
string   ordSymbol = "A234567890";
int      ordSymbolSize;
int      ordType[1];
double   ordLots[1];
double   ordOpenPrice[1];
double   ordClosePrice[1];
int      ordSlippage[1];
double   ordStopLoss[1];
double   ordTakeProfit[1];
string   ordComment = "B2345678901234567890123456789012345678901234567890123456789012345678901234567890";
int      ordCommentSize;
int      ordMagicNumber[1];
datetime ordExpiration[1];
color    ordArrowColor[1];
int      ordErrorNumber[1];
string   ordErrorMessage = "C2345678901234567890123456789012345678901234567890123456789012345678901234567890";
int      ordErrorMessageSize;

string tradeTable          = "TRADE";
int    insertTradeStmt     = 0;
int    updateTradeStmt     = 0;
int    deleteTradeStmt     = 0;
string historyTable        = "HISTORY";
int    insertHistoryStmt   = 0;
int    deleteHistoryStmt   = 0;

// 0: ticket, 1: type, 2: close_time
int  tradeList[MAX_ORDER_SIZE][3];
int  tradeListSize = 0;

int      trdTicket[1];
datetime trdOpenTime[1];
int      trdType[1];
double   trdLots[1];
string   trdSymbol = "D234567890";
int      trdSymbolSize;
double   trdOpenPrice[1];
double   trdStopLoss[1];
double   trdTakeProfit[1];
datetime trdCloseTime[1];
double   trdClosePrice[1];
double   trdCommission[1];
double   trdSwap[1];
double   trdProfit[1];
datetime trdExpiration[1];
int      trdMagicNumber[1];
string   trdComment = "E2345678901234567890123456789012345678901234567890123456789012345678901234567890";
int      trdCommentSize;

string accountTable     = "ACCOUNT";
int insertAccountStmt   = 0;

datetime accServerTime[1];
double   accBalance[1];
double   accEquity[1];
double   accMargin[1];
double   accFreeMargin[1];
double   accProfit[1];
int      accOpenTrades[1];
int      accClosedTrades[1];

//+------------------------------------------------------------------+
//| expert initialization function                                   |
//+------------------------------------------------------------------+
int init()
  {
   if (!IsDllsAllowed()) {
      Alert("ERROR: [Allow DLL imports] NOT Checked.");
      return (-1);
   }
//----
   mob_create();
   int rc;
   
   rc = mob_open(ExtDataSourceName, ExtUsername, ExtPassword);
   if (rc < 0) return (rc);

   rc = mob_setAutoCommit(true);
   if (rc < 0) return (rc);
   
   // Preparing order table
   orderTable = ExtTablePrefix + orderTable;
   rc = createTableIfNotExists(orderTable, createOrderTableSQL(orderTable));
   if (rc < 0) return (rc);

   // Regstering select order statement   
   rc = mob_registerStatement(selectOrderSQL(orderTable));
   if (rc < 0) return (rc);
   selectOrderStmt = rc;

   int i = 1;
   mob_bindIntColumn       (selectOrderStmt, i, ordId);              i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordOperation);       i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordTicket);          i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordOppositeTicket);  i++;
   mob_bindStringColumn    (selectOrderStmt, i, ordSymbol);          i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordType);            i++;
   mob_bindDoubleColumn    (selectOrderStmt, i, ordLots);            i++;
   mob_bindDoubleColumn    (selectOrderStmt, i, ordOpenPrice);       i++;
   mob_bindDoubleColumn    (selectOrderStmt, i, ordClosePrice);      i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordSlippage);        i++;
   mob_bindDoubleColumn    (selectOrderStmt, i, ordStopLoss);        i++;
   mob_bindDoubleColumn    (selectOrderStmt, i, ordTakeProfit);      i++;
   mob_bindStringColumn    (selectOrderStmt, i, ordComment);         i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordMagicNumber);     i++;
   mob_bindDatetimeColumn  (selectOrderStmt, i, ordExpiration);      i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordArrowColor);      i++;
   mob_bindIntColumn       (selectOrderStmt, i, ordErrorNumber);     i++;
   mob_bindStringColumn    (selectOrderStmt, i, ordErrorMessage);    i++;

   // Registering update order statement
   rc = mob_registerStatement(updateOrderSQL(orderTable));
   if (rc < 0) return (rc);
   updateOrderStmt = rc;

   i = 1;
   mob_bindIntParameter       (updateOrderStmt, i, ordOperation);       i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordTicket);          i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordOppositeTicket);  i++;
   mob_bindStringParameter    (updateOrderStmt, i, ordSymbol);          i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordType);            i++;
   mob_bindDoubleParameter    (updateOrderStmt, i, ordLots);            i++;
   mob_bindDoubleParameter    (updateOrderStmt, i, ordOpenPrice);       i++;
   mob_bindDoubleParameter    (updateOrderStmt, i, ordClosePrice);      i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordSlippage);        i++;
   mob_bindDoubleParameter    (updateOrderStmt, i, ordStopLoss);        i++;
   mob_bindDoubleParameter    (updateOrderStmt, i, ordTakeProfit);      i++;
   mob_bindStringParameter    (updateOrderStmt, i, ordComment);         i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordMagicNumber);     i++;
   mob_bindDatetimeParameter  (updateOrderStmt, i, ordExpiration);      i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordArrowColor);      i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordErrorNumber);     i++;
   mob_bindStringParameter    (updateOrderStmt, i, ordErrorMessage);    i++;
   mob_bindIntParameter       (updateOrderStmt, i, ordId);              i++;
   
   // Storing string buffer size for order table
   ordSymbolSize = StringLen(ordSymbol);
   ordCommentSize = StringLen(ordComment);
   ordErrorMessageSize = StringLen(ordErrorMessage);

   // Preparing trade table
   tradeTable = ExtTablePrefix + tradeTable;
   rc = createTableIfNotExists(tradeTable, createTradeTableSQL(tradeTable));
   if (rc < 0) return (rc);

   // Registering insert trade statement
   rc = mob_registerStatement(insertTradeSQL(tradeTable));
   if (rc < 0) return (rc);
   insertTradeStmt = rc;

   i = 1;
   mob_bindIntParameter       (insertTradeStmt, i, trdTicket);       i++;
   mob_bindDatetimeParameter  (insertTradeStmt, i, trdOpenTime);     i++;
   mob_bindIntParameter       (insertTradeStmt, i, trdType);         i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdLots);         i++;
   mob_bindStringParameter    (insertTradeStmt, i, trdSymbol);       i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdOpenPrice);    i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdStopLoss);     i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdTakeProfit);   i++;
   mob_bindDatetimeParameter  (insertTradeStmt, i, trdCloseTime);    i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdClosePrice);   i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdCommission);   i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdSwap);         i++;
   mob_bindDoubleParameter    (insertTradeStmt, i, trdProfit);       i++;
   mob_bindDatetimeParameter  (insertTradeStmt, i, trdExpiration);   i++;
   mob_bindIntParameter       (insertTradeStmt, i, trdMagicNumber);  i++;
   mob_bindStringParameter    (insertTradeStmt, i, trdComment);      i++;

   // Registering update trade statement
   rc = mob_registerStatement(updateTradeSQL(tradeTable));
   if (rc < 0) return (rc);
   updateTradeStmt = rc;

   i = 1;
   mob_bindDatetimeParameter  (updateTradeStmt, i, trdOpenTime);     i++;
   mob_bindIntParameter       (updateTradeStmt, i, trdType);         i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdLots);         i++;
   mob_bindStringParameter    (updateTradeStmt, i, trdSymbol);       i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdOpenPrice);    i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdStopLoss);     i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdTakeProfit);   i++;
   mob_bindDatetimeParameter  (updateTradeStmt, i, trdCloseTime);    i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdClosePrice);   i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdCommission);   i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdSwap);         i++;
   mob_bindDoubleParameter    (updateTradeStmt, i, trdProfit);       i++;
   mob_bindDatetimeParameter  (updateTradeStmt, i, trdExpiration);   i++;
   mob_bindIntParameter       (updateTradeStmt, i, trdMagicNumber);  i++;
   mob_bindStringParameter    (updateTradeStmt, i, trdComment);      i++;
   mob_bindIntParameter       (updateTradeStmt, i, trdTicket);       i++;

   // Registering delete trade statement
   rc = mob_registerStatement(deleteTradeSQL(tradeTable));
   if (rc < 0) return (rc);
   deleteTradeStmt = rc;

   i = 1;
   mob_bindIntParameter       (deleteTradeStmt, i, ordTicket);       i++;

   // Storing string buffer size for trade table
   trdSymbolSize = StringLen(trdSymbol);
   trdCommentSize = StringLen(trdComment);

   // Preparing history table
   historyTable = ExtTablePrefix + historyTable;
   rc = createTableIfNotExists(historyTable, createTradeTableSQL(historyTable));
   if (rc < 0) return (rc);

   // Registering insert history statement
   rc = mob_registerStatement(insertTradeSQL(historyTable));
   if (rc < 0) return (rc);
   insertHistoryStmt = rc;

   i = 1;
   mob_bindIntParameter       (insertHistoryStmt, i, trdTicket);       i++;
   mob_bindDatetimeParameter  (insertHistoryStmt, i, trdOpenTime);     i++;
   mob_bindIntParameter       (insertHistoryStmt, i, trdType);         i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdLots);         i++;
   mob_bindStringParameter    (insertHistoryStmt, i, trdSymbol);       i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdOpenPrice);    i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdStopLoss);     i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdTakeProfit);   i++;
   mob_bindDatetimeParameter  (insertHistoryStmt, i, trdCloseTime);    i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdClosePrice);   i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdCommission);   i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdSwap);         i++;
   mob_bindDoubleParameter    (insertHistoryStmt, i, trdProfit);       i++;
   mob_bindDatetimeParameter  (insertHistoryStmt, i, trdExpiration);   i++;
   mob_bindIntParameter       (insertHistoryStmt, i, trdMagicNumber);  i++;
   mob_bindStringParameter    (insertHistoryStmt, i, trdComment);      i++;

   // Registering delete history statement
   rc = mob_registerStatement(deleteTradeSQL(historyTable));
   if (rc < 0) return (rc);
   deleteHistoryStmt = rc;

   i = 1;
   mob_bindIntParameter       (deleteHistoryStmt, i, ordTicket);       i++;

   // Preparing account table
   accountTable = ExtTablePrefix + accountTable;
   rc = createTableIfNotExists(accountTable, createAccountTableSQL(accountTable));
   if (rc < 0) return (rc);

   // Registering insert account statement
   rc = mob_registerStatement(insertAccountSQL(accountTable));
   if (rc < 0) return (rc);
   insertAccountStmt = rc;

   i = 1;
   mob_bindDatetimeParameter  (insertAccountStmt, i, accServerTime);    i++;
   mob_bindDoubleParameter    (insertAccountStmt, i, accBalance);       i++;
   mob_bindDoubleParameter    (insertAccountStmt, i, accEquity);        i++;
   mob_bindDoubleParameter    (insertAccountStmt, i, accMargin);        i++;
   mob_bindDoubleParameter    (insertAccountStmt, i, accFreeMargin);    i++;
   mob_bindDoubleParameter    (insertAccountStmt, i, accProfit);        i++;
   mob_bindIntParameter       (insertAccountStmt, i, accOpenTrades);    i++;
   mob_bindIntParameter       (insertAccountStmt, i, accClosedTrades);  i++;

   // Move closed trades from trade table to history table
   string whereClause = " where close_time > '1970-01-01 00:00:00'";
   mob_setAutoCommit(false);
   rc = mob_execute("delete from " + historyTable
      + " where ticket in (select ticket from " + tradeTable + whereClause + ")");
   if (rc < 0) { mob_rollback(); mob_setAutoCommit(true); return (rc); }
   rc = mob_execute("insert into " + historyTable + " select * from " + tradeTable + whereClause);
   if (rc < 0) { mob_rollback(); mob_setAutoCommit(true); return (rc); }
   rc = mob_execute("delete from " + tradeTable + whereClause);
   if (rc < 0) { mob_rollback(); mob_setAutoCommit(true); return (rc); }
   mob_commit();
   mob_setAutoCommit(true);

   // To sync trades first time
   tradeListSize = 0;
//----
   return(0);
  }
//+------------------------------------------------------------------+
//| expert deinitialization function                                 |
//+------------------------------------------------------------------+
int deinit()
  {
//----
   int rc = mob_close();
   if (rc < 0) return (rc);
//----
   return(0);
  }
//+------------------------------------------------------------------+
//| expert start function                                            |
//+------------------------------------------------------------------+
int start()
  {
//----
   int rc = 0;
   
   rc = selectOrders();
   if (rc < 0) return (rc);

   rc = syncTrades();
   if (rc < 0) return (rc);
//----
   return(0);
  }
//+------------------------------------------------------------------+

string createOrderTableSQL(string tableName) {
   // MySQL has AUTO_INCREMENT option at column definition,
   // but PostgreSQL needs SEQUENCE object...
   string opeIndName = tableName + "_OPEIND";
   string sql = ""
      + "create table " + tableName + " ("
      + "  id integer identity"
      + ", operation integer default 0"
      + ", ticket integer default -1"
      + ", opposite_ticket integer default -1"
      + ", symbol varchar(10) default ''"
      + ", type integer default -1"
      + ", lots double precision default 0.0"
      + ", open_price double precision default 0.0"
      + ", close_price double precision default 0.0"
      + ", slippage integer default " + ORD_DEFAULT_SLIPPAGE
      + ", stop_loss double precision default 0.0"
      + ", take_profit double precision default 0.0"
      + ", comment varchar(140) default ''"
      + ", magic_number integer default 0"
      + ", expiration timestamp default '1970-01-01 00:00:00'"
      + ", arrow_color integer default " + CLR_NONE
      + ", error_number integer default 0"
      + ", error_message varchar(140) default ''"
      + ");"
      + "drop index if exists " + opeIndName + ";"
      + "create index " + opeIndName + " on " + tableName + " (operation)";
   return (sql);
}

string selectOrderSQL(string tableName) {
   string sql = "select"
      + "  id"
      + ", operation"
      + ", ticket"
      + ", opposite_ticket"
      + ", symbol"
      + ", type"
      + ", lots"
      + ", open_price"
      + ", close_price"
      + ", slippage"
      + ", stop_loss"
      + ", take_profit"
      + ", comment"
      + ", magic_number"
      + ", expiration"
      + ", arrow_color"
      + ", error_number"
      + ", error_message"
      + " from " + tableName + " where "
      + "operation > 0 order by id";
   return (sql);
}

string updateOrderSQL(string tableName) {
   string sql = "update " + tableName + " set"
      + "  operation = ?"
      + ", ticket = ?"
      + ", opposite_ticket = ?"
      + ", symbol = ?"
      + ", type = ?"
      + ", lots = ?"
      + ", open_price = ?"
      + ", close_price = ?"
      + ", slippage = ?"
      + ", stop_loss = ?"
      + ", take_profit = ?"
      + ", comment = ?"
      + ", magic_number = ?"
      + ", expiration = ?"
      + ", arrow_color = ?"
      + ", error_number = ?"
      + ", error_message = ?"
      + " where id = ?";
   return (sql);
}

void bindOrderParameters() {
   ordTicket[0]      = OrderTicket();
   ordType[0]        = OrderType();
   ordLots[0]        = OrderLots();
   mob_strcpy(ordSymbol, ordSymbolSize, OrderSymbol());
   ordOpenPrice[0]   = OrderOpenPrice();
   ordStopLoss[0]    = OrderStopLoss();
   ordTakeProfit[0]  = OrderTakeProfit();
   ordClosePrice[0]  = OrderClosePrice();
   ordExpiration[0]  = OrderExpiration();
   ordMagicNumber[0] = OrderMagicNumber();
   mob_strcpy(ordComment, ordCommentSize, OrderComment());
}

int selectOrders() {
   int rc = 0;

   rc = mob_selectToFetch(selectOrderStmt);
   if (rc < 0) return (rc);
   
   while (mob_fetch(selectOrderStmt) != 0) {
   
      switch (ordOperation[0]) {
      case OPE_SEND:
         orderSend();
         break;
      case OPE_CLOSE:
         orderClose();
         break;
      case OPE_CLOSEBY:
         orderCloseBy();
         break;
      case OPE_DELETE:
         orderDelete();
         break;
      case OPE_MODIFY:
         orderModify();
         break;
      case OPE_DUMP_TRADES:
         dumpTradesInTransaction(false);
         break;
      case OPE_DUMP_HISTORY:
         dumpTradesInTransaction(true);
         break;
      case OPE_GET_ACCOUNT:
         insertAccount();
         break;
      case OPE_GET_TRADE:
         getTradeInTransaction();
         break;
      case OPE_COPY_RATES:
         copyRates();
         break;
      default:
         setServerResponse(-1, "Unknown operation: " + ordOperation[0]);
      }
      
      ordOperation[0] = - ordOperation[0];
      if (ordTicket[0] > 0) {
         OrderSelect(ordTicket[0], SELECT_BY_TICKET);
         bindOrderParameters();
      }
      rc = mob_executeStatement(updateOrderStmt);
      if (rc < 0) {
         mob_fetchedAll(selectOrderStmt);
         return (rc);
      }
      Print("ID #", ordId[0],
         " Operation #", - ordOperation[0], " on ticket #", ordTicket[0],
         " has been done with #", ordErrorNumber[0], " (", ordErrorMessage, ")");
   }
   
   mob_fetchedAll(selectOrderStmt);
   
   return (rc);
}

void setServerResponse(int no, string mesg) {
   ordErrorNumber[0] = no;
   mob_strcpy(ordErrorMessage, ordErrorMessageSize, mesg);
}

void getServerResponse() {
   int err = GetLastError();
   string errMesg = ErrorDescription(err);
   setServerResponse(err, errMesg);
}

void getDatabaseResponse() {
   int err = mob_getLastErrorNo();
   string errMesg = mob_getLastErrorMesg();
   setServerResponse(err, errMesg);
}

void orderSend() {
   double margin;
   double thePip    = getPipSize(ordSymbol);
   int    theDigits = MarketInfo(ordSymbol, MODE_DIGITS);
   
   // open at market price
   if (ordOpenPrice[0] == 0.0) {
      switch (ordType[0]) {
      case OP_BUY:
         ordOpenPrice[0] = MarketInfo(ordSymbol, MODE_ASK);
         break;
      case OP_SELL:
         ordOpenPrice[0] = MarketInfo(ordSymbol, MODE_BID);
         break;
      }
   }
   else if (ordOpenPrice[0] < 0.0) {
      // relative limit/stop open price
      margin = MathAbs(ordOpenPrice[0]) * thePip;
      switch (ordType[0]) {
      case OP_BUYLIMIT:
         ordOpenPrice[0] = NormalizeDouble(MarketInfo(ordSymbol, MODE_ASK) - margin, theDigits);
         break;
      case OP_SELLLIMIT:
         ordOpenPrice[0] = NormalizeDouble(MarketInfo(ordSymbol, MODE_BID) + margin, theDigits);
         break;
      case OP_BUYSTOP:
         ordOpenPrice[0] = NormalizeDouble(MarketInfo(ordSymbol, MODE_ASK) + margin, theDigits);
         break;
      case OP_SELLSTOP:
         ordOpenPrice[0] = NormalizeDouble(MarketInfo(ordSymbol, MODE_BID) - margin, theDigits);
         break;
      }
   }
   
   if (ordStopLoss[0] < 0.0) {
      // relative stop loss
      margin = MathAbs(ordStopLoss[0]) * thePip;
      switch (ordType[0]) {
      case OP_BUY:
      case OP_BUYLIMIT:
      case OP_BUYSTOP:
         ordStopLoss[0] = NormalizeDouble(ordOpenPrice[0] - margin, theDigits);
         break;
      case OP_SELL:
      case OP_SELLLIMIT:
      case OP_SELLSTOP:
         ordStopLoss[0] = NormalizeDouble(ordOpenPrice[0] + margin, theDigits);
         break;
      }
   }
   
   if (ordTakeProfit[0] < 0.0) {
      // relative take profit
      margin = MathAbs(ordTakeProfit[0]) * thePip;
      switch (ordType[0]) {
      case OP_BUY:
      case OP_BUYLIMIT:
      case OP_BUYSTOP:
         ordTakeProfit[0] = NormalizeDouble(ordOpenPrice[0] + margin, theDigits);
         break;
      case OP_SELL:
      case OP_SELLLIMIT:
      case OP_SELLSTOP:
         ordTakeProfit[0] = NormalizeDouble(ordOpenPrice[0] - margin, theDigits);
         break;
      }
   }
   
   if (ExtDebugPrint) Print("orderSend: ",
      ordSymbol, ", ",
      ordType[0], ", ",
      ordLots[0], ", ",
      ordOpenPrice[0], ", ",
      ordSlippage[0], ", ",
      ordStopLoss[0], ", ",
      ordTakeProfit[0], ", ",
      ordComment, ", ",
      ordMagicNumber[0], ", ",
      TimeToStr(ordExpiration[0]), ", ",
      ordArrowColor[0]
   );
   
   // ordering
   int ticket = OrderSend(
      ordSymbol,
      ordType[0],
      ordLots[0],
      ordOpenPrice[0],
      ordSlippage[0],
      ordStopLoss[0],
      ordTakeProfit[0],
      ordComment,
      ordMagicNumber[0],
      ordExpiration[0],
      ordArrowColor[0]
   );
   
   if (ticket == -1) {
      getServerResponse();
   }
   ordTicket[0] = ticket;
}

void orderClose() {
   if (OrderSelect(ordTicket[0], SELECT_BY_TICKET)) {
      // close at market price
      if (ordClosePrice[0] == 0.0) {
         switch (OrderType()) {
         case OP_BUY:
            ordClosePrice[0] = MarketInfo(OrderSymbol(), MODE_BID);
            break;
         case OP_SELL:
            ordClosePrice[0] = MarketInfo(OrderSymbol(), MODE_ASK);
            break;
         }
      }
      // close all position
      if (ordLots[0] == 0.0) {
         ordLots[0] = OrderLots();
      }
      
      if (ExtDebugPrint) Print("orderClose: ",
         ordTicket[0], ", ",
         ordLots[0], ", ",
         ordClosePrice[0], ", ",
         ordSlippage[0], ", ",
         ordArrowColor[0]
      );
      
      // closing
      if (OrderClose(
         ordTicket[0],
         ordLots[0],
         ordClosePrice[0],
         ordSlippage[0],
         ordArrowColor[0]))
      {
      } else {
         getServerResponse();
      }
   } else { // cannot select order
      getServerResponse();
   }
}

void orderCloseBy() {
   if (ExtDebugPrint) Print("orderCloseBy: ",
      ordTicket[0], ", ",
      ordOppositeTicket[0], ", ",
      ordArrowColor[0]
   );
   
   if (OrderCloseBy(
      ordTicket[0],
      ordOppositeTicket[0],
      ordArrowColor[0]))
   {
   } else {
      getServerResponse();
   }
}

void orderDelete() {
   if (ExtDebugPrint) Print("orderDelete: ",
      ordTicket[0], ", ",
      ordArrowColor[0]
   );
   
   if (OrderDelete(
      ordTicket[0],
      ordArrowColor[0]))
   {
   } else {
      getServerResponse();
   }
}

void orderModify() {
   if (OrderSelect(ordTicket[0], SELECT_BY_TICKET)) {
      double margin;
      double thePip    = getPipSize(OrderSymbol());
      int    theDigits = MarketInfo(OrderSymbol(), MODE_DIGITS);
      
      if (ordOpenPrice[0] == 0.0) {
         ordOpenPrice[0] = OrderOpenPrice();
      }
      else if (ordOpenPrice[0] < 0.0) {
         // relative open price
         margin = MathAbs(ordOpenPrice[0]) * thePip;
         switch (OrderType()) {
         case OP_BUYLIMIT:
            ordOpenPrice[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_ASK) - margin, theDigits);
            break;
         case OP_SELLLIMIT:
            ordOpenPrice[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_BID) + margin, theDigits);
            break;
         case OP_BUYSTOP:
            ordOpenPrice[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_ASK) + margin, theDigits);
            break;
         case OP_SELLSTOP:
            ordOpenPrice[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_BID) - margin, theDigits);
            break;
         default:
            ordOpenPrice[0] = OrderOpenPrice();
         }
      }
      
      if (ordStopLoss[0] == 0.0) {
         ordStopLoss[0] = OrderStopLoss();
      }
      else if (ordStopLoss[0] < 0.0) {
         // relative stop loss
         margin = MathAbs(ordStopLoss[0]) * thePip;
         switch (OrderType()) {
         case OP_BUY:
            ordStopLoss[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_BID) - margin, theDigits);
            break;
         case OP_BUYLIMIT:
         case OP_BUYSTOP:
            ordStopLoss[0] = NormalizeDouble(ordOpenPrice[0] - margin, theDigits);
            break;
         case OP_SELL:
            ordStopLoss[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_ASK) + margin, theDigits);
            break;
         case OP_SELLLIMIT:
         case OP_SELLSTOP:
            ordStopLoss[0] = NormalizeDouble(ordOpenPrice[0] + margin, theDigits);
            break;
         }
      }
      
      if (ordTakeProfit[0] == 0.0) {
         ordTakeProfit[0] = OrderTakeProfit();
      }
      else if (ordTakeProfit[0] < 0.0) {
         // relative take profit
         margin = MathAbs(ordTakeProfit[0]) * thePip;
         switch (OrderType()) {
         case OP_BUY:
            ordTakeProfit[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_BID) + margin, theDigits);
            break;
         case OP_BUYLIMIT:
         case OP_BUYSTOP:
            ordTakeProfit[0] = NormalizeDouble(ordOpenPrice[0] + margin, theDigits);
            break;
         case OP_SELL:
            ordTakeProfit[0] = NormalizeDouble(MarketInfo(OrderSymbol(), MODE_ASK) - margin, theDigits);
            break;
         case OP_SELLLIMIT:
         case OP_SELLSTOP:
            ordTakeProfit[0] = NormalizeDouble(ordOpenPrice[0] - margin, theDigits);
            break;
         }
      }
      
      if (ordExpiration[0] == 0) {
         ordExpiration[0] = OrderExpiration();
      }
      
      if (ExtDebugPrint) Print("orderModify: ",
         ordTicket[0], ", ",
         ordOpenPrice[0], ", ",
         ordStopLoss[0], ", ",
         ordTakeProfit[0], ", ",
         TimeToStr(ordExpiration[0]), ", ",
         ordArrowColor[0]
      );
      
      // modifying
      if (OrderModify(
         ordTicket[0],
         ordOpenPrice[0],
         ordStopLoss[0],
         ordTakeProfit[0],
         ordExpiration[0],
         ordArrowColor[0]))
      {
      } else {
         getServerResponse();
      }
   } else { // cannot select order
      getServerResponse();
   }
}

string createTradeTableSQL(string tableName) {
   string sql = "create table " + tableName + " ("
      + "  ticket integer primary key"
      + ", open_time timestamp default '1970-01-01 00:00:00'"
      + ", type integer default -1"
      + ", lots double precision default 0.0"
      + ", symbol varchar(10) default ''"
      + ", open_price double precision default 0.0"
      + ", stop_loss double precision default 0.0"
      + ", take_profit double precision default 0.0"
      + ", close_time timestamp default '1970-01-01 00:00:00'"
      + ", close_price double precision default 0.0"
      + ", commission double precision default 0.0"
      + ", swap double precision default 0.0"
      + ", profit double precision default 0.0"
      + ", expiration timestamp default '1970-01-01 00:00:00'"
      + ", magic_number integer default 0"
      + ", comment varchar(140) default ''"
      + ")";
   return (sql);
}

string deleteTradeSQL(string tableName) {
   string sql = "delete from " + tableName + " where ticket = ?";
   return (sql);
}

string insertTradeSQL(string tableName) {
   string sql = "insert into " + tableName + " ("
      + "  ticket"
      + ", open_time"
      + ", type"
      + ", lots"
      + ", symbol"
      + ", open_price"
      + ", stop_loss"
      + ", take_profit"
      + ", close_time"
      + ", close_price"
      + ", commission"
      + ", swap"
      + ", profit"
      + ", expiration"
      + ", magic_number"
      + ", comment"
      + ") values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
   return (sql);
}

string updateTradeSQL(string tableName) {
   string sql = "update " + tableName + " set"
      + "  open_time = ?"
      + ", type = ?"
      + ", lots = ?"
      + ", symbol = ?"
      + ", open_price = ?"
      + ", stop_loss = ?"
      + ", take_profit = ?"
      + ", close_time = ?"
      + ", close_price = ?"
      + ", commission = ?"
      + ", swap = ?"
      + ", profit = ?"
      + ", expiration = ?"
      + ", magic_number = ?"
      + ", comment = ?"
      + " where ticket = ?";
   return (sql);
}

void bindTradeParameters() {
   trdTicket[0]      = OrderTicket();
   trdOpenTime[0]    = OrderOpenTime();
   trdType[0]        = OrderType();
   trdLots[0]        = OrderLots();
   mob_strcpy(trdSymbol, trdSymbolSize, OrderSymbol());
   trdOpenPrice[0]   = OrderOpenPrice();
   trdStopLoss[0]    = OrderStopLoss();
   trdTakeProfit[0]  = OrderTakeProfit();
   trdCloseTime[0]   = OrderCloseTime();
   trdClosePrice[0]  = OrderClosePrice();
   trdCommission[0]  = OrderCommission();
   trdSwap[0]        = OrderSwap();
   trdProfit[0]      = OrderProfit();
   trdExpiration[0]  = OrderExpiration();
   trdMagicNumber[0] = OrderMagicNumber();
   mob_strcpy(trdComment, trdCommentSize, OrderComment());
}

void dumpTradeParameters(string mesg) {
   Print(mesg, ": ",
      trdTicket[0], ", ",
      TimeToStr(trdOpenTime[0]), ", ",
      trdType[0], ", ",
      trdLots[0], ", ",
      trdSymbol, ", ",
      trdOpenPrice[0], ", ",
      trdStopLoss[0], ", ",
      trdTakeProfit[0], ", ",
      TimeToStr(trdCloseTime[0]), ", ",
      trdClosePrice[0], ", ",
      trdCommission[0], ", ",
      trdSwap[0], ", ",
      trdProfit[0], ", ",
      TimeToStr(trdExpiration[0]), ", ",
      trdMagicNumber[0], ", ",
      trdComment
   );
}

int dumpTrades(bool history) {
   int rc = 0;
   int deleteStmt;
   int insertStmt;
   int n;
   int pool;

   if (history) {
      deleteStmt = deleteHistoryStmt;
      insertStmt = insertHistoryStmt;
      n = OrdersHistoryTotal();
      pool = MODE_HISTORY;
   } else {
      deleteStmt = deleteTradeStmt;
      insertStmt = insertTradeStmt;
      n = OrdersTotal();
      pool = MODE_TRADES;
   }

   for (int i = 0; i < n; i++) {
      if (OrderSelect(i, SELECT_BY_POS, pool)) {
         bindTradeParameters();
         if (ExtDebugPrint) dumpTradeParameters("dumpTrades-" + history);
         ordTicket[0] = trdTicket[0];
         mob_executeStatement(deleteStmt); // ignore result
         rc = mob_executeStatement(insertStmt);
         if (rc < 0) {
            getDatabaseResponse();
            return (rc);
         }
      } else {
         getServerResponse();
      }
   }
   
   return (rc);
}

void dumpTradesInTransaction(bool history) {
   mob_setAutoCommit(false);
   int rc = dumpTrades(history);
   if (rc < 0) {
      mob_rollback();
   } else {
      mob_commit();
   }
   mob_setAutoCommit(true);
}

int getTrade() {
   int rc = 0;
   mob_executeStatement(deleteTradeStmt); // ignore result
   if (OrderSelect(ordTicket[0], SELECT_BY_TICKET)) {
      bindTradeParameters();
      if (ExtDebugPrint) dumpTradeParameters("getTrade");
      rc = mob_executeStatement(insertTradeStmt);
      if (rc < 0) {
         getDatabaseResponse();
         return (rc);
      }
   } else {
      getServerResponse();
   }
   return (rc);
}

void getTradeInTransaction() {
   mob_setAutoCommit(false);
   int rc = getTrade();
   if (rc < 0) {
      mob_rollback();
   } else {
      mob_commit();
   }
   mob_setAutoCommit(true);
}

string createAccountTableSQL(string tableName) {
   string sql = ""
      + "create table " + tableName + " ("
      + "  id integer identity"
      + ", server_time timestamp default '1970-01-01 00:00:00'"
      + ", balance double precision"
      + ", equity double precision"
      + ", margin double precision"
      + ", free_margin double precision"
      + ", profit double precision"
      + ", open_trades integer"
      + ", closed_trades integer"
      + ")";
   return (sql);
}

string insertAccountSQL(string tableName) {
   string sql = "insert into " + tableName + " ("
      + "  server_time"
      + ", balance"
      + ", equity"
      + ", margin"
      + ", free_margin"
      + ", profit"
      + ", open_trades"
      + ", closed_trades"
      + ") values (?, ?, ?, ?, ?, ?, ?, ?)";
   return (sql);
}

void insertAccount() {
   accServerTime[0]     = TimeCurrent();
   accBalance[0]        = AccountBalance();
   accEquity[0]         = AccountEquity();
   accMargin[0]         = AccountMargin();
   accFreeMargin[0]     = AccountFreeMargin();
   accProfit[0]         = AccountProfit();
   accOpenTrades[0]     = OrdersTotal();
   accClosedTrades[0]   = OrdersHistoryTotal();

   int rc = mob_executeStatement(insertAccountStmt);
   if (rc < 0) {
      getDatabaseResponse();
   }
}

string createBarTableSQL(string tableName) {
   string sql = "create table " + tableName + " ("
      + "  time timestamp primary key"
      + ", open double precision"
      + ", high double precision"
      + ", low double precision"
      + ", close double precision"
      + ", volume double precision"
      + ")";
   return (sql);
}

string insertBarSQL(string tableName) {
   string sql = "insert into " + tableName
      + " (time, open, high, low, close, volume) values (?, ?, ?, ?, ?, ?)";
   return (sql);
}

void copyRates() {
   string symbol = ordSymbol;
   int    period = ordType[0];
   string tableName = ExtTablePrefix + symbol + "_" + getPeriodSymbol(period);
   
   int rc = createTableIfNotExists(tableName, createBarTableSQL(tableName));
   if (rc < 0) {
      getDatabaseResponse();
      return;
   }

   int count      = mob_selectInt("select count(*) from " + tableName, -1);
   datetime dtMax = mob_selectDatetime("select max(time) from " + tableName, -1);
   datetime dtMin = mob_selectDatetime("select min(time) from " + tableName, -1);
   int start;
   int end;
   int rv;

   double rates[][6];
   int nBars = ArrayCopyRates(rates, symbol, period);
   if (nBars < 0) {
      getServerResponse();
      return;
   }

   if (count <= 0) {
      start = 0;
      end = nBars - 1; // Exclude the latest bar.
      rv = copyRatesBetween(tableName, start, end, symbol, period, rates);
   } else {
      datetime dts[];
      ArrayCopySeries(dts, MODE_TIME, symbol, period);
      int iMax = ArrayBsearch(dts, dtMax, WHOLE_ARRAY, 0 , MODE_DESCEND);
      start = nBars - iMax;
      end = nBars - 1; // Exclude the latest bar.
      rv = copyRatesBetween(tableName, start, end, symbol, period, rates);
   }
   if (rv < 0) {
      getDatabaseResponse();
      return;
   }
   
   string mesg = "Copied " + rv + " bars";
   if (rv > 0) {
      mesg = mesg + " from " + TimeToStr(rates[nBars - start - 1][0])
                  + " to "   + TimeToStr(rates[nBars - end][0]);
   }
   setServerResponse(rv, mesg);
}

int copyRatesBetween(string tableName, int start, int end, string symbol, int period, double& rates[]) {
   int rv = 0;
   
   int insertBarStmt = mob_registerStatement(insertBarSQL(tableName));
   mob_setAutoCommit(false);

   // RateInfo structure's order is OLHCV, which will be adjusted to OHLCV in database.
   rv = mob_copyRates(insertBarStmt, rates, ArraySize(rates), start, end);
   if (rv < 0) {
      mob_rollback();
   } else {
      mob_commit();
   }

   mob_setAutoCommit(true);
   mob_unregisterStatement(insertBarStmt);

   return(rv);
}

void fillTradeList() {
   tradeListSize = OrdersTotal();
   for (int i = 0; i < tradeListSize; i++) {
      OrderSelect(i, SELECT_BY_POS, MODE_TRADES);
      tradeList[i][0] = OrderTicket();
      tradeList[i][1] = OrderType();
      tradeList[i][2] = OrderCloseTime();
   }
}

bool isNewOrder(int ticket) {
   for (int i = 0; i < tradeListSize; i++) {
      if (ticket == tradeList[i][0]) return (false);
   }
   return (true);
}

int syncTrades() {
   int rc = 0;
   int i = 0;
   
   for (i = 0; i < tradeListSize; i++) {
      int ticket = tradeList[i][0];
      int odtype = tradeList[i][1];
      int closet = tradeList[i][2];
      OrderSelect(ticket, SELECT_BY_TICKET);
      // Check if closed or a pending order has been hit
      if (closet != OrderCloseTime() || odtype != OrderType()) {
         bindTradeParameters();
         if (ExtDebugPrint) dumpTradeParameters("syncTrades-close-or-hit");
         rc = mob_executeStatement(updateTradeStmt);
         if (rc < 0) return (rc);
      }
   }
   
   // Check if a new order
   int n = OrdersTotal();
   for (i = 0; i < n; i++) {
      OrderSelect(i, SELECT_BY_POS, MODE_TRADES);
      if (isNewOrder(OrderTicket())) {
         bindTradeParameters();
         if (ExtDebugPrint) dumpTradeParameters("syncTrades-new");
         ordTicket[0] = trdTicket[0];
         mob_executeStatement(deleteTradeStmt); // ignore result
         rc = mob_executeStatement(insertTradeStmt);
         if (rc < 0) return (rc);
      }
   }

   fillTradeList();
   return (rc);
}

