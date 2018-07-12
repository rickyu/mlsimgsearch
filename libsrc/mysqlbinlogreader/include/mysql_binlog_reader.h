#ifndef _MYSQL_BINLOG_READER_H_
#define _MYSQL_BINLOG_READER_H_


#define MYSQL_CLIENT
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>

#include <string>
#include <map>
#include <list>
#include <ostream>
#include <tr1/unordered_map>


#include "mysql_operation.h"



class MysqlBinlogEventInfo {
  public:
    static const char* GetTypeStr(uint8_t type);
    time_t when_;
    uint8_t type_;
    string logname_;//< 文件名.
    uint64_t pos_;
    uint64_t data_length_;
};
class ErrorInfo {
  public:
    ErrorInfo():line_(0) {}
    int line_;
    string file_;
    string function_;
    string msg_;
};

class Table_map_log_event;
class Rows_log_event;
class Format_description_log_event;
class table_def;
struct st_bitmap;
class MysqlBinlogReader {
  public:
    MysqlBinlogReader();
  private:
    MysqlBinlogReader(const MysqlBinlogReader&);
    const MysqlBinlogReader& operator=(const MysqlBinlogReader&);
  public:
    ~MysqlBinlogReader(){
      TryClose();
      SetDescriptionEvent(NULL);
    }
    //0 - succeed,other :fail.
    int Init(const string& binlog_dir,const string& logprefix,
        const string& from_logname,uint64_t from_pos);
    
    /**
      * @return int 0-succeed,other fail
      */
    int GetNextOperation(MysqlOperation** operation,
        std::list<MysqlBinlogEventInfo>* scanned_event);

    const ErrorInfo& last_error() const { return last_error_;}


    uint64_t next_position() const {return  next_position_;}
    const string& logname() const { return logname_;}

  protected:
    typedef std::tr1::unordered_map<ulong,Table_map_log_event*> TableEventMap;
    int TryOpenLogname();
    int TryOpenNextLogname();
    int CheckHeader(FILE* fp);
    void TryClose();
    static string GetNextLogname(const string& logname,const string& binlog_index);
    bool IsValid() const ;
    void SetError(const char* file,int line,const char* func,const char* msg);

    MysqlOperation* ProcessRowsEvent(Rows_log_event* rows_event) ;

    Table_map_log_event* GetTableMapEvent(uint32_t table_id);
    int SetTableMapEvent(Table_map_log_event* table_map_event);
    int SetDescriptionEvent(Format_description_log_event* description_event);
    void ClearTableMap();
  private:
    size_t ParseOneRow(const uint8_t* value,int columns,st_bitmap* cols_bitmap,
        table_def* table,MysqlRow& row);
    size_t ParseValue(const uint8_t* ptr,uint32_t type,uint32_t meta,DataValue*& val);
    int ScanTableMap(uint64_t to_position);
    
  private:
    Format_description_log_event* description_event_;
    FILE* fp_;
    string logname_;
    uint64_t start_position_;
    uint64_t next_position_;// the position read from here when call GetNextOperation.
    TableEventMap table_map_;
    string log_dir_;
    string log_prefix_;
    ErrorInfo last_error_;
};
#endif  // _MYSQL_BINLOG_READER_H_
