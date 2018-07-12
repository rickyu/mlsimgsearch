#ifndef __BLITZ3_MYSQLNET_HPP
#define __BLITZ3_MYSQLNET_HPP
#include <stdint.h>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <mysql.h>
#include <my_global.h>
#include <mysqld_error.h>
#include "mysql_error.hpp"

static const int  MAX_PACKET_LENGTH  = (256L*256L*256L-1);
struct MysqlHeader {
  unsigned  int packet_number() const { return buff[3];}
  unsigned int packet_len() const { return uint3korr(buff);}

  uint8_t buff[NET_HEADER_SIZE + COMP_HEADER_SIZE];
  uint32_t len;
};
class DataBuffer {
 public:
  DataBuffer() {
    buff_ = NULL;
    capacity_ = 0;
    len_ = 0;
  }
  ~DataBuffer() {
    if (buff_) {
      free(buff_);
      buff_ = NULL;
    }
    capacity_ = 0;
    len_ = 0;
  }
 private:
  DataBuffer(const DataBuffer&);
  DataBuffer& operator=(const DataBuffer&);
 public:
  void Extend(size_t more) {
    if (len_ + more > capacity_) {
      capacity_ = len_ + more;
      uint8_t* old_buff = buff_;
      buff_ = (uint8_t*)malloc(capacity_);
      memcpy(buff_, old_buff, len_);
      free(old_buff);
    }
  }
  uint8_t* buff() { return buff_ ; }
  const uint8_t* buff() const { return buff_  ; }
  uint8_t* read_pos() { return buff_ + len_ ; }
  const uint8_t* read_pos() const { return buff_ + len_ ; }
  size_t capacity() const { return capacity_; }
  size_t& len() { return len_; }
  size_t len() const { return len_; }

 private:
  uint8_t *buff_;
  size_t capacity_;
  size_t len_;

};
/**
 * 封装基本的packet收发功能.
 */

class MysqlNet {
 public:
  MysqlNet(boost::asio::io_service& io) : socket_(io) {
    compress_ = false;
    compress_pkt_nr_ = 0;
    pkt_nr_ = 0;
  }
  boost::asio::ip::tcp::socket& socket() {
    return socket_;
  }
  template<typename Handler>
  void ReadOne(MysqlHeader* header, DataBuffer* data, Handler handler);
  template<typename Handler>
  void ReadPacket(DataBuffer* data, Handler handler);
  template<typename Handler>
  void WritePacket(const uint8_t* packet, size_t len, Handler handler);
  
 private:
  template<typename Handler>
  class ReadOneCo: public boost::asio::coroutine {
   public:
    ReadOneCo() {
      net_ = NULL;
      header_ = NULL;
      data_ = NULL;
      handler_ = Handler();
    }
    ReadOneCo(MysqlNet* net, MysqlHeader* header, DataBuffer* data, 
             Handler handler) {
      net_ = net;
      header_ = header;
      data_ = data;
      handler_ = handler;
    }
    void operator()(const boost::system::error_code& ec, std::size_t sz);
    void Exit(const boost::system::error_code& ec, std::size_t sz) {
      net_->reading_or_writing_ = 0;
      net_->compress_pkt_nr_ = ++net_->pkt_nr_;
      handler_(ec, sz);

    }
    MysqlNet* net_;
    MysqlHeader* header_;
    DataBuffer* data_;
    Handler handler_;
    
  };
  template<typename Handler>
  class ReadPacketCo: public boost::asio::coroutine {
   public:
    ReadPacketCo() {
      net_ = NULL;
      data_ = NULL;
      handler_ = Handler();
    }
    ReadPacketCo(MysqlNet* net,  DataBuffer* data, 
             Handler handler) {
      net_ = net;
      data_ = data;
      handler_ = handler;
    }
    void operator()(const boost::system::error_code& ec, std::size_t sz);
    void Exit(const boost::system::error_code& ec, std::size_t sz) {
      handler_(ec, sz);
    }
    MysqlNet* net_;
    MysqlHeader header_;
    DataBuffer* data_;
    Handler handler_;
  };
  template<typename Handler>
  class WritePacketCo: public boost::asio::coroutine {
   public:
    WritePacketCo() {
      net_ = NULL;
      write_pos_ = NULL;
      len_ = 0;
    }
    WritePacketCo(MysqlNet* net, const uint8_t* packet,
         size_t packet_len, Handler handler) 
      : handler_(handler)
    {
      net_ = net;
      len_ = packet_len;
      write_pos_ = packet;
    }
   public:
    void operator()(const boost::system::error_code& ec, std::size_t size);
   private:
    void Exit(const boost::system::error_code& ec, std::size_t sz) {
      net_->reading_or_writing_ = 0;
      handler_(ec, sz);
    }
   private:
    MysqlNet* net_;
    uint8_t header_[NET_HEADER_SIZE];
    const uint8_t* write_pos_;
    size_t len_;
    Handler handler_;
  };


