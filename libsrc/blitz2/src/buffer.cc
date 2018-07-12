#include <blitz2/buffer.h>
#include <blitz2/buffer_writer.h>

namespace blitz2 {
Buffer::TDestroyFunc Buffer::s_default_destroy_func_ = Buffer::DestroyDeleteImpl;
int BufferWriter::Reserve(int hint_length) {
  buffer_.Alloc( hint_length );
  return 0;
}
int BufferWriter::Extend(int len) {
  if ( buffer_.capacity() > buffer_.length() + len ) {
    return 0;
  }
  DataBuffer new_buffer;
  new_buffer.Alloc( buffer_.length() + len );
  memcpy( new_buffer.mem(), buffer_.mem(), buffer_.length() );
  int orig_len = buffer_.length();
  buffer_.Clear();
  buffer_.Attach( new_buffer.mem(), new_buffer.capacity(),
      new_buffer.destroy_functor() );
  buffer_.length() = orig_len;
  new_buffer.Detach();
  return 0;
}


} // namespace blitz
