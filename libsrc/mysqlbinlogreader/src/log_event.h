
/**
  @addtogroup Replication
  @{

  @file
  
  @brief Binary log event definitions.  This includes generic code
  common to all types of log events, as well as specific code for each
  type of log event.
*/


#ifndef _log_event_h
#define _log_event_h

#if defined(USE_PRAGMA_INTERFACE) && !defined(MYSQL_CLIENT)
#pragma interface			/* gcc class implementation */
#endif

#include <string>
#include <vector>
//#include "my_bitmap.h"
//#include "rpl_constants.h"
#include "rpl_utility.h"
//#include "hash.h"
//#include "rpl_tblmap.h"
//#include "rpl_tblmap.cc"

#include <mysql.h>
#include <my_compiler.h>
#include <my_sys.h>
#include <m_string.h>
#include "my_bitmap.h"


#include "log_event_constants.h"
#define PREFIX_SQL_LOAD "SQL_LOAD-"



class Format_description_log_event;


/**
  @class Log_event

  This is the abstract base class for binary log events.
  
*/
class Log_event {
public:
//  /**
//     Enumeration of what kinds of skipping (and non-skipping) that can
//     occur when the slave executes an event.
//
//     @see shall_skip
//     @see do_shall_skip
//   */
//  enum enum_skip_reason {
//    /**
//       Don't skip event.
//    */
//    EVENT_SKIP_NOT,
//
//    /**
//       Skip event by ignoring it.
//
//       This means that the slave skip counter will not be changed.
//    */
//    EVENT_SKIP_IGNORE,
//
//    /**
//       Skip event and decrease skip counter.
//    */
//    EVENT_SKIP_COUNT
//  };


  /*
    The following type definition is to be used whenever data is placed 
    and manipulated in a common buffer. Use this typedef for buffers
    that contain data containing binary and character data.
  */
  typedef unsigned char Byte;

  /*
    The offset in the log where this event originally appeared (it is
    preserved in relay logs, making SHOW SLAVE STATUS able to print
    coordinates of the event in the master's binlog). Note: when a
    transaction is written by the master to its binlog (wrapped in
    BEGIN/COMMIT) the log_pos of all the queries it contains is the
    one of the BEGIN (this way, when one does SHOW SLAVE STATUS it
    sees the offset of the BEGIN, which is logical as rollback may
    occur), except the COMMIT query which has its real offset.
  */
  my_off_t log_pos;
  /*
     A temp buffer for read_log_event; it is later analysed according to the
     event's type, and its content is distributed in the event-specific fields.
  */
  char *temp_buf;
  /*
    Timestamp on the master(for debugging and replication of
    NOW()/TIMESTAMP).  It is important for queries and LOAD DATA
    INFILE. This is set at the event's creation time, except for Query
    and Load (et al.) events where this is set at the query's
    execution time, which guarantees good replication (otherwise, we
    could have a query and its event with different timestamps).
  */
  time_t when;
  /* The number of seconds the query took to run on the master. */
  ulong exec_time;
  /* Number of bytes written by write() function */
  ulong data_written;

  /*
    The master's server id (is preserved in the relay log; used to
    prevent from infinite loops in circular replication).
  */
  uint32 server_id;

  /**
    Some 16 flags. See the definitions above for LOG_EVENT_TIME_F,
    LOG_EVENT_FORCED_ROTATE_F, LOG_EVENT_THREAD_SPECIFIC_F, and
    LOG_EVENT_SUPPRESS_USE_F for notes.
  */
  uint16 flags;

  bool cache_stmt;

  /**
    A storage to cache the global system variable's value.
    Handling of a separate event will be governed its member.
  */
  ulong slave_exec_mode;

  Log_event() : temp_buf(0) {}
  static Log_event* read_log_event(FILE* fp,
                                   const Format_description_log_event
                                   *description_event);
  static void *operator new(size_t size)
  {
    return (void*) my_malloc((uint)size, MYF(MY_WME|MY_FAE));
  }

  static void operator delete(void *ptr, size_t)
  {
    my_free(ptr, MYF(MY_WME|MY_ALLOW_ZERO_PTR));
  }

