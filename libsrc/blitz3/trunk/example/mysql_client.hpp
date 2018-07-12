#ifndef __MYSQL_CLIENT_HPP
#define __MYSQL_CLIENT_HPP

class MysqlClient;
class MyPacketWriteCoroutine: public boost::asio::coroutine {
 public
  MyPacketWriteCoroutine(tcp::socket& socket) 
    : socket_(socket), 
  {
    packet_ = NULL;
    packet_len_ = 0;
    write_pos_ = NULL;
    len_ = 0;
    pkt_nr_ = 0;
  }
  template<typename Handler>
  void Start(uchar* packet, size_t packet_len, Handler handler) {
    packet_ = packet;
    packet_len_ = packet_len;
    write_pos_ = packet_;
    len_ = packet_len_;
    pkt_nr_ = pkt_nr;
    handler_ = handler;
    LoopWrite(boost::system::error_code(), 0);
  }
 private:
  void WriteCoroutine(const boost::system::error_code& ec, std::size_t size) 
  void Exit(const boost::system::error_code& ec, std::size_t sz) {
    handler_(ec, sz);
  }
 private:
  uchar buff_[NET_HEADER_SIZE];
  uchar* packet_;
  size_t packet_len_;
  uchar* write_pos_;
  size_t len_;
  const_buffer buffers_[2];
  boost::function<void(const boost::system::error_code&, std::size_t)> handler_;
};

class MyPacketReadCoroutine: public boost::asio::coroutine {
 public
  MyPacketReadCoroutine(tcp::socket& socket) 
    : socket_(socket), 
  {
    packet_ = NULL;
    packet_len_ = 0;
    write_pos_ = NULL;
    len_ = 0;
    pkt_nr_ = 0;
  }
  template<typename Handler>
  void Start(uchar* packet, size_t packet_len, Handler handler) {
    packet_ = packet;
    packet_len_ = packet_len;
    write_pos_ = packet_;
    len_ = packet_len_;
    pkt_nr_ = pkt_nr;
    handler_ = handler;
    LoopWrite(boost::system::error_code(), 0);
  }
 private:
  void ReadCoroutine(const boost::system::error_code& ec, std::size_t size) 
  void Exit(const boost::system::error_code& ec, std::size_t sz) {
    handler_(ec, sz);
  }
 private:
  uchar buff_[NET_HEADER_SIZE];
  uchar* packet_;
  size_t packet_len_;
  uchar* write_pos_;
  size_t len_;
  const_buffer buffers_[2];
  boost::function<void(const boost::system::error_code&, std::size_t)> handler_;
};


class MyConnectCoroutine: public boost::asio::coroutine {
 public:
  MyConnectCoroutine(MysqlClient& client):client_(client) {
  }
  template<typename Handler>
  void Start(Handler handler) {
  }
 private:
  void ConnectRoutine(const boost::system::error_code& ec, std::size_t sz) 
 private:
  MysqlClient& client_;
};

class HelloPacket {
 public:
  unsigned int protocol_version;
  unsigned long thread_id;
  unsigned int server_language;
  unsigned int server_status;
  char scramble[SCRAMBLE_LENGTH+1];




  void Parse(const char* read_pos, size_t packet_len) {
    protocol_version_ =  resp_buf_[0];
    if (protocol_version_ != PROTOCOL_VERSION) {
      SetError(CR_VERSION_ERROR);
    }
    strmov(mysql->server_version,(char*) net->read_pos+1);
    end = strend(read_pos+1);
    thread_id_ = uint4korr(end+1);
    end += 5;
    strmake(scramble, end, SCRAMBLE_LENGTH_323);
    end += SCRAMBLE_LENGTH_323 + 1;
    if (pkt_length >= (uint) (end+1 - (char*) read_pos))
      server_capabilities=uint2korr(end);
    if (pkt_length >= (uint) (end+18 - (char*) read_pos))
    {
      /* New protocol with 16 bytes to describe server characteristics */
      server_language=end[2];
      server_status=uint2korr(end+3);
    }
    end+= 18;
    if (pkt_length >= (uint) (end + SCRAMBLE_LENGTH - SCRAMBLE_LENGTH_323 + 1 - 
                             (char *) read_pos))
      strmake(scramble+SCRAMBLE_LENGTH_323, end,
              SCRAMBLE_LENGTH-SCRAMBLE_LENGTH_323);
    else
      mysql->server_capabilities&= ~CLIENT_SECURE_CONNECTION;
  

  }
};
/**
 * 封装基本的packet收发功能.
 */
class MysqlNet {
 public:
  template<typename Handler>
  void ReadOnePacket(Handler handler);
  template<typename Handler>
  void ReadPacket(Handler handler);
  template<typename Handler>
  void WritePacket(const uchar* packet, size_t len, Handler handler);
 private:
  void ReadOnePacketCoroutine(const boost::system::error_code& ec,
                              std::size_t sz);
  void ExitReadOne(const boost::system::error_code& ec, 
                  std::size_t sz) {
    handler_(ec, sz);
    reading_or_writing_ = 0;
    
  }
 private:
  tcp::socket_ socket_;
  char header_[NET_HEADER_SIZE + COMP_HEADER_SIZE];
  size_t header_size_;
  unsigned int packet_size_;
  unsigned char* buff_; //缓冲区
  size_t buff_size_; // 缓冲区大小
  size_t data_size_; // 缓冲区已有数据大小.
  bool compress_;
  int reading_or_writing_;
  int error_;
  int last_errno_;
  boost::asio::coroutine read_one_co_;
  unsigned int compress_pkt_nr_;
  unsigned int pkt_nr_;
  
  
};
class MysqlClient {
 public:
  /**
   * code from cli_mysql_real_connect.
  */
  template<Handler handler>
  void Connect(Handler handler) {
    coconnect_.Start(handler);
  }

  template<Handler handler>
  void SendQuery();
  void set_mysql_error(int errcode, const char* sqlstate) {
    net->last_errno= errcode;
    strmov(net->last_error, ER(errcode));
    strmov(net->sqlstate, sqlstate);
  }

  template<Handler handler>
  void my_net_write(const uchar* packet, size_t len, Handler handler) {
    cowriter_.Start(packet, len, handler);
  }
  template<Handler handler>
  void read_packet(Handler handler) {
    coreader_.Start(handler);
  }

 private:
  ulong client_flag_;
  struct st_mysql_options options;
  char last_error[MYSQL_ERRMSG_SIZE]; /* error message */
  char sqlstate[SQLSTATE_LENGTH+1];
  MysqlPacketWriter cowriter_;
};

#endif
