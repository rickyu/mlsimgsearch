#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

#include "mysql_binlog_reader.h"
#include "log_event.h"
C_MODE_START
#include <decimal.h>
C_MODE_END
#define BIN_LOG_HEADER_SIZE	4
#define PROBE_HEADER_LEN	(EVENT_LEN_OFFSET+4)


const size_t kMaxErrorMsgLen = 512;

#define OBJ_SET_ERROR(fmt,...) \
{ \
  char errmsg_buf[kMaxErrorMsgLen] = ""; \
  snprintf(errmsg_buf,sizeof(errmsg_buf),fmt,##__VA_ARGS__); \
  SetError(__FILE__,__LINE__,__FUNCTION__,errmsg_buf); \
}

#define mi_uint4korr_b(A)       ((uint32) (((uint32) (((uchar*) (A))[3])) +\
                                (((uint32) (((uchar*) (A))[2])) << 8) +\
                                (((uint32) (((uchar*) (A))[1])) << 16) +\
                                (((uint32) (((uchar*) (A))[0])) << 24))) 
  
const char* MysqlBinlogEventInfo::GetTypeStr(uint8_t type){
  return Log_event::get_type_str(static_cast<Log_event_type>(type));
}

//#define OBJ_SET_ERROR(fmt,...) 
MysqlBinlogReader::MysqlBinlogReader():description_event_(NULL),
  fp_(NULL),start_position_(0),next_position_(0) {
}

int MysqlBinlogReader::Init(const string& binlog_dir,const string& logprefix,
        const string& from_logname,uint64_t from_pos) {
  using namespace std;
  log_dir_ = binlog_dir;
  log_prefix_ = logprefix;
  logname_ = from_logname;
  start_position_ = from_pos;
  next_position_ = start_position_;

  string binlog_index = log_dir_ + "/" + log_prefix_ + ".index";
  FILE* fp = fopen(binlog_index.c_str(),"r");
  if(!fp){
    OBJ_SET_ERROR("fail_to_open_file(%s)",binlog_index.c_str());
    return -1;
  }
  fclose(fp);
  return 0;
}

int MysqlBinlogReader::GetNextOperation(MysqlOperation** mysql_operation,
    std::list<MysqlBinlogEventInfo>* scanned_event) {
  using namespace std;
  if(!mysql_operation){
    OBJ_SET_ERROR("null_param(mysql_operation");
    return -1;
  }
  int ret = 0;
  if(!fp_){
    ret = TryOpenLogname();
    if(ret > 0){
      return 0;
    }
    else if(ret < 0){
      return -1;
    }
  }
  
  // scan until we find a operation(QUERY,ROWS),the events checked will be push into scanned_event.
  // if no operation is found , the end of file is reached,we return 1.
  while(1){
    uint64_t last_event_pos = ftell(fp_);
    Log_event* log_event  = Log_event::read_log_event(fp_,description_event_);
    if(!log_event){
      if(feof(fp_)){
        //读到文件尾部
        string new_logname = GetNextLogname(logname_,log_dir_ + '/' + log_prefix_ + ".index");
        if(new_logname.empty()) {
           fseek(fp_,next_position_,SEEK_SET);
        }
        else {
          TryClose();
          logname_ = new_logname;
          next_position_ = 0;
        }
        return 0;
      }
      int file_error = ferror(fp_);
      if(file_error){
        OBJ_SET_ERROR("read_file_error(%lu)",last_event_pos);
        TryClose();
        return -1;
      }
      else {
        OBJ_SET_ERROR("read_log_evnet_error(%lu)",last_event_pos);
        TryClose();
        return -1;
      }
    }
    std::auto_ptr<Log_event> event_ptr(log_event);
    if(scanned_event){
      //save event info.
      MysqlBinlogEventInfo event_info;
      event_info.when_ = log_event->when;
      event_info.type_ = log_event->get_type_code();
      event_info.logname_ = logname_;
      event_info.pos_ = log_event->log_pos;
      event_info.data_length_ = log_event->data_written;
      scanned_event->push_back(event_info);
    }
    switch(log_event->get_type_code()){
      case QUERY_EVENT: {
          Query_log_event* query_log_event = 
            dynamic_cast<Query_log_event*>(log_event);
          MysqlSqlOperation *operation = new MysqlSqlOperation();
          operation->sql_ = query_log_event->query;
          operation->dbname_ = query_log_event->db;
          *mysql_operation = operation;
          next_position_ = ftell(fp_);
          return 0;
        }
        break;
      case DELETE_ROWS_EVENT:
      case UPDATE_ROWS_EVENT:
      case WRITE_ROWS_EVENT:{
          Rows_log_event* rows_event = 
            dynamic_cast<Rows_log_event*>(log_event);
          *mysql_operation = ProcessRowsEvent(rows_event);
          next_position_ = ftell(fp_);
          return 0;
        }
        break;
      case TABLE_MAP_EVENT: {
          Table_map_log_event* table_map_event = 
            dynamic_cast<Table_map_log_event*>(log_event);
          SetTableMapEvent(table_map_event);
          event_ptr.release();
        }
        break;
      case ROTATE_EVENT:{
        next_position_ = ftell(fp_);
          Rotate_log_event* rotate_event = 
            dynamic_cast<Rotate_log_event*>(log_event);
          TryClose();
          logname_ = rotate_event->new_logname_;
          next_position_ = rotate_event->pos_;
          return 0;
          
        }
        break; 

      default:
        next_position_ = ftell(fp_);
        break;
    }
  }
  return 0;
}

