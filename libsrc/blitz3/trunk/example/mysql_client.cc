#include "mysql_client.hpp"

void MysqlPacketWriter::WriteCoroutine(const boost::system::error_code& ec, std::size_t size) {
  reenter(*this) {
    /*
      Big packets are handled by splitting them in packets of MAX_PACKET_LENGTH
      length. The last packet is always a packet that is < MAX_PACKET_LENGTH.
      (The last packet may even have a length of 0)
    */
    while (len_ >= MAX_PACKET_LENGTH) {
      int3store(buff_, MAX_PACKET_LENGTH);
      buff_[3]= (uchar) pkt_nr_++;
      buffers_[0] = const_buffer(buffer_, sizeof(buffer_));
      buffers_[1] = const_buffer(write_pos_, MAX_PACKET_LENGTH);
      yield async_write(socket_, buffers_, boost::bind(&MysqlPacketWriter::LoopWrite,this));
      if (ec) {
        Exit(ec, write_pos_ - packet_);
        return;
      }
      write_pos_ += MAX_PACKET_LENGTH;
      len -=     MAX_PACKET_LENGTH;
    }
    /* Write last packet */
    int3store(buff_,len_);
    buff_[3]= (uchar) pkt_nr_++;
    buffers_[0] = const_buffer(buffer_, sizeof(buffer_));
    buffers_[1] = const_buffer(write_pos_, len_);
    yield async_write(socket_, buffers_, boost::bind(,this));
    if (ec) {
      Exit(ec, write_pos_ - packet_);
      return;
    }
    len_ = 0;
    write_pos_ += len_;
    len_ = 0;
    Exit(ec, packet_len_);
  }
}


void MysqlNet::ReadOnePacketCouroutine(const boost::system::error_code& ec,
                                       std::size_t size) {
  reenter(read_one_co_) {
    // read header
    reading_or_writing = 1;
    header_size_  = compress_ ? NET_HEADER_SIZE+COMP_HEADER_SIZE :
                  NET_HEADER_SIZE;
    yield boost::asio::async_read(socket_, boost::asio::buffer(header_, header_size_), 
        boost::bind(&MysqlNet::ReadOnePacketCoroutine, this));
    if (ec) {
      error_ = 2;
      last_errno_ = ER_NET_FCNTL_ERROR;
      ExitReadOne(ec, size);
      return;
    }
    // check header and extend buffer
    if (header_[3] != pkt_nr_) {
      last_errno_ = ER_NET_PACKETS_OUT_OF_ORDER;
      ExitReadOne(ec, size);
      return;
    }
    compress_pkt_nr_ = ++pkt_nr_;
    packet_size_ = uint3korr(header);
    if (!packet_size_) {
      // 包大小为0
      ExitReadOne(boost::system::error_code(), size);
      return;
    }
    //分配缓冲区
    Reserve(packet_size_);
    yield async_read(socket_, boost::asio::buffer(buffer_ + data_size_, packet_size_), 
        boost::bind(&MysqlNet::ReadOnePacketCoroutine, this));
    if (ec) {
      //fail to recv packet data
      ExitReadOne(ec, size);
      return;
    }
    data_size_ += packet_size_;
    ExitReadOne(ec, size);
  }
}
void ReadCoroutine(const boost::system::error_code& ec, std::size_t size) {
  size_t len, complen;
  if (!net->compress) {
    yield ReadOnePacket(boost::bind(&MysqlNet::ReadCoroutine, this));
    if (ec) {

    }
    if (packet_size_ == MAX_PACKET_LENGTH) {
      /* First packet of a multi-packet.  Concatenate the packets */
      ulong save_pos = net->where_b;
      size_t total_length= 0;
      do
      {
	net->where_b += len;
	total_length += len;
	len = my_real_read(net,&complen);
      } while (len == MAX_PACKET_LENGTH);
      if (len != packet_error)
	len+= total_length;
      net->where_b = save_pos;
    }
    net->read_pos = net->buff + net->where_b;
    if (len != packet_error)
      net->read_pos[len]=0;		/* Safeguard for mysql_use_result */
    return len;
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
void MyConnectCoroutine::ConnectRoutine(const boost::system::error_code& ec, std::size_t sz) {
  reenter(*this) {
    yield client_.socket_.async_connect(endpoint(),
        boost::bind(&MyConnectCoroutine::ConnectRoutine, this, _1, _2));
    if (ec) {
      Exit(ec, sz);
      return;
    }
    // part1 : connection established, read and parse first packet.
    yield client_.ReadPacket(boost::bind(&MyConnectCoroutine::ConnectRoutine, this, _1, _2));
    if (ec) {
      client_.SetError();
      Exit(ec, sz);
    }
    hello_.Parse(resp_buf_, pkt_length_);
    if (options_.secure_auth && passwd[0] &&
        !(hello_->server_capabilities & CLIENT_SECURE_CONNECTION)) {
      client_.set_mysql_error(CR_SECURE_AUTH, unknown_sqlstate);
      Exit(, 0);
    }

    //if (mysql_init_character_set(mysql))
    //  goto error;
    client_flag_ |= options_.client_flag;
    client_flag_ |= CLIENT_CAPABILITIES;
    if (client_flag_  & CLIENT_MULTI_STATEMENTS)
      client_flag_ |= CLIENT_MULTI_RESULTS;
    client_flag_ |= CLIENT_CONNECT_WITH_DB;

    client_flag_ = ((client_flag_ &
		 ~(CLIENT_COMPRESS | CLIENT_SSL | CLIENT_PROTOCOL_41)) |
		(client_flag_ & hello_.server_capabilities));

    client_flag_ &= ~CLIENT_COMPRESS;
    if (client_flag_ & CLIENT_PROTOCOL_41) {
      /* 4.1 server and 4.1 client has a 32 byte option flag */
      int4store(buff,client_flag);
      int4store(buff+4, net->max_packet_size);
      buff[8]= (char) mysql->charset->number;
      bzero(buff+9, 32-9);
      end= buff+32;
    }
    else {
      int2store(buff,client_flag);
      int3store(buff+2,net->max_packet_size);
      end= buff+5;
    }
    strmake(end,user,USERNAME_LENGTH);          /* Max user name */
    end= strend(end) + 1;
    if (passwd[0])
    {
      if (mysql->server_capabilities & CLIENT_SECURE_CONNECTION)
      {
        *end++= SCRAMBLE_LENGTH;
        scramble(end, mysql->scramble, passwd);
        end+= SCRAMBLE_LENGTH;
      }
      else
      {
        scramble_323(end, mysql->scramble, passwd);
        end+= SCRAMBLE_LENGTH_323 + 1;
      }
    }
    else
      *end++= '\0';                               /* empty password */

    /* Add database if needed */
    if (db && (mysql->server_capabilities & CLIENT_CONNECT_WITH_DB))
    {
      end= strmake(end, db, NAME_LEN) + 1;
      mysql->db= my_strdup(db,MYF(MY_WME));
      db= 0;
    }
    yield packet_writer_->Start(buff, end-buff);
    
    /* Write authentication package */
    if (my_net_write(net, (uchar*) buff, (size_t) (end-buff)) || net_flush(net))
    {
      set_mysql_extended_error(mysql, CR_SERVER_LOST, unknown_sqlstate,
                               ER(CR_SERVER_LOST_EXTENDED),
                               "sending authentication information",
                               errno);
      goto error;
    }
  }
}