  /* Placement version of the above operators */
  static void *operator new(size_t, void* ptr) { return ptr; }
  static void operator delete(void*, void*) { }

  virtual Log_event_type get_type_code() = 0;
  virtual bool is_valid() const = 0;
  void set_artificial_event() { flags |= LOG_EVENT_ARTIFICIAL_F; }
  void set_relay_log_event() { flags |= LOG_EVENT_RELAY_LOG_F; }
  bool is_artificial_event() const { return flags & LOG_EVENT_ARTIFICIAL_F; }
  bool is_relay_log_event() const { return flags & LOG_EVENT_RELAY_LOG_F; }
  inline bool get_cache_stmt() const { return cache_stmt; }
  Log_event(const char* buf, const Format_description_log_event
            *description_event);
  virtual ~Log_event() { free_temp_buf();}
  void register_temp_buf(char* buf) { temp_buf = buf; }
  void free_temp_buf()
  {
    if (temp_buf)
    {
      my_free(temp_buf, MYF(0));
      temp_buf = 0;
    }
  }
  /*
    Get event length for simple events. For complicated events the length
    is calculated during write()
  */
  virtual int get_data_size() { return 0;}
  static Log_event* read_log_event(const char* buf, uint event_len,
				   const char **error,
                                   const Format_description_log_event
                                   *description_event);
  /**
    Returns the human readable name of the given event type.
  */
  static const char* get_type_str(Log_event_type type);
  /**
    Returns the human readable name of this event's type.
  */
  const char* get_type_str();

  /* Return start of query time or current time */

};


class Dumb_log_event: public Log_event {
  public:
    Dumb_log_event(const char* buf,uint len,
        const Format_description_log_event* description_event,
        Log_event_type type);
    Log_event_type get_type_code(){
      return type_code_;
    }
  bool is_valid() const {
    return true;
  }
    Log_event_type type_code_;
};
/**
  @class Start_log_event_v3

  Start_log_event_v3 is the Start_log_event of binlog format 3 (MySQL 3.23 and
  4.x).

  Format_description_log_event derives from Start_log_event_v3; it is
  the Start_log_event of binlog format 4 (MySQL 5.0), that is, the
  event that describes the other events' Common-Header/Post-Header
  lengths. This event is sent by MySQL 5.0 whenever it starts sending
  a new binlog if the requested position is >4 (otherwise if ==4 the
  event will be sent naturally).

  @section Start_log_event_v3_binary_format Binary Format
*/
class Start_log_event_v3: public Log_event {
public:
  /*
    If this event is at the start of the first binary log since server
    startup 'created' should be the timestamp when the event (and the
    binary log) was created.  In the other case (i.e. this event is at
    the start of a binary log created by FLUSH LOGS or automatic
    rotation), 'created' should be 0.  This "trick" is used by MySQL
    >=4.0.14 slaves to know whether they must drop stale temporary
    tables and whether they should abort unfinished transaction.

    Note that when 'created'!=0, it is always equal to the event's
    timestamp; indeed Start_log_event is written only in log.cc where
    the first constructor below is called, in which 'created' is set
    to 'when'.  So in fact 'created' is a useless variable. When it is
    0 we can read the actual value from timestamp ('when') and when it
    is non-zero we can read the same value from timestamp
    ('when'). Conclusion:
     - we use timestamp to print when the binlog was created.
     - we use 'created' only to know if this is a first binlog or not.
     In 3.23.57 we did not pay attention to this identity, so mysqlbinlog in
     3.23.57 does not print 'created the_date' if created was zero. This is now
     fixed.
  */
  time_t created;
  uint16 binlog_version;
  char server_version[ST_SERVER_VER_LEN];
  /*
    We set this to 1 if we don't want to have the created time in the log,
    which is the case when we rollover to a new log.
  */
  bool dont_set_created;

  Start_log_event_v3() {}
  Start_log_event_v3(const char* buf,
                     const Format_description_log_event* description_event);
  ~Start_log_event_v3() {}
  Log_event_type get_type_code() { return START_EVENT_V3;}
  bool is_valid() const { return 1; }
  int get_data_size()
  {
    return START_V3_HEADER_LEN; //no variable-sized part
  }

};