/**
  Reads the @c Format_description_log_event from the beginning of a
  local input file.
*/
int MysqlBinlogReader::CheckHeader(FILE* fp) {
  // only support mysql5.1.xx ,binlog version = v4
  uchar header[BIN_LOG_HEADER_SIZE];
  long tmp_pos, pos;

  // 记录文件位置,因为可能指定了start_position.
  pos= ftell(fp);
  fseek(fp, (my_off_t)0,SEEK_SET);
  if (fread(header, 1,sizeof(header) , fp) != sizeof(header)) {
    OBJ_SET_ERROR("fail_read_head(%lu)",sizeof(header));
    if(feof(fp)){
      return 1;
    }
    return -1;
  }
  if (memcmp(header, BINLOG_MAGIC, sizeof(header))) {
    OBJ_SET_ERROR("invalid_magic(0x%X)",*(uint32_t*)header);
    return -1;
  }

  Format_description_log_event tmp_description_event(4);
  uchar buf[PROBE_HEADER_LEN];
  // 循环寻找description_event
  for(;;) {
    tmp_pos= ftell(fp); /* should be 4 the first time */
    //先读13个字节,然后判断是v1,v3还是v4.
    //v1的event_length = 69,type_code=1.v3:event_length=75,type_code=1
    //v4:event_length>=91,type_code=15
    if (fread(buf, 1,sizeof(buf), fp) != sizeof(buf)) {
        OBJ_SET_ERROR("fail_read_desc");
        if(feof(fp)){
          return 1;
        }
        else {
          return -1;
        }
    }
    if (buf[EVENT_TYPE_OFFSET] == FORMAT_DESCRIPTION_EVENT) {
      /* This is 5.0 */
      fseek(fp, tmp_pos,SEEK_SET); /* seek back to event's start */
      Format_description_log_event* description_event = NULL;
      if (!(description_event= (Format_description_log_event*) 
            Log_event::read_log_event(fp, &tmp_description_event)))
        /* EOF can't be hit here normally, so it's a real error */
      {
        OBJ_SET_ERROR("fail_read_description_event");
        return -1;
      }
      SetDescriptionEvent(description_event);
      break;
    }
    else if (buf[EVENT_TYPE_OFFSET] == ROTATE_EVENT) {
      Log_event *ev = NULL;
      fseek(fp, tmp_pos,SEEK_SET); /* seek back to event's start */
      if (!(ev= Log_event::read_log_event(fp, description_event_)))
      {
        OBJ_SET_ERROR("fail_to_read_rotate(%lu)",tmp_pos);
        /* EOF can't be hit here normally, so it's a real error */
       // error("Could not read a Rotate_log_event event at offset %llu;"
       //       " this could be a log format error or read error.",
       //       (ulonglong)tmp_pos);
        return -1;
      }
      delete ev;
    }
    else {
      OBJ_SET_ERROR("invalid_event_type_when_checking_head(%lu,%d)",
          tmp_pos,buf[EVENT_TYPE_OFFSET]);
      return -1;
    }
  }
  fseek(fp, pos,SEEK_SET);
  return 0;
}



/**
  * @return int ,0 succeed,1 eof,<0 error.
  */
