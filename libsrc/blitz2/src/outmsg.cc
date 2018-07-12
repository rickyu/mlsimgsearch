#include <blitz2/outmsg.h>
namespace blitz2 {
size_t NotSharedOutMsg::Convert2iovec(iovec* vec) {
  assert(vec!=NULL);
  size_t data_count = data_vec_.size();
  if (data_count == 0) {
    return 0;
  }
  MsgData* msg_data = &data_vec_[0];
  for (size_t d = 0; d < data_count; ++d) {
    vec[d].iov_base = const_cast<uint8_t*>(
        msg_data[d].mem() + msg_data[d].offset() );
    vec[d].iov_len =  static_cast<int>(msg_data[d].length() );
  }
  return static_cast<int>(data_count);
}
} // namespace blitz
