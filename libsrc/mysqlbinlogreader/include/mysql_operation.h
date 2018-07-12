#ifndef _BINLOG_MYSQL_OPERATION_H_
#define _BINLOG_MYSQL_OPERATION_H_

#include <string>
#include <map>
#include "data_value.h"

using std::string;
// one row in mysql ,contains multi column.
class MysqlRow {
  public:
    typedef std::map<int,DataValue*> ColumnMap;
    MysqlRow(){}
  private:
    MysqlRow(const MysqlRow&);
    MysqlRow& operator=(const MysqlRow&);
  public:
    ~MysqlRow(){
      Free();
    }
    void Free();
    DataValue* GetValue(int column) ;
    // MysqlRow负责释放value.
    int SetValue(int column,DataValue* value);
    const ColumnMap* columns() const { return &columns_; }
    void Print(ostream& out);
  private:
    ColumnMap columns_;
};
class MysqlOperation {
  public:
    enum {
      UNKNOWN = 0,
      SQL = 1,
      ROW
    };
    MysqlOperation():type_(UNKNOWN) {}
    
    int type() const {return type_;}
    virtual ~MysqlOperation(){}
    virtual void Print(ostream& out) = 0;
  public:
    int type_;
};
class MysqlSqlOperation: public MysqlOperation {
  public:
    MysqlSqlOperation() {type_ = SQL;}
    virtual void Print(ostream& out);
    string sql_;
    string dbname_;
};
class MysqlRowChange {
  public:
    enum TypeEnum {
      UNKNOWN = 0,
      INSERT = 1,
      DELETE,
      UPDATE
    };
    MysqlRowChange(TypeEnum type):type_(type) { }
    void Print(ostream& out);
  public:
    TypeEnum type_;
    MysqlRow data_;
    MysqlRow condition_;
};


class MysqlRowOperation : public MysqlOperation {
  public:
    MysqlRowOperation(){type_ = ROW;}
    ~MysqlRowOperation() {Free();}
  private:
    MysqlRowOperation(const MysqlRowOperation& rhs);
    MysqlRowOperation& operator=(const MysqlRowOperation&);
  public:
    virtual void Print(ostream& out);
    int AddRowChange(MysqlRowChange* row_change);
    size_t GetRowCount() const { return rows_.size();}
    const MysqlRowChange* GetRow(size_t i) { return rows_[i];}
    void Free();
  public:
    string db_name_;
    string table_name_;
    vector< MysqlRowChange* > rows_;
};


#endif // _BINLOG_MYSQL_OPERATION_H_