int MysqlBinlogReader::TryOpenLogname() {
  using namespace std;
  if(fp_){
    OBJ_SET_ERROR("close_logname_first");
    return -1;
  }
  FILE* fp = NULL;
  int retval = 0;
  string logfile = log_dir_ + '/' +  logname_;
  /* read from normal file */
  if ((fp = fopen(logfile.c_str(), "rb")) == NULL) {
    OBJ_SET_ERROR("fail_to_open_file(%s)",logfile.c_str());
    return -1;
  }
  retval= CheckHeader(fp);
  if(retval != 0){
    fclose(fp);
    return retval;
  }
  fp_ = fp;
  if(next_position_ != 0) {
    fseek(fp_,BIN_LOG_HEADER_SIZE,SEEK_SET);
    ScanTableMap(start_position_);
    fseek(fp_,next_position_,SEEK_SET);
    //scan 所有的table_map_event.
  } else {
    fseek(fp_,BIN_LOG_HEADER_SIZE,SEEK_SET);
  }
  return 0;
}
bool MysqlBinlogReader::IsValid() const {
  if(!fp_ || !description_event_){
    return false;
  }
  return true;
}
size_t MysqlBinlogReader::ParseOneRow( const uint8_t* value,int columns,
    MY_BITMAP* cols_bitmap,table_def* table,MysqlRow& mysql_row){
    using namespace std;
  // |null_fields_bits|values|
  const uchar *value0= value;
  const uchar *null_bits= value;
  uint null_bit_index= 0;
  
  value += (columns + 7) / 8;
  for (size_t i= 0; i < columns; i ++) {
    int is_null= (null_bits[null_bit_index / 8] 
                  >> (null_bit_index % 8))  & 0x01;

    if (bitmap_is_set(cols_bitmap, i) == 0)
      continue;
    
    DataValue* val = NULL;
    if (is_null) {
      val = new DataValueNull();
      mysql_row.SetValue(i,val);
    }
    else {
      size_t size = ParseValue( value, table->type(i), 
          table->field_metadata(i),val);
      if(val){
        mysql_row.SetValue(i,val);
      }
      
      if (!size){
        return 0;
      }
      value+= size;
    }
    
    null_bit_index++;
  }
  return value - value0;
}
MysqlOperation* MysqlBinlogReader::ProcessRowsEvent(Rows_log_event* rows_event){
  Log_event_type type_code= rows_event->get_type_code();
  MysqlRowChange::TypeEnum change_type = MysqlRowChange::UNKNOWN;
  switch (type_code) {
    case WRITE_ROWS_EVENT:
      change_type = MysqlRowChange::INSERT;
      break;
    case DELETE_ROWS_EVENT:
      change_type = MysqlRowChange::DELETE;
      break;
    case UPDATE_ROWS_EVENT:
      change_type = MysqlRowChange::UPDATE;
      break;
    default:
      return NULL;
  }
  MysqlRowOperation* operation =  new MysqlRowOperation();
  Table_map_log_event* table_map_event = GetTableMapEvent(rows_event->m_table_id);
  if(!table_map_event){
    delete operation;
    return NULL;
  }
  operation->db_name_ = table_map_event->db_name();
  operation->table_name_ = table_map_event->table_name();
  table_def* table_def_obj = table_map_event->GetTable();

  const uchar *value= rows_event->m_rows_buf; 
  while(value < rows_event->m_rows_end ) {
    size_t length;
    MysqlRowChange* row_change = new MysqlRowChange(change_type);
    MysqlRow* row1 = NULL;
    MysqlRow* row2 = NULL;
    if(type_code == WRITE_ROWS_EVENT){
      row1 = &row_change->data_;
    } else {
      row1 = &row_change->condition_;
      row2 = &row_change->data_;
    }
    if (!(length= ParseOneRow(value,rows_event->m_width, &rows_event->m_cols,
            table_def_obj,*row1 ))){
      break;
    }
    value+= length;

    /* Print the second image (for UPDATE only) */
    if (type_code == UPDATE_ROWS_EVENT) {
      //MysqlRow *row_where = new MysqlRow();
      if (!(length= ParseOneRow(value,rows_event->m_width,
              &rows_event->m_cols_ai,table_def_obj,*row2))){
        break;
      }
      value+= length;
    }
    operation->AddRowChange(row_change);
  }
  return operation;
}
  
 size_t MysqlBinlogReader::ParseValue(const uint8_t *ptr, uint32_t type, uint32_t meta,
     DataValue*& value)
{
  using std::vector;
  using std::string;
  uint32 length= 0;

  if (type == MYSQL_TYPE_STRING) {
    if (meta >= 256)
    {
      uint byte0= meta >> 8;
      uint byte1= meta & 0xFF;
      
      if ((byte0 & 0x30) != 0x30)
      {
        /* a long CHAR() field: see #37426 */
        length= byte1 | (((byte0 & 0x30) ^ 0x30) << 4);
        type= byte0 | 0x30;
        goto beg;
      }

      switch (byte0)
      {
        case MYSQL_TYPE_SET:
        case MYSQL_TYPE_ENUM:
        case MYSQL_TYPE_STRING:
          type= byte0;
          length= byte1;
          break;

        default:

        {
          return 0;
        }
      }
    }
    else
      length= meta;
  }


beg:
  
  switch (type) {
  case MYSQL_TYPE_LONG: {
      value = new DataValueInteger(sint4korr(ptr));
      return 4;
    }

  case MYSQL_TYPE_TINY: {
      value = new DataValueInteger((int)*ptr);
      return 1;
    }

  case MYSQL_TYPE_SHORT:
    {
      value = new DataValueInteger(sint2korr(ptr));
      return 2;
    }
  
  case MYSQL_TYPE_INT24:
    {
      value = new DataValueInteger(sint3korr(ptr));
      return 3;
    }

  case MYSQL_TYPE_LONGLONG:
    {
      value = new DataValueInteger(sint8korr(ptr));
      return 8;
    }

  case MYSQL_TYPE_NEWDECIMAL:
    {
      uint precision= meta >> 8;
      uint decimals= meta & 0xFF;
      uint bin_size= decimal_bin_size(precision, decimals);
      decimal_t dec;
      //初始化buf指针
      dec.buf =(decimal_digit_t *)malloc(sizeof(decimal_digit_t));
      int ret = bin2decimal((uchar*) ptr, &dec,
                        precision, decimals);
      double double_val = 0.0;
      decimal2double(&dec,&double_val);
      value = new DataValueFloat(double_val);
      //free buf指针
      free(dec.buf);
      return bin_size;
    }

  case MYSQL_TYPE_FLOAT:
    {
      float fl;
      float4get(fl, ptr);
      value = new DataValueFloat(fl);
      return 4;
    }

  case MYSQL_TYPE_DOUBLE:
    {
      double dbl;
      float8get(dbl, ptr);
      value = new DataValueFloat(dbl);
      return 8;
    }
  
  case MYSQL_TYPE_BIT:
    {
      /* Meta-data: bit_len, bytes_in_rec, 2 bytes */
      uint nbits= ((meta >> 8) * 8) + (meta & 0xFF);
      length= (nbits + 7) / 8;
      value = new DataValueBlob(ptr,length,nbits);
      return length;
    }
  case 17:
    {
      uint32_t val = mi_uint4korr_b(ptr);
      //std::cout << "debug:" << val << std::endl;
      value = new DataValueInteger(val);
      return 4;
    }
  case MYSQL_TYPE_TIMESTAMP:
    {
      uint32_t val = uint4korr(ptr);
      value = new DataValueInteger(val);
      return 4;
    }

  case 18:
  case MYSQL_TYPE_DATETIME:
    {
      //size_t d, t;
      uint8_t val = uint8korr(ptr); /* YYYYMMDDhhmmss */
      //d= i64 / 1000000;
      //t= i64 % 1000000;
      //my_b_printf(file, "%04d-%02d-%02d %02d:%02d:%02d",
      //            d / 10000, (d % 10000) / 100, d % 100,
      //            t / 10000, (t % 10000) / 100, t % 100);
      //my_snprintf(typestr, typestr_length, "DATETIME");
      value = new DataValueInteger(val);
      return 8;
    }

  case MYSQL_TYPE_TIME:
    {
      uint32 i32= uint3korr(ptr);
      //val = i32;
      //my_b_printf(file, "'%02d:%02d:%02d'",
      //            i32 / 10000, (i32 % 10000) / 100, i32 % 100);
      //my_snprintf(typestr,  typestr_length, "TIME");
      value = new DataValueInteger(i32);
      return 3;
    }
    
  case MYSQL_TYPE_DATE:
    {
      uint i32= uint3korr(ptr);
      value = new DataValueInteger(i32);
      return 3;
    }
  
  case MYSQL_TYPE_YEAR:
    {
      uint32 i32= *ptr;
      value = new DataValueInteger(i32);
      return 1;
    }
  
  case MYSQL_TYPE_ENUM:
    switch (length) { 
    case 1:
      {
      int val = (int)*ptr;
      value = new DataValueInteger(val);
      return 1;
      }
    case 2:
      {
        int32 i32= uint2korr(ptr);
        value = new DataValueInteger(i32);
        return 2;
      }
    default:
      return 0;
    }
    break;
    
  case MYSQL_TYPE_SET:
    //my_b_write_bit(file, ptr , length * 8);
    value = new DataValueBlob(ptr,length,0);
    return length;
  
  case MYSQL_TYPE_BLOB:
    switch (meta) {
    case 1:
      length= *ptr;
      value = new DataValueBlob(ptr + 1,length,0);
      return length + 1;
    case 2:
      length= uint2korr(ptr);
      value = new DataValueBlob(ptr + 2,length,0);
      return length + 2;
    case 3:
      length= uint3korr(ptr);
      value = new DataValueBlob(ptr + 3,length,0);
      return length + 3;
    case 4:
      length= uint4korr(ptr);
      value = new DataValueBlob(ptr + 4,length,0);
      return length + 4;
    default:
      return 0;
    }

  case MYSQL_TYPE_VARCHAR:
  case MYSQL_TYPE_VAR_STRING:
    length= meta;
  case MYSQL_TYPE_STRING:
    if(length < 256){
      length = *ptr;
      value = new DataValueString((const char*)(ptr + 1),  length);
      return length + 1;
    }
    else {
      length= uint2korr(ptr);
      value = new DataValueString((const char*)( ptr + 2),  length);
      return length + 2;
    }


  default:
    {
    }
    break;
  }
  return 0;
}
void MysqlBinlogReader::TryClose(){
  if(fp_){
    fclose(fp_);
    fp_ = NULL;
    ClearTableMap();
  }

}
void MysqlBinlogReader::ClearTableMap() {
  TableEventMap::iterator iter = table_map_.begin();
  TableEventMap::iterator iter_end = table_map_.end();
  for(;iter != iter_end; ++ iter){
    delete iter->second;
  }
  table_map_.clear();
}