 private:
  boost::asio::ip::tcp::socket socket_;
  boost::system::error_code last_error_;
  bool compress_;
  int reading_or_writing_;
  unsigned int compress_pkt_nr_;
  unsigned int pkt_nr_;
};

template<typename Handler>
inline void MysqlNet::ReadOne(MysqlHeader* header,
                   DataBuffer* data, Handler handler) {
  reading_or_writing_ = 1;
  ReadOneCo<Handler>(this, header, data, handler)(boost::system::error_code(), 0);
}
template<typename Handler>
inline void MysqlNet::ReadPacket(DataBuffer* data, Handler handler) {
  ReadPacketCo<Handler>(this, data, handler)(boost::system::error_code(), 0);
}
template<typename Handler>
inline void MysqlNet::WritePacket(const uint8_t* packet, size_t len, Handler handler) {
  reading_or_writing_ = 1;
  WritePacketCo<Handler>(this, packet, len, handler)(boost::system::error_code(), 0);
}

template<typename Handler>
void MysqlNet::ReadOneCo<Handler>::operator()(const boost::system::error_code& ec,
                                       std::size_t size) {
  using boost::system::error_code;
  reenter(*this) {
    // read header
    header_->len  = net_->compress_ ? NET_HEADER_SIZE+COMP_HEADER_SIZE :
                  NET_HEADER_SIZE;
    yield boost::asio::async_read(net_->socket_,
        boost::asio::buffer(header_->buff, header_->len), 
        *this);
    if (ec) {
      Exit(ec, 0);
      return;
    }
    // check header and extend buffer
    if (header_->packet_number() != net_->pkt_nr_) {
      Exit(error_code(ER_NET_PACKETS_OUT_OF_ORDER, GetMysqlCategory()), 0);
      return;
    }
    if (!header_->packet_len()) {
      // 包大小为0
      data_->len() = 0;
      Exit(error_code(), size);
      return;
    }
    //分配缓冲区
    data_->Extend(header_->packet_len());
    yield async_read(net_->socket_, 
        boost::asio::buffer(data_->read_pos(), header_->packet_len()), 
        *this);
    if (ec) {
      //fail to recv packet data
      Exit(ec, size);
      return;
    }
    // succeed
    data_->len() += header_->packet_len();
    Exit(error_code(), 0);
  }
}
template<typename Handler>
void MysqlNet::ReadPacketCo<Handler>::operator()(const boost::system::error_code& ec,
                                   std::size_t size) {
  // logic from mysql::my_net_read.
  reenter (*this) {
  if (!net_->compress_) {
    yield net_->ReadOne(&header_, data_, *this);
    if (ec) {
      Exit(ec, size);
      return;
    }
    if (header_.packet_len() == MAX_PACKET_LENGTH) {
      /* First packet of a multi-packet.  Concatenate the packets */
      do
      {
	yield net_->ReadOne(&header_, data_, *this);
        if (ec) {
          Exit(ec, size);
          return;
        }
      } while (header_.packet_len() == MAX_PACKET_LENGTH);
    }
    *(data_->buff() + data_->len() ) = 0;
    Exit(boost::system::error_code(), 0);
  }
  else
  {
    assert(0);
#if 0
    /* We are using the compressed protocol */
    ulong buf_length;
    ulong start_of_packet;
    ulong first_packet_offset;
    uint read_length, multi_byte_packet=0;

    if (net->remain_in_buf)
    {
      buf_length= net->buf_length;		/* Data left in old packet */
      first_packet_offset= start_of_packet= (net->buf_length -
					     net->remain_in_buf);
      /* Restore the character that was overwritten by the end 0 */
      net->buff[start_of_packet]= net->save_char;
    }
    else
    {
      /* reuse buffer, as there is nothing in it that we need */
      buf_length= start_of_packet= first_packet_offset= 0;
    }
    for (;;)
    {
      ulong packet_len;

      if (buf_length - start_of_packet >= NET_HEADER_SIZE)
      {
	read_length = uint3korr(net->buff+start_of_packet);
	if (!read_length)
	{ 
	  /* End of multi-byte packet */
	  start_of_packet += NET_HEADER_SIZE;
	  break;
	}
	if (read_length + NET_HEADER_SIZE <= buf_length - start_of_packet)
	{
	  if (multi_byte_packet)
	  {
	    /* Remove packet header for second packet */
	    memmove(net->buff + first_packet_offset + start_of_packet,
		    net->buff + first_packet_offset + start_of_packet +
		    NET_HEADER_SIZE,
		    buf_length - start_of_packet);
	    start_of_packet += read_length;
	    buf_length -= NET_HEADER_SIZE;
	  }
	  else
	    start_of_packet+= read_length + NET_HEADER_SIZE;

	  if (read_length != MAX_PACKET_LENGTH)	/* last package */
	  {
	    multi_byte_packet= 0;		/* No last zero len packet */
	    break;
	  }
	  multi_byte_packet= NET_HEADER_SIZE;
	  /* Move data down to read next data packet after current one */
	  if (first_packet_offset)
	  {
	    memmove(net->buff,net->buff+first_packet_offset,
		    buf_length-first_packet_offset);
	    buf_length-=first_packet_offset;
	    start_of_packet -= first_packet_offset;
	    first_packet_offset=0;
	  }
	  continue;
	}
      }
      /* Move data down to read next data packet after current one */
      if (first_packet_offset)
      {
	memmove(net->buff,net->buff+first_packet_offset,
		buf_length-first_packet_offset);
	buf_length-=first_packet_offset;
	start_of_packet -= first_packet_offset;
	first_packet_offset=0;
      }

      net->where_b=buf_length;
      if ((packet_len = my_real_read(net,&complen)) == packet_error)
	return packet_error;
      if (my_uncompress(net->buff + net->where_b, packet_len,
			&complen))
      {
	net->error= 2;			/* caller will close socket */
        net->last_errno= ER_NET_UNCOMPRESS_ERROR;
	return packet_error;
      }
      buf_length+= complen;
    }

    net->read_pos=      net->buff+ first_packet_offset + NET_HEADER_SIZE;
    net->buf_length=    buf_length;
    net->remain_in_buf= (ulong) (buf_length - start_of_packet);
    len = ((ulong) (start_of_packet - first_packet_offset) - NET_HEADER_SIZE -
           multi_byte_packet);
    net->save_char= net->read_pos[len];	/* Must be saved */
    net->read_pos[len]=0;		/* Safeguard for mysql_use_result */
#endif
  }
  }
}
template<typename Handler>
void MysqlNet::WritePacketCo<Handler>::operator()(
    const boost::system::error_code& ec, std::size_t size) {
  reenter(*this) {
    /*
      Big packets are handled by splitting them in packets of MAX_PACKET_LENGTH
      length. The last packet is always a packet that is < MAX_PACKET_LENGTH.
      (The last packet may even have a length of 0)
    */
    while (len_ >= MAX_PACKET_LENGTH) {
      int3store(header_, MAX_PACKET_LENGTH);
      header_[3]= (uint8_t) net_->pkt_nr_++;
      yield  {
        boost::array<boost::asio::const_buffer, 2> buffers;
        buffers[0] = boost::asio::const_buffer(header_, sizeof(header_));
        buffers[1] = boost::asio::const_buffer(write_pos_, MAX_PACKET_LENGTH);
        async_write(net_->socket_, buffers, *this);
      }
      if (ec) {
        Exit(ec, 0);
        return;
      }
      write_pos_ += MAX_PACKET_LENGTH;
      len_ -=     MAX_PACKET_LENGTH;
    }
    /* Write last packet */
    int3store(header_, len_);
    header_[3]= (uint8_t) net_->pkt_nr_++;
    yield {
      boost::array<boost::asio::const_buffer, 2> buffers;
      buffers[0] = boost::asio::const_buffer(header_, sizeof(header_));
      buffers[1] = boost::asio::const_buffer(write_pos_, len_);
      async_write(net_->socket_, buffers, *this);
    }
    if (ec) {
      Exit(ec, 0);
      return;
    }
    write_pos_ += len_;
    len_ = 0;
    Exit(ec, 0);
  }
}


#endif