/**
  @class Format_description_log_event

  For binlog version 4.
  This event is saved by threads which read it, as they need it for future
  use (to decode the ordinary events).

  @section Format_description_log_event_binary_format Binary Format
*/

class Format_description_log_event: public Start_log_event_v3
{
public:
  /*
     The size of the fixed header which _all_ events have
     (for binlogs written by this version, this is equal to
     LOG_EVENT_HEADER_LEN), except FORMAT_DESCRIPTION_EVENT and ROTATE_EVENT
     (those have a header of size LOG_EVENT_MINIMAL_HEADER_LEN).
  */
  uint8 common_header_len;
  uint8 number_of_event_types;
  /* The list of post-headers' lengthes */
  uint8 *post_header_len;
  uchar server_version_split[3];
  const uint8 *event_type_permutation;

  Format_description_log_event(uint8 binlog_ver, const char* server_ver=0);
  Format_description_log_event(const char* buf, uint event_len,
                               const Format_description_log_event
                               *description_event);
  ~Format_description_log_event()
  {
    my_free((uchar*)post_header_len, MYF(MY_ALLOW_ZERO_PTR));
  }
  Log_event_type get_type_code() { return FORMAT_DESCRIPTION_EVENT;}
  bool is_valid() const
  {
    return ((common_header_len >= ((binlog_version==1) ? OLD_HEADER_LEN :
                                   LOG_EVENT_MINIMAL_HEADER_LEN)) &&
            (post_header_len != NULL));
  }
  int get_data_size()
  {
    /*
      The vector of post-header lengths is considered as part of the
      post-header, because in a given version it never changes (contrary to the
      query in a Query_log_event).
    */
    return FORMAT_DESCRIPTION_HEADER_LEN;
  }

  void calc_server_version_split();

};

/**
  @class Query_log_event
   
  A @c Query_log_event is created for each query that modifies the
  database, unless the query is logged row-based.

*/
class Query_log_event: public Log_event
{
  LEX_STRING user;
  LEX_STRING host;
protected:
  Log_event::Byte* data_buf;
public:
  const char* query;
  const char* catalog;
  const char* db;
  /*
    If we already know the length of the query string
    we pass it with q_len, so we would not have to call strlen()
    otherwise, set it to 0, in which case, we compute it with strlen()
  */
  uint32 q_len;
  uint32 db_len;
  uint16 error_code;
  ulong thread_id;
  /*
    For events created by Query_log_event::do_apply_event (and
    Load_log_event::do_apply_event()) we need the *original* thread
    id, to be able to log the event with the original (=master's)
    thread id (fix for BUG#1686).
  */
  ulong slave_proxy_id;

  /*
    Binlog format 3 and 4 start to differ (as far as class members are
    concerned) from here.
  */

  uint catalog_len;			// <= 255 char; 0 means uninited

  /*
    We want to be able to store a variable number of N-bit status vars:
    (generally N=32; but N=64 for SQL_MODE) a user may want to log the number
    of affected rows (for debugging) while another does not want to lose 4
    bytes in this.
    The storage on disk is the following:
    status_vars_len is part of the post-header,
    status_vars are in the variable-length part, after the post-header, before
    the db & query.
    status_vars on disk is a sequence of pairs (code, value) where 'code' means
    'sql_mode', 'affected' etc. Sometimes 'value' must be a short string, so
    its first byte is its length. For now the order of status vars is:
    flags2 - sql_mode - catalog - autoinc - charset
    We should add the same thing to Load_log_event, but in fact
    LOAD DATA INFILE is going to be logged with a new type of event (logging of
    the plain text query), so Load_log_event would be frozen, so no need. The
    new way of logging LOAD DATA INFILE would use a derived class of
    Query_log_event, so automatically benefit from the work already done for
    status variables in Query_log_event.
 */
  uint16 status_vars_len;

  /*
    'flags2' is a second set of flags (on top of those in Log_event), for
    session variables. These are thd->options which is & against a mask
    (OPTIONS_WRITTEN_TO_BIN_LOG).
    flags2_inited helps make a difference between flags2==0 (3.23 or 4.x
    master, we don't know flags2, so use the slave server's global options) and
    flags2==0 (5.0 master, we know this has a meaning of flags all down which
    must influence the query).
  */
  bool flags2_inited;
  bool sql_mode_inited;
  bool charset_inited;