string MysqlBinlogReader::GetNextLogname(const string& logname,
    const string& binlog_index) {
  using std::ifstream;
  ifstream fin(binlog_index.c_str());
  if(!fin){
    return string();
  }
  char buf[256];
  while(1){
    fin.getline(buf,sizeof(buf));
    if(!fin){
      return string();
    }
    char* new_logname = strrchr(buf,'/');
    if(new_logname){
      new_logname += 1;
    }
    else {
      new_logname = buf;
    }
    
    if(logname.compare(new_logname) == 0){
      fin.getline(buf,sizeof(buf));
      if(fin){
        new_logname = strrchr(buf,'/');
        if(new_logname){
          new_logname += 1;
        }
        else {
          new_logname = buf;
        }
 
        return string(new_logname);
      } //fin
    } //logname.compare
  } //while(1)
}

int MysqlBinlogReader::SetTableMapEvent(Table_map_log_event* table_map_event){
      using std::pair;
      if(!table_map_event){
        return 0;
      }
      pair<TableEventMap::iterator,bool> insert_result = 
        table_map_.insert(TableEventMap::value_type(table_map_event->table_id(),table_map_event));
      if(insert_result.second){
        return 0;
      }
      delete insert_result.first->second;
      insert_result.first->second = table_map_event;
      return 0;
    }
