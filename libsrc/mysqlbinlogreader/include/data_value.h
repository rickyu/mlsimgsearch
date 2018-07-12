#ifndef _BINLOG_DATA_VALUE_H_
#define _BINLOG_DATA_VALUE_H_

#include <ostream>
#include <vector>
using std::ostream;
using std::vector;
using std::string;
class DataValue {
  public:
    enum {
      UNKNOWN = 0,
      NULL_VALUE,
      INTEGER,
      STRING,
      BLOB,
      FLOAT
    };
    DataValue(int type):type_(type){ }
    
    int type() const{
      return type_;
    }
    virtual DataValue* Clone() {
      return new DataValue(type_);
    }
    virtual void Print(ostream& out) {
      
    }
    virtual ~DataValue(){
    }
  protected:
    int type_;
};
class DataValueInteger : public DataValue {
  public:
    DataValueInteger(int64_t val)
      :DataValue(DataValue::INTEGER),value_(val){}
    virtual void Print(ostream& out) {
      out << value_ ;
    }

  public:
    int64_t value_;

};
class DataValueFloat:public DataValue {
  public:
    DataValueFloat(double val)
      :DataValue(DataValue::FLOAT),value_(val){}
    virtual void Print(ostream& out) {
      out << value_ ;
    }
  public:
    double value_;
};
class DataValueBlob:public DataValue {
  public:
    DataValueBlob(const uint8_t* ptr,size_t len,size_t bits_count)
      :DataValue(DataValue::BLOB),value_(ptr,ptr+len),bits_count_(bits_count)
    {

    }
    virtual void Print(ostream& out) {
    }

  public:
    vector<uint8_t> value_;
    size_t bits_count_;
};
class DataValueString : public DataValue {
  public:
    DataValueString(const char* ptr,size_t len)
      :DataValue(DataValue::STRING),value_(ptr,ptr+len) {
      }

    virtual void Print(ostream& out) {
      out << value_;
    }
    string value_;
};
class DataValueNull : public DataValue {
  public:
    DataValueNull():DataValue(DataValue::NULL_VALUE){
    }
    virtual void Print(ostream& out) {
      out << "NULL";
    }

};
#endif // _BINLOG_MYSQL_VALUE_H_