  uint32 flags2;
  /* In connections sql_mode is 32 bits now but will be 64 bits soon */
  ulong sql_mode;
  ulong auto_increment_increment, auto_increment_offset;
  char charset[6];
  uint time_zone_len; /* 0 means uninited */
  const char *time_zone_str;
  uint lc_time_names_number; /* 0 means en_US */
  uint charset_database_number;
  /*
    map for tables that will be updated for a multi-table update query
    statement, for other query statements, this will be zero.
  */
  ulonglong table_map_for_update;
  /*
    Holds the original length of a Query_log_event that comes from a
    master of version < 5.0 (i.e., binlog_version < 4). When the IO
    thread writes the relay log, it augments the Query_log_event with a
    Q_MASTER_DATA_WRITTEN_CODE status_var that holds the original event
    length. This field is initialized to non-zero in the SQL thread when
    it reads this augmented event. SQL thread does not write 
    Q_MASTER_DATA_WRITTEN_CODE to the slave's server binlog.
  */
  uint32 master_data_written;

  const char* get_db() { return db; }

  Query_log_event();
  Query_log_event(const char* buf, uint event_len,
                  const Format_description_log_event *description_event,
                  Log_event_type event_type);
  ~Query_log_event()
  {
    if (data_buf)
      my_free((uchar*) data_buf, MYF(0));
  }
  Log_event_type get_type_code() { return QUERY_EVENT; }
  bool is_valid() const { return query != 0; }

  /*
    Returns number of bytes additionaly written to post header by derived
    events (so far it is only Execute_load_query event).
  */
  virtual ulong get_post_header_size_for_derived() { return 0; }
  /* Writes derived event-specific part of post header. */

public:        /* !!! Public in this patch to allow old usage */
  /*
    If true, the event always be applied by slave SQL thread or be printed by
    mysqlbinlog
   */
  bool is_trans_keyword()
  {
    /*
      Before the patch for bug#50407, The 'SAVEPOINT and ROLLBACK TO'
      queries input by user was written into log events directly.
      So the keywords can be written in both upper case and lower case
      together, strncasecmp is used to check both cases. they also could be
      binlogged with comments in the front of these keywords. for examples:
        / * bla bla * / SAVEPOINT a;
        / * bla bla * / ROLLBACK TO a;
      but we don't handle these cases and after the patch, both quiries are
      binlogged in upper case with no comments.
     */
    return !strncmp(query, "BEGIN", q_len) ||
      !strncmp(query, "COMMIT", q_len) ||
      !strncasecmp(query, "SAVEPOINT", 9) ||
      !strncasecmp(query, "ROLLBACK", 8);
  }
};

/**
  @class Rows_log_event

 Common base class for all row-containing log events.

 RESPONSIBILITIES

   Encode the common parts of all events containing rows, which are:
   - Write data header and data body to an IO_CACHE.
   - Provide an interface for adding an individual row to the event.

  @section Rows_log_event_binary_format Binary Format
*/


class Rows_log_event : public Log_event
{
public:
  /**
     Enumeration of the errors that can be returned.
   */
  enum enum_error
  {
    ERR_OPEN_FAILURE = -1,               /**< Failure to open table */
    ERR_OK = 0,                                 /**< No error */
    ERR_TABLE_LIMIT_EXCEEDED = 1,      /**< No more room for tables */
    ERR_OUT_OF_MEM = 2,                         /**< Out of memory */
    ERR_BAD_TABLE_DEF = 3,     /**< Table definition does not match */
    ERR_RBR_TO_SBR = 4  /**< daisy-chanining RBR to SBR not allowed */
  };

  /*
    These definitions allow you to combine the flags into an
    appropriate flag set using the normal bitwise operators.  The
    implicit conversion from an enum-constant to an integer is
    accepted by the compiler, which is then used to set the real set
    of flags.
  */
  enum enum_flag
  {
    /* Last event of a statement */
    STMT_END_F = (1U << 0),

