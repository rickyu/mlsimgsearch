#include "mysql_operation.h"

void MysqlRow::Free(){
  ColumnMap::iterator iter = columns_.begin();
  ColumnMap::iterator iter_end = columns_.end();
  while(iter != iter_end){
    delete iter->second;
    iter->second = NULL;
    ++ iter;
  }
  columns_.clear();
}
DataValue* MysqlRow::GetValue(int column) {
  ColumnMap::iterator iter = columns_.find(column);
  if(iter == columns_.end()){
    return NULL;
  }
  return iter->second;
}
int MysqlRow::SetValue(int column,DataValue* value){
  using std::pair;
  pair<ColumnMap::iterator,bool> result = 
    columns_.insert(ColumnMap::value_type(column,value));
  if(result.second){
    return 0;
  }
  else {
    delete result.first->second;
    result.first->second = value;
    return 0;
  }
}
void MysqlRow::Print(ostream& out){
  ColumnMap::const_iterator iter = columns_.begin();
  ColumnMap::const_iterator iter_end = columns_.end();
  if(iter != iter_end){
     out << iter->first << ':';
     iter->second->Print(out);
     ++ iter;
     for(; iter != iter_end; ++ iter){
      out << ',' << iter->first << ':' ;
      iter->second->Print(out);
    }
  }
}
// MysqlRowChange methods implementatin.
void MysqlRowChange::Print(ostream& out){
  switch(type_){
    case INSERT:
      out << "insert ";
      data_.Print(out);
      break;
    case DELETE:
      out << "delete where ";
      condition_.Print(out);
      break;
    case UPDATE:
      // update 1:abc,2:3 where 1:abc,2:3
      out << "update ";
      data_.Print(out);
      out << " where ";
      condition_.Print(out);
      break;
    default:
      out << "unknown";
      break;
  }
  
}
// MysqlRowOperation method implementation.
void MysqlRowOperation::Print(ostream& out){
  out << "db=" << db_name_
    << "  table=" << table_name_ << ' ';
  vector<MysqlRowChange*>::iterator iter = rows_.begin();
  vector<MysqlRowChange*>::iterator iter_end = rows_.end();
  for(;iter != iter_end; ++ iter){
    (*iter)->Print(out); 
    out << ' ';
  }
}
int MysqlRowOperation::AddRowChange(MysqlRowChange* row_change){
  if(!row_change){
    return -1;
  }
  rows_.push_back(row_change);
  return 0;
}
void MysqlRowOperation::Free(){
  vector< MysqlRowChange* >::iterator iter = rows_.begin();
  vector< MysqlRowChange* >::iterator iter_end= rows_.end();
  for(; iter != iter_end; ++ iter){
    delete *iter;
  }
  rows_.clear();
}
void MysqlSqlOperation::Print(ostream& out){
  out << "[" << dbname_ << "] [" << sql_ << "]";
}
