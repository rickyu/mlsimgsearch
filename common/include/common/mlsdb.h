#ifndef IMAGE_SERVICE_COMMON_MLSDB_H_
#define IMAGE_SERVICE_COMMON_MLSDB_H_

#include <mlsstorage_service.h>
#include <common/constant.h>
#include <boost/lexical_cast.hpp>
#include <typeinfo>
#include <utils/logger.h>


typedef std::vector<std::string> dbresult_row_t;

/**
 *@brief A class to store query result.
 */
class MlsDBResult {
 private:
  typedef struct _db_result_set {
    dbresult_row_t fields;
    std::vector<dbresult_row_t> rows;
  }dbresult_set_t;
  
 public:
  MlsDBResult();
  
  void Insert(const dbresult_row_t &row);
  void Clear();
  void SetFields(const dbresult_row_t &fields);
  int GetRowNum() const;
  int GetColNum() const;
  int GetRow(int row, dbresult_row_t &res) const;
  int GetValue(int row, int col, std::string &value) const;

  template<typename T> T GetValue(int row, int col) const {
    std::string value;
    GetValue(row, col, value);

    if (typeid(T)==typeid(std::string)) {
      value.clear();
    }
    else if (value.empty()) {
      value = "0";
    }

    return boost::lexical_cast<T>(value);
  }

 private:
  dbresult_set_t results_;
};

class MlsDB {
 public:
  /**
   * @param [in] addrs_list IP:PORT[,IP:PORT...]
   */
  MlsDB();
  ~MlsDB();

  int Init(const std::string &write_addrs_list,
           const std::string &read_addrs_list);
  int QueryRead(const char *table,
                const char *cols,
                const char *sql,
                MlsDBResult *result = NULL);
  int QueryWrite(const char *sql,
                 int64_t *last_insert_id = NULL);
  
 private:
  MlsStorageService *storage_service_;
  static CLogger& logger_;
};

#endif