    /* Value of the OPTION_NO_FOREIGN_KEY_CHECKS flag in thd->options */
    NO_FOREIGN_KEY_CHECKS_F = (1U << 1),

    /* Value of the OPTION_RELAXED_UNIQUE_CHECKS flag in thd->options */
    RELAXED_UNIQUE_CHECKS_F = (1U << 2),

    /** 
      Indicates that rows in this event are complete, that is contain
      values for all columns of the table.
     */
    COMPLETE_ROWS_F = (1U << 3)
  };

  typedef uint16 flag_set;

  /* Special constants representing sets of flags */
  enum 
  {
      RLE_NO_FLAGS = 0U
  };

  virtual ~Rows_log_event();

  void set_flags(flag_set flags_arg) { m_flags |= flags_arg; }
  void clear_flags(flag_set flags_arg) { m_flags &= ~flags_arg; }
  flag_set get_flags(flag_set flags_arg) const { return m_flags & flags_arg; }


  /* Member functions to implement superclass interface */
  virtual int get_data_size();

  Log_event_type get_type_code(){
    return type_;
  }
  MY_BITMAP const *get_cols() const { return &m_cols; }
  size_t get_width() const          { return m_width; }
  ulong get_table_id() const        { return m_table_id; }

  /*
    Check that malloc() succeeded in allocating memory for the rows
    buffer and the COLS vector. Checking that an Update_rows_log_event
    is valid is done in the Update_rows_log_event::is_valid()
    function.
  */
  virtual bool is_valid() const
  {
    return m_rows_buf && m_cols.bitmap;
  }

  uint     m_row_count;         /* The number of rows added to the event */

public:
  Rows_log_event(const char *row_data, uint event_len, 
		 Log_event_type event_type,
		 const Format_description_log_event *description_event);


  ulong       m_table_id;	/* Table ID */
  MY_BITMAP   m_cols;		/* Bitmap denoting columns available */
  ulong       m_width;          /* The width of the columns bitmap */
  /*
    Bitmap for columns available in the after image, if present. These
    fields are only available for Update_rows events. Observe that the
    width of both the before image COLS vector and the after image
    COLS vector is the same: the number of columns of the table on the
    master.
  */
  MY_BITMAP   m_cols_ai;

  ulong       m_master_reclength; /* Length of record on master side */

  /* Bit buffers in the same memory as the class */
  uint32    m_bitbuf[128/(sizeof(uint32)*8)];
  uint32    m_bitbuf_ai[128/(sizeof(uint32)*8)];

  uchar    *m_rows_buf;		/* The rows in packed format */
  uchar    *m_rows_cur;		/* One-after the end of the data */
  uchar    *m_rows_end;		/* One-after the end of the allocated space */

  flag_set m_flags;		/* Flags for row-level events */
  Log_event_type type_;


  friend class Old_rows_log_event;
};

using std::string;
using std::vector;
class table_def;
class Table_map_log_event : public Log_event
{
public:
  /* Constants */
  enum
  {
    TYPE_CODE = TABLE_MAP_EVENT
  };

  ~Table_map_log_event();

  Table_map_log_event(const char *buf, uint event_len, 
                      const Format_description_log_event *description_event);


  int Parse(const char *buf, uint event_len, 
            const Format_description_log_event *description_event);
  table_def* GetTable() const {
    return table_ptr_;
  }
  ulong table_id() const        { return table_id_; }
  const string& table_name() const { return table_name_; }
  const string& db_name() const    { return db_name_; }

  virtual Log_event_type get_type_code() { return TABLE_MAP_EVENT; }
  virtual bool is_valid() const { return true;}

private:
  ulong          table_id_;
  uint16         flags_;
  string db_name_;
  string table_name_;
  ulong          columns_cnt_;
  vector<uchar>  columns_type_;
  table_def *table_ptr_;
};

class Rotate_log_event: public Log_event
{
public:
  string new_logname_;
  ulonglong pos_;

  Rotate_log_event(const char* buf, uint event_len,
                   const Format_description_log_event* description_event);
  ~Rotate_log_event() { }
  Log_event_type get_type_code() { return ROTATE_EVENT;}
  bool is_valid() const { return true; }
};


/**
  @} (end of group Replication)
*/

#endif /* _log_event_h */
