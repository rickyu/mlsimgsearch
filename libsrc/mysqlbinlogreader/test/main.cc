#include <iostream>
#include "main.h"
#include "mysql_binlog_reader.h"


int main(int argc,char* argv[]){
  using namespace std;
  if(argc != 5){
    cout << "usage:" << argv[0] 
      << " binlog_dir prefix start_logname start_pos"  << endl;
    return 1;
  }
  MysqlBinlogReader binlog_reader;
  int ret = binlog_reader.Init(argv[1],argv[2],argv[3],atoi(argv[4]));
  int num = 0;
  MysqlOperation* operation = NULL;
  std::list<MysqlBinlogEventInfo> scanned_event;
  while(1) {
    ret = binlog_reader.GetNextOperation(&operation,&scanned_event);
    if(ret != 0){
      cout << "error occured,ret=" << ret << endl;
      const ErrorInfo& err = binlog_reader.last_error();
      cout << err.file_ << ',' << err.msg_ << ','<<err.function_ << endl;
      break;
    }
    
    cout << "<<<<< next pos:" << binlog_reader.logname() << "," << binlog_reader.next_position() ;
    cout << ",scanned event " << scanned_event.size() << endl;
    ++ num;
    if(operation){
      operation->Print(cout);
      cout << endl;
      delete operation;
      operation = NULL;
    }
    else {
      cout << "null" << endl;
      sleep(1);
    }
    scanned_event.clear();
  }
  return 0;
}
