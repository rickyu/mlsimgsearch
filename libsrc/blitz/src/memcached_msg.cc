#include "blitz/memcached_msg.h"

namespace blitz {

#define MEMCACHED_COMMAND_GET "get"
#define MEMCACHED_COMMAND_DELETE "delete"
#define MEMCACHED_COMMAND_INCREMENT "incr"
#define MEMCACHED_COMMAND_DECREMENT "decr"
#define MEMCACHED_COMMAND_SET "set"
#define MEMCACHED_COMMAND_ADD "add"
#define MEMCACHED_COMMAND_REPLACE "replace"
#define MEMCACHED_COMMAND_APPEND "append"
#define MEMCACHED_COMMAND_PREPEND "prepend"
#define MEMCACHED_COMMAND_CAS "cas"
#define MEMCACHED_END_ESCAPE "\r\n"
#define MEMCACHED_END_KEY "END"

static inline const char *storage_op_string(memcached_storage_op_t verb)
{
  switch (verb)
  {
  case SET_OP:
    return MEMCACHED_COMMAND_SET;
  case REPLACE_OP:
    return MEMCACHED_COMMAND_REPLACE;
  case ADD_OP:
    return MEMCACHED_COMMAND_ADD;
  case PREPEND_OP:
    return MEMCACHED_COMMAND_PREPEND;
  case APPEND_OP:
    return MEMCACHED_COMMAND_APPEND;
  case CAS_OP:
    return MEMCACHED_COMMAND_CAS;
  default:
    return "tosserror";
  }

  /* NOTREACHED */
}

int MemcachedReqFunction::Get(char *req_buf, size_t buf_size,
			      const char *key, size_t key_len)
{
  int len = -1;

  if (!req_buf || !key)
    return len;

  len = snprintf(req_buf, buf_size, "%s %.*s\r\n",
                 MEMCACHED_COMMAND_GET, static_cast<int>(key_len), key);

  return len;
}

int MemcachedReqFunction::Mget(char *req_buf, size_t buf_size,
			       const char* const* key, const size_t *key_length,
			       size_t number_of_keys)
{
  int tlen = -1;
  int len = -1;

  if (!req_buf || !key)
    return tlen;
	
  len = snprintf(req_buf, buf_size, "%s ", MEMCACHED_COMMAND_GET);
  tlen = len;
  for (size_t i = 0; i < number_of_keys; i++) {
    len = snprintf(req_buf+tlen, buf_size-tlen, " %.*s",
		   static_cast<int>(*(key_length+i)), key[i]);
    tlen += len;
  }
  len = snprintf(req_buf+tlen, buf_size-tlen, "\r\n");
  tlen += len;

  return tlen;
}

int MemcachedReqFunction::Delete(char *req_buf, size_t buf_size,
				 const char *key, size_t key_length,
				 time_t expiration)
{
  int len = -1;

  if (!req_buf || !key)
    return len;

  len = snprintf(req_buf, buf_size,
		 "%s %.*s %lu\r\n",
		 MEMCACHED_COMMAND_DELETE,
		 static_cast<int>(key_length), key,
		 expiration);
  return len;
}

int MemcachedReqFunction::Increase(char *req_buf, size_t buf_size,
				   const char *key, size_t key_length,
				   uint64_t delta)
{
  int len = -1;

  if (!req_buf || !key)
    return len;

  len = snprintf(req_buf, buf_size, "%s %.*s %lu\r\n",
		 MEMCACHED_COMMAND_INCREMENT,
		 static_cast<int>(key_length), key, delta);

  return len;
}

int MemcachedReqFunction::Decrease(char *req_buf, size_t buf_size,
				   const char *key, size_t key_length,
				   uint64_t delta)
{
  int len = -1;

  if (!req_buf || !key)
    return len;

  len = snprintf(req_buf, buf_size, "%s %.*s %lu\r\n",
		 MEMCACHED_COMMAND_DECREMENT,
		 static_cast<int>(key_length), key, delta);

  return len;
}

int MemcachedReqFunction::Storage(char *req_buf, size_t buf_size,
				  const char *key, size_t key_length,
				  const char *value, size_t value_length,
				  time_t expiration,
				  uint32_t flags,
				  uint64_t cas,
				  memcached_storage_op_t op)
{
  int len = -1;

  if (!req_buf || !key)
    return len;

  len = snprintf(req_buf, buf_size,
		 "%s %.*s %u %llu %zu %lu\r\n%.*s\r\n",
		 storage_op_string(op),
		 static_cast<int>(key_length), key,
		 flags,
		 (unsigned long long)expiration,
		 value_length,
		 op==CAS_OP?cas:0,
		 static_cast<int>(value_length), value);

  return len;
}


/////////////////////////////////////////////////////
///////////////// MemcachedReplyDecoder /////////////
/////////////////////////////////////////////////////
typedef enum {
  MEMCACHED_RC_SUCCESS, /* 0 */
  MEMCACHED_RC_PROTOCOL_ERROR, /* 1 */
  MEMCACHED_RC_UNKNOWN_READ_FAILURE, /* 2 */
  MEMCACHED_RC_STAT, /* 3*/
  MEMCACHED_RC_SERVER_ERROR, /* 4 */
  MEMCACHED_RC_STORED, /* 5 */
  MEMCACHED_RC_DELETED, /* 6 */
  MEMCACHED_RC_NOTFOUND, /* 7 */
  MEMCACHED_RC_NOTSTORED, /* 8 */
  MEMCACHED_RC_END, /* 9 */
  MEMCACHED_RC_DATA_EXISTS, /* 10 */
  MEMCACHED_RC_ITEM, /* 11 */
  MEMCACHED_RC_CLIENT_ERROR, /* 12 */
  MEMCACHED_RC_READ_PATIAL, /* 13 */
} memcached_result_code_t;

/**
 * @breif read one line from response
 * @param src [in] response buffer
 * @param src_length [in]
 * @param dest [out]
 * @param dest_length [in] size of dest
 * @return 0: success, 1: invalid src & dest buffer, 2: protocal error
 */
static memcached_result_code_t _memcached_read_line(char *src, size_t src_length,
						    char *dest, size_t dest_length)
{
  bool line_complete = false;
  size_t total_nr= 0;

  if (!src || !dest)
    return MEMCACHED_RC_PROTOCOL_ERROR;

  while (src_length && total_nr < dest_length && !line_complete) {
    *dest = *src;
    if (*src == '\n')
      line_complete = true;
    --src_length;
    ++src;
    ++dest;
    ++total_nr;
  }

  if (total_nr == dest_length)
    return MEMCACHED_RC_PROTOCOL_ERROR;

  if (!line_complete)
    return MEMCACHED_RC_READ_PATIAL;

  return MEMCACHED_RC_SUCCESS;
}

// static memcached_return_t _textual_value_fetch(char **src, size_t src_length,
// 																							 char *cur_line, memcached_result_st *result)
// {
//   Memcached_Return_t rc = MEMCACHED_SUCCESS;
//   char *string_ptr;
//   char *end_ptr;
//   char *next_ptr;
//   size_t value_length;
//   size_t to_read;
//   char *value_ptr;
//   ssize_t read_length= 0;
//   memcached_return_t rrc;

//   end_ptr= cur_line + MEMCACHED_DEFAULT_COMMAND_SIZE;

//   memcached_result_reset(result);

//   string_ptr= cur_line;
//   string_ptr+= 6; /* "VALUE " */

//   /* We load the key */
//   {
//     char *key;
//     size_t prefix_length;

//     key= result->item_key;
//     result->key_length= 0;

//     for (prefix_length= 0;
// 				 !(iscntrl(*string_ptr) || isspace(*string_ptr));
// 				 string_ptr++) {
//       if (prefix_length == 0) {
//         *key= *string_ptr;
//         key++;
//         result->key_length++;
//       }
//       else
//         prefix_length--;
//     }
//     result->item_key[result->key_length]= 0;
//   }

//   if (end_ptr == string_ptr)
//     goto read_error;

//   /* Flags fetch move past space */
//   string_ptr++;
//   if (end_ptr == string_ptr)
//     goto read_error;
//   for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++);
//   result->item_flags= (uint32_t) strtoul(next_ptr, &string_ptr, 10);

//   if (end_ptr == string_ptr)
//     goto read_error;

//   /* Length fetch move past space*/
//   string_ptr++;
//   if (end_ptr == string_ptr)
//     goto read_error;

//   for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++);
//   value_length= (size_t)strtoull(next_ptr, &string_ptr, 10);

//   if (end_ptr == string_ptr)
//     goto read_error;

//   /* Skip spaces */
//   if (*string_ptr == '\r') {
//     /* Skip past the \r\n */
//     string_ptr+= 2;
//   }
//   else {
//     string_ptr++;
//     for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++);
//     result->item_cas= strtoull(next_ptr, &string_ptr, 10);
//   }

//   if (end_ptr < string_ptr)
//     goto read_error;

//   /* We add two bytes so that we can walk the \r\n */
//   rc= memcached_string_check(&result->value, value_length+2);
//   if (rc != MEMCACHED_SUCCESS) {
//     value_length= 0;
//     return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
//   }

// 	*src += string_ptr-cur_line;
//   value_ptr= memcached_string_value_mutable(&result->value);
//   /*
//     We read the \r\n into the string since not doing so is more
//     cycles then the waster of memory to do so.

//     We are null terminating through, which will most likely make
//     some people lazy about using the return length.
//   */
//   to_read= (value_length) + 2;
// 	read_length = to_read;
//   memcpy(value_ptr, (*src)+(string_ptr-cur_line), to_read);

//   if (read_length != (ssize_t)(value_length + 2)) {
//     goto read_error;
//   }

//   /* This next bit blows the API, but this is internal....*/
//   {
//     char *char_ptr;
//     char_ptr = memcached_string_value_mutable(&result->value);;
//     char_ptr[value_length] = 0;
//     char_ptr[value_length + 1] = 0;
//     memcached_string_set_length(&result->value, value_length);
//   }
	
// 	*src += read_length;
	
//   return MEMCACHED_SUCCESS;

// read_error:
//   return MEMCACHED_PARTIAL_READ;
// }

/**
 * @brief
 * @param src [in] responce string
 * @param src_size [in] size of src
 * @param valid_len [out] length of valid responce string escapse last \r\n
 * @return 0: success
 */
static memcached_result_code_t _textual_parse_response(char *src, size_t src_size, int32_t *valid_len)
{
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  char *cur = NULL;
  memcached_result_code_t rc = _memcached_read_line(src, src_size, buffer, sizeof(buffer));
  if (0 != rc)
    return rc;

  switch (buffer[0])  {
  case 'V': /* VALUE || VERSION */
    if (buffer[1] == 'A') {/* VALUE */
      /* We add back in one because we will need to search for END */
      //return _textual_value_fetch(src, src_length, buffer, result);
      cur = strstr(src, MEMCACHED_END_KEY);
      if (!cur)
	return MEMCACHED_RC_READ_PATIAL;
      *valid_len = cur-src+3; /* 3<->END */
      return MEMCACHED_RC_SUCCESS;
    }
    else if (buffer[1] == 'E') {/* VERSION */
      cur = strstr(src, MEMCACHED_END_ESCAPE);
      if (!cur)
	return MEMCACHED_RC_READ_PATIAL;
      *valid_len = cur-src;
      return MEMCACHED_RC_SUCCESS;
    }
    else {
      return MEMCACHED_RC_UNKNOWN_READ_FAILURE;
    }
  case 'O': /* OK */
    *valid_len = 2; /* OK */
    return MEMCACHED_RC_SUCCESS;
  case 'S': {/* STORED STATS SERVER_ERROR */
    if (buffer[2] == 'A') {/* STORED STATS */
      cur = strstr(src, MEMCACHED_END_KEY);
      if (!cur)
	return MEMCACHED_RC_READ_PATIAL;
      *valid_len = cur-src+3; /* 3<->END */
      return MEMCACHED_RC_STAT;
    }
    else if (buffer[1] == 'E') { /* SERVER_ERROR */
      cur = strstr(src, MEMCACHED_END_ESCAPE);
      if (!cur)
	return MEMCACHED_RC_READ_PATIAL;
      *valid_len = cur-src;
      return MEMCACHED_RC_SERVER_ERROR;
    }
    else if (buffer[1] == 'T') {
      *valid_len = 6; /* 6<->STORED */
      return MEMCACHED_RC_STORED;
    }
    else {
      return MEMCACHED_RC_UNKNOWN_READ_FAILURE;
    }
  }
  case 'D': { /* DELETED */
    *valid_len = 7; /* 7<->DELETED */
    return MEMCACHED_RC_DELETED;
  }
  case 'N': {/* NOT_FOUND */
    if (buffer[4] == 'F') {
      *valid_len = 9; /* 9<->NOT_FOUND */
      return MEMCACHED_RC_NOTFOUND;
    }
    else if (buffer[4] == 'S') {
      *valid_len = 10; /* 10<->NOT_STORED */
      return MEMCACHED_RC_NOTSTORED;
    }
    else {
      return MEMCACHED_RC_UNKNOWN_READ_FAILURE;
    }
  }
  case 'E': {/* PROTOCOL ERROR or END */
    if (buffer[1] == 'N') {
      *valid_len = 3; /* 3<->END */
      return MEMCACHED_RC_END;
    }
    else if (buffer[1] == 'R') {
      *valid_len = 5; /* 5<->ERROR */
      return MEMCACHED_RC_PROTOCOL_ERROR;
    }
    else if (buffer[1] == 'X') {
      *valid_len = 6; /* EXISTS */
      return MEMCACHED_RC_DATA_EXISTS;
    }
    else {
      return MEMCACHED_RC_UNKNOWN_READ_FAILURE;
    }
  }
  case 'I': {/* CLIENT ERROR */
    /* We add back in one because we will need to search for END */
    cur = strstr(src, MEMCACHED_END_KEY);
    if (!cur)
      return MEMCACHED_RC_READ_PATIAL;
    *valid_len = cur-src+3;
    return MEMCACHED_RC_ITEM;
  }
  case 'C': { /* CLIENT ERROR */
    *valid_len = 12; /* 12<->CLIENT_ERROR */
    return MEMCACHED_RC_CLIENT_ERROR;
  }
  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1) {
	// only a number
	cur = strstr(src, MEMCACHED_END_ESCAPE);
	*valid_len = cur-src;
	return MEMCACHED_RC_SUCCESS;
      }

      return MEMCACHED_RC_UNKNOWN_READ_FAILURE;
    }
    /* NOTREACHED */
  }
}

