#include <common/mlsdb.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

CLogger& MlsDB::logger_ = CLogger::GetInstance("mlsdb");

MlsDBResult::MlsDBResult() {
  results_.rows.clear();
}

void MlsDBResult::Insert(const dbresult_row_t &row) {
  results_.rows.push_back(row);
}

void MlsDBResult::SetFields(const dbresult_row_t &fields) {
  results_.fields = fields;
}

int MlsDBResult::GetRowNum() const {
  return results_.rows.size();
}

int MlsDBResult::GetColNum() const {
  return results_.fields.size();
}

int MlsDBResult::GetRow(int row, dbresult_row_t &res) const {
  if (row < 0 || row >= (int)results_.rows.size())
    return 0;

  res = results_.rows[row];
  return 1;
}

void MlsDBResult::Clear() {
  results_.fields.clear();
  results_.rows.clear();
}

int MlsDBResult::GetValue(int row, int col, std::string &value) const {
  if (row < 0 || row >= (int)results_.rows.size()
      || col < 0 ||
      col >= (int)results_.rows[row].size())
    return 0;

  value = results_.rows[row][col];
  return 1;
}


MlsDB::MlsDB()
  : storage_service_(NULL) {
}

MlsDB::~MlsDB() {
  if (storage_service_) {
    delete storage_service_;
    storage_service_ = NULL;
  }
}

int MlsDB::Init(const std::string &write_addrs_list,
                const std::string &read_addrs_list) {
  std::vector<ServerInfo> server_info_list;
  std::vector<std::string> addrs;
  boost::algorithm::split(addrs, write_addrs_list, boost::algorithm::is_any_of(",")); 
  if (addrs.size() <= 0) {
    return CR_MYSQL_INIT_FAILED;
  }
  for (size_t i = 0; i < addrs.size(); i++) {
    std::vector<std::string> ipport;
    boost::algorithm::split(ipport, addrs[i], boost::algorithm::is_any_of(":"));
    ServerInfo sinfo;
    sinfo.ip_ = ipport[0];
    boost::algorithm::trim(ipport[1]);
    sinfo.port_ = boost::lexical_cast<unsigned int>(ipport[1]);
    server_info_list.push_back(sinfo);
  }
  std::vector<ServerInfo> slave_server_info_list;
  std::vector<std::string> slave_addrs;
  boost::algorithm::split(slave_addrs, read_addrs_list, boost::algorithm::is_any_of(",")); 
  if (slave_addrs.size() <= 0) {
    return CR_MYSQL_INIT_FAILED;
  }
  for (size_t i = 0; i < slave_addrs.size(); i++) {
    std::vector<std::string> ipport;
    boost::algorithm::split(ipport, slave_addrs[i], boost::algorithm::is_any_of(":"));
    ServerInfo sinfo;
    sinfo.ip_ = ipport[0];
    boost::algorithm::trim(ipport[1]);
    sinfo.port_ = boost::lexical_cast<unsigned int>(ipport[1]);
    slave_server_info_list.push_back(sinfo);
  }
  storage_service_ = new MlsStorageService(server_info_list,
                                           slave_server_info_list);
  if (0 != storage_service_->Init()) {
    return CR_MYSQL_INIT_FAILED;
  }
  return CR_SUCCESS;
}

int MlsDB::QueryRead(const char *table,
                     const char *cols,
                     const char *sql,
                     MlsDBResult *result /*= NULL*/) {
  QueryReadResp qresp;
  storage_service_->GetQueryData(table, cols, sql, qresp);
  if (qresp.result_ < 0) {
    LOGGER_ERROR(logger_, "act=MlsDB::QueryRead failed, ret=%d, sql=%s",
                 qresp.result_, sql);
    return CR_MYSQL_QUERY_FAILED;
  }
  LOGGER_DEBUG(logger_, "act=MlsDB::QueryRead success, sql=%s, select_columns=%s,"
               "rows=%ld", sql, qresp.selected_columns_.c_str(), qresp.rows_.size());
  if (NULL == result) {
    return CR_SUCCESS;
  }
  result->Clear();
  std::vector<RowSingle>::iterator iter = qresp.rows_.begin();
  for (; iter != qresp.rows_.end(); iter ++) {
    std::vector<AnyVal>::iterator it = iter->column_values_.begin();
    dbresult_row_t res_row;
    for (; it != iter->column_values_.end(); it ++) {
      std::string* tmp = it->getStringVal();
      res_row.push_back(*tmp);
      delete tmp;
    }
    result->Insert(res_row);
  }
  return CR_SUCCESS;
}

int MlsDB::QueryWrite(const char *sql,
                      int64_t *last_insert_id /*=NULL*/) {
  QueryWriteResp wresp;
  storage_service_->PreStmtWrite(sql, std::vector<AnyVal>(), wresp);
  if (0 != wresp.result_) {
    LOGGER_ERROR(logger_, "act=MlsDB::QueryWrite failed, ret=%d, sql=%s",
                 wresp.result_, sql);
    return CR_MYSQL_QUERY_FAILED;
  }
  LOGGER_DEBUG(logger_, "act=MlsDB::QueryWrite success, sql=%s", sql);
  if (last_insert_id) {
    *last_insert_id = wresp.last_insert_id_;
  }
  return CR_SUCCESS;
}
