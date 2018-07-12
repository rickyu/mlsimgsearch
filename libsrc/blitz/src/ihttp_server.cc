
#include <protocol/TJSONProtocol.h>
#include <utils/time_util.h>
#include <utils/logger.h>
#include "blitz/ihttp_server.h"
#include "utils/compile_aux.h"

namespace blitz
{

static CLogger &g_log_http = CLogger::GetInstance("http_access",CLogger::LEVEL_DEBUG);
static CLogger &g_log_pro = CLogger::GetInstance("protocol",CLogger::LEVEL_DEBUG);

/********************************************************/
/* iHttpHandler						*/
/********************************************************/
struct iHttpHandler
{
	iHttpHandler(iHttpServer *server, const IoInfo &info, const InMsg &msg, uint64_t timeout, unsigned int num)
	{
		m_server = server;
		m_info = info;		
		m_msg = msg;		
		m_timeout = timeout;
		m_num = num;
	}

	void operator()()
	{
		if( 0 < m_timeout ) {

			uint64_t timecur = TimeUtil::GetTickMs();
			if( m_timeout < timecur ) {

				if( m_msg.fn_free ) {

					m_msg.fn_free(m_msg.data,m_msg.free_ctx);
				}
				return;
			}
		}
		m_server->ProcessMsg(m_info,m_msg,m_num);
	}
	iHttpServer *m_server;
	IoInfo m_info;
	InMsg m_msg;
	uint64_t m_timeout;
	unsigned int m_num;
};

/********************************************************/
/* iHttpServer						*/
/********************************************************/
int iHttpServer::Bind(const char *addr)
{
	return m_framework.Bind(addr,&m_decoder_factory,this,NULL);
}

int iHttpServer::ProcessMsg(const IoInfo &info, const InMsg &msg)
{
	uint64_t timeout = 0;
	if( 0 < m_timeout ) {
 
		timeout = TimeUtil::GetTickMs() + m_timeout;
	}
	unsigned int num = __sync_fetch_and_add(&m_num,1);
	num %= m_thread_count;
	iHttpHandler hand(this,info,msg,timeout,num);
  	m_services[num]->post(hand);
	return 1;
}

static int
__generate_random_int() // dummy random
{
  struct timeval cur_time;
  gettimeofday(&cur_time, 0);
  return cur_time.tv_usec + pthread_self() % 1000000 ;
}

int iHttpServer::ProcessMsg(const IoInfo &info, const InMsg &msg, int num)
{
	char error[256]; error[0] = '\0';
	char *out = NULL;
	char *content = NULL;
	int outlen = 0, code = 200;

	std::string http_line = "-", http_content = "-", http_header = "-", http_error = "-";

	iHttpConnection conn;
	conn.m_io = info.get_id();
	conn.m_keep_alive = false;
	conn.m_version = 1;

	uint64_t timeout = 0;
	if( 0 < m_timeout ) {

		timeout = TimeUtil::GetTickMs() + m_timeout;
	}

	iHttpRequest *req = reinterpret_cast<iHttpRequest *>(msg.data);
  int loggerID = __generate_random_int();
	if( NULL == req ) {

		snprintf(error,255,HTTP_REQUEST_ERRO,100101,"server-side exception.");
		code = 500;

	} else {

		const char *line = req->GetRequestLine().c_str();
                if( NULL != line && '\0' != line[0] ) {

			http_line = line;
		}

		const char *header = req->GetHeader(HTTP_REQUEST_HEAD);
		if( NULL == header || '\0' == header[0] ) {

			snprintf(error,255,HTTP_REQUEST_ERRO,100001,"{http-header}: Meilishuo field is empty.");
			code = 404;

		} else {

			http_header = header;

			switch( req->get_method() ) {

				case iHttpRequest::METHOD_E::HTTP_GET:
					content = (char *)req->GetUriParams().c_str();
          // Get can be empty
					//if( NULL == content || '\0' == content[0] ) {
					//	
					//	snprintf(error,255,HTTP_REQUEST_ERRO,100002,"GET {http-param}: is empty.");
					//	code = 404;
					//}
					break;
				case iHttpRequest::METHOD_E::HTTP_POST:
					content = (char *)req->GetContent().c_str();
					if( 0 == req->GetContentLength() ) {

						snprintf(error,255,HTTP_REQUEST_ERRO,100003,"POST {http-header}: Content-Length field is empty.");
						code = 404;
					}
					break;
				default:
					snprintf(error,255,HTTP_REQUEST_ERRO,100004,"unknown http request.");
					code = 404;
					break;
			}

			if( 200 == code ) {

				const char *option = req->GetUriHandle().c_str();
				http_content = content;
        LOGGER_DEBUG(g_log_http,"[id=%d] [%s] [%s]", loggerID, http_header.c_str(), http_content.c_str());

				try {

					LOGGER_DEBUG(g_log_pro,"[HTTP-SERVER] -> processing http request.");
					code = m_processor[num]->Process(option,header,content,&out,&outlen,error);

				} catch( const std::string &str ) {

					snprintf(error,255,HTTP_REQUEST_ERRO,100102,"server-side exception.");
					code = 500;

				} catch( std::exception &e ) {

					snprintf(error,255,HTTP_REQUEST_ERRO,100103,"server-side exception.");
					code = 500;

				} catch(...) {

					snprintf(error,255,HTTP_REQUEST_ERRO,100104,"server-side exception.");
					code = 500;
				}

				if( 200 == code ) {

					if( NULL == out || '\0' == out[0] ) {

						snprintf(error,255,HTTP_REQUEST_ERRO,100105,"server-side exception.");
						code = 500;
					}
				}
			}
		}
	}

	iHttpResponse resp;
	resp.SetCode(code);
	switch( code ) {

		case 200:
			resp.SetContent(out);
			resp.SetContentLength(outlen);
			break;
		case 404:
		case 500:
		default:
			resp.SetContent(error);
			resp.SetContentLength(strlen(error));
			break;
	}
	iHttpProcHandler::Delete(out);
	SendResponse(conn,&resp);
	if( msg.fn_free ) {

		msg.fn_free(msg.data,msg.free_ctx);
	}
	if( '\0' != error[0] ) {

		http_error = error;
	}
	LOGGER_DEBUG(g_log_http,"[id=%d] [%s] [%d] [%d] [%s] [%s]",loggerID,http_line.c_str(),code,outlen,http_header.c_str(),http_error.c_str());
	return 1;
}

int iHttpServer::SendResponse(const iHttpConnection &conn, iHttpResponse *resp)
{
	if( resp ) {

		if( conn.m_keep_alive ) {

			resp->SetHeader("Connection","keep-alive");

		} else {

			resp->SetHeader("Connection","close");
		}
		resp->SetVersion(conn.m_version);

		OutMsg *out = NULL;
		OutMsg::CreateInstance(&out);
		resp->Write(out);
		m_framework.SendMsg(conn.m_io,out);
		out->Release();
		if( 200 != resp->GetCode() ) {

			m_framework.CloseIO(conn.m_io);
		}
		return 1;
	}
	return 0;
}

int iHttpServer::Start(int thread_count)
{
	m_thread_count = thread_count;

	for( int i = 0; thread_count > i; ++i ) {

		boost::asio::io_service *io = new boost::asio::io_service();
		m_services.push_back(io);
  	}

	for( int i = 0; thread_count > i; ++i ) {

		boost::asio::io_service::work *work = new boost::asio::io_service::work(*m_services[i]);
		m_works.push_back(work);
		m_threads.create_thread(boost::bind(&boost::asio::io_service::run,m_services[i]));
	}
	return 1;
}

void iHttpServer::Stop()
{
	for( int i = 0; (int)m_thread_count > i; ++i ) {

		m_services[i]->stop();
		delete m_works[i];
		delete m_services[i];
	}
	m_threads.join_all();
}

} /* namespace blitz. */