MemcachedReplyDecoder::MemcachedReplyDecoder()
  : result_buf_(NULL)
  , result_buf_len_(0)
  , recved_buf_(NULL)
  , recved_buf_size_(0)
  , recved_buf_len_(0)
{
}

MemcachedReplyDecoder::~MemcachedReplyDecoder()
{
  if (result_buf_) {
    free(result_buf_);
    result_buf_ = NULL;
  }

  if (recved_buf_) {
    free(recved_buf_);
    recved_buf_ = NULL;
  }
}

int32_t MemcachedReplyDecoder::GetBuffer(void **buf, uint32_t *len)
{
  uint32_t ret = 1;
	
  result_buf_len_ = MEMCACHED_MAX_BUFFER;
  result_buf_ = (char*)malloc(sizeof(char)*result_buf_len_);
  if (!result_buf_)
    return ret;
  *buf = result_buf_;
  *len = result_buf_len_;
  ret = 0;

  return ret;
}

void MemcachedReplyDecoder::FreeMemcachedReply(void* data, void* ctx)
{
  AVOID_UNUSED_VAR_WARN(data);
  AVOID_UNUSED_VAR_WARN(ctx);

}

int32_t MemcachedReplyDecoder::Feed(void* buf, uint32_t length, std::vector<InMsg>* msgs)
{
  int32_t len = 0;

  // error here
  if (0 == length)
    return -1;

  if (!recved_buf_) {
    recved_buf_size_ = length>MEMCACHED_MAX_BUFFER ?  length+1 : MEMCACHED_MAX_BUFFER;
    recved_buf_ = (char*)malloc(sizeof(char)*recved_buf_size_);
    if (!recved_buf_)
      return -2;
  }

  if (length > recved_buf_size_ - recved_buf_len_ -1) {
    recved_buf_size_ += length + 10;
    recved_buf_ = (char*)realloc(recved_buf_, recved_buf_size_*sizeof(char));
    if (!recved_buf_)
      return -2;
  }

  memcpy(recved_buf_+recved_buf_len_, buf, length);
  recved_buf_len_ += length;

  memcached_result_code_t rc = _textual_parse_response((char*)recved_buf_, recved_buf_len_, &len);
	
  if (MEMCACHED_RC_UNKNOWN_READ_FAILURE == rc
      && MEMCACHED_RC_PROTOCOL_ERROR == rc) {
    return -3;
  }

  if (MEMCACHED_RC_READ_PATIAL != rc) {
    InMsg msg;
    msg.type = "memcachedReply";
    msg.data = recved_buf_;
    msg.fn_free = FreeMemcachedReply;
    msg.free_ctx = NULL;
    msg.bytes_length = len;
    msgs->push_back(msg);
    return 0;
  }

  return 1;
}
} // namespace blitz.
