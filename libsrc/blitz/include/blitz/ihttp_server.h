/********************************************************/
/*	Copyright (C) Meilishuo.com  			*/
/*	Project:	Http Server			*/
/*	Author:		zhanglizhao			*/
/*	Date:		2015_10_20			*/
/*	File:		ihttp_server.h			*/
/********************************************************/

#ifndef __BLITZ_IHTTP_SERVER_H_
#define __BLITZ_IHTTP_SERVER_H_

#include <boost/asio.hpp>
#include <TProcessor.h>
#include <vector>
#include "framework.h"
#include "ihttp_common.h"
#include "ihttp_handler.h"

namespace blitz
{

/********************************************************/
/* iHttpConnection					*/
/********************************************************/
struct iHttpConnection
{
	uint64_t m_io;
	bool m_keep_alive;
	int  m_version;
};

/********************************************************/
/* iHttpServer						*/
/********************************************************/
#define HTTP_REQUEST_HEAD		"Meilishuo"
#define HTTP_REQUEST_ERRO		"{\"error_code\":%d,\"message\":\"%s\"}"

class iHttpServer : public MsgProcessor
{
public:
	iHttpServer(Framework& framework, std::vector<iHttpProcHandler *> &processor)
			:m_processor(processor), m_framework(framework)
	{
		m_num = 0;
		m_thread_count = 16;
		m_timeout = 0;
	}
	int Bind(const char *addr);
	int ProcessMsg(const IoInfo &info, const InMsg &msg);
	int ProcessMsg(const IoInfo &info, const InMsg &msg, int num);
	int SendResponse(const iHttpConnection &conn, iHttpResponse *resp);
	int Start(int thread_count);
	void Stop();
	void SetTimeout(int timeout) { m_timeout = timeout; }
	int GetTimeout() const { return m_timeout; }
protected:
	std::vector<iHttpProcHandler *> m_processor;
	Framework& m_framework;
	SimpleMsgDecoderFactory<iHttpReqDecoder> m_decoder_factory;
	std::vector<boost::asio::io_service *> m_services;
	std::vector<boost::asio::io_service::work *> m_works;
	boost::thread_group m_threads;
	unsigned int m_num;
	unsigned int m_thread_count;
	unsigned int m_timeout;
};

}

#endif /* __BLITZ_IHTTP_SERVER_H_ */