Table_map_log_event* MysqlBinlogReader::GetTableMapEvent(uint32_t table_id){
      TableEventMap::iterator iter = table_map_.find(table_id);
      if(iter == table_map_.end()){
        return NULL;
      }
      return iter->second;
    }

void MysqlBinlogReader::SetError(const char* file,int line,const char* func,const char* msg){
  last_error_.file_ = file;
  last_error_.function_ = func;
  last_error_.line_ = line;
  last_error_.msg_ = msg;
}
int MysqlBinlogReader::SetDescriptionEvent(
    Format_description_log_event* description_event){
  if(description_event_){
    delete description_event_;
    description_event_ = NULL;
  }
  description_event_ = description_event;
  return 0;
}
int MysqlBinlogReader::ScanTableMap(uint64_t to_position){
  using namespace std;
  int ret = 0;
  while(1){
    uint64_t last_event_pos = ftell(fp_);
    if(last_event_pos >= to_position){
      return 0;
    }
    Log_event* log_event  = Log_event::read_log_event(fp_,description_event_);
    if(!log_event){
      return 0;
    }
    std::auto_ptr<Log_event> event_ptr(log_event);
    switch(log_event->get_type_code()){
      case TABLE_MAP_EVENT: {
          Table_map_log_event* table_map_event = 
            dynamic_cast<Table_map_log_event*>(log_event);
          SetTableMapEvent(table_map_event);
          event_ptr.release();
        }
        break;
      default:
        break;
    }
  }
  return 0;

}
