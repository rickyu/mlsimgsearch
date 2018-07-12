
#include <utils/logger.h>
#include <utils/string_util.h>
#include "blitz/framework.h"
#include "blitz/ihttp_common.h"

namespace blitz
{

/********************************************************/
/* iHttpResponse					*/
/********************************************************/
void iHttpResponse::Write(OutMsg *out)
{
	if( out ) {

		char line[32]; line[0] = '\0';
		int len = 0;
		BufferWriter writer;
		writer.Init(m_content_length + 1024);

 		len = sprintf(line,"HTTP/1.%d %d %s\r\n",m_version,m_code,GetCodeStr(m_code));

 		writer.WriteData(line,len);

  		ParamHashIter it = m_params.begin();
		while ( m_params.end() != it ) {

			writer.WriteData(it->first.c_str(),it->first.length());
			writer.WriteData(": ",2);
			writer.WriteData(it->second.c_str(),it->second.length());
			writer.WriteData("\r\n",2);
			++it;
		}

		if( 0 < m_content_length ) {

			writer.WriteData("Content-Length: ",16);
			writer.PrintInt(m_content_length,'\r');
			writer.WriteData("\n",1);
		}

		writer.WriteData("\r\n",2);

		if( 0 < m_content_length ) {

			writer.WriteData(m_content.c_str(),m_content_length);
		}
		out->AppendBuffers(writer.get_buffers(),writer.get_buffer_count());
		writer.DetachBuffers();
	}
}

/********************************************************/
/* iHttpReqDecoder					*/
/********************************************************/
int iHttpReqDecoder::GetBuffer(void **buf, uint32_t *length)
{
	if( !m_buf ) {
		
		m_buf_size = HTTP_BUF_SIZE;
    		m_buf = new char[m_buf_size + 4];
    		assert(m_buf);
	}

	if( !m_request ) {

		m_request = new iHttpRequest();
		assert(m_request);
	}

	switch( m_status ) {

		case kFirstLine:
		case kHeader:
			*buf = m_buf + m_buf_length;
			*length = m_buf_size - m_buf_length;
			break;
		case kBody:
			*buf = m_body  + m_body_read;
			*length = m_body_length - m_body_read;
			break;
		case kChunkSize:
		case kChunkData:
		case kChunkTrailer:
		{
			// [chunk-size] \r\n[chunk-data]\r\n0\r\n 
			int buf_size = HTTP_BUF_SIZE + m_chunk_left;
			int left = m_buf_size - m_buf_length;
			if( buf_size > left ) {

				char *buf = new char[buf_size + 4];
				assert(buf);
		  		m_buf_size = buf_size;
				memcpy(buf,m_buf+m_line_start,m_buf_length-m_line_start);
				delete [] m_buf;
				m_buf = buf;
				m_parse_pos = m_buf_length - m_parse_pos;
				m_buf_length = m_buf_length - m_line_start;
				m_line_start = 0;
			}
			*buf = m_buf + m_buf_length;
			*length = m_buf_size - m_buf_length;
			break;
		}
		default:
			return -1;
	}
	return 0;
}

int iHttpReqDecoder::Feed(void *buf, uint32_t length, std::vector<InMsg> *msg)
{
	int res = -1;

	switch( m_status ) {

		case kFirstLine: 
			if( m_buf + m_buf_length == reinterpret_cast<char*>(buf) ) {

				m_buf_length += length;
				res = ParseFirstLine();
			}
			break;
		case kHeader:
			if( m_buf + m_buf_length == reinterpret_cast<char*>(buf) ) {

				m_buf_length += length;
				res = ParseHeader();
			}
			break;
		case kBody:
			if( m_body + m_body_read == reinterpret_cast<char*>(buf) ) {

				m_body_read += length;
				res = ReadBody();
			}
			break;
		case kChunkSize:
			if( m_buf + m_buf_length == reinterpret_cast<char*>(buf) ) {

				m_buf_length += length;
				res = ReadChunkSize();
			}
			break;
		case kChunkData:
		case kChunkTrailer:
			if( m_buf + m_buf_length == reinterpret_cast<char*>(buf) ) {

				m_buf_length += length;
				res = ReadChunkData();
			}
			break;
		default:
			break;
	}

	switch( res ) {

		case 0:
			break;
		case 1:
		{
			if( 0 < m_body_read && m_body ) {

				m_request->SetContentLength(m_body_read);
				m_request->SetContent(m_body);
			}
			InMsg one_msg;
			one_msg.type = "HttpRequest";
			one_msg.data = m_request;
			one_msg.fn_free = DeleteRequest;
			one_msg.free_ctx = NULL;
			one_msg.bytes_length = 0;
			msg->push_back(one_msg);
			m_request = NULL;
			Reset();			
			res = 0;
			break;
		}
		default:
			Reset();
			break;
	}
	return res;
}

int iHttpReqDecoder::ParseFirstLine()
{
	int res = SearchEOL();
	if( 0 == res ) {

		int line_len = m_line_end - m_line_start;
		char *line = m_buf + m_line_start;
		line[m_line_end] = '\0';
		m_line_start = m_parse_pos;

		int uri_start = 4;
		if( 0 == strncasecmp(line,"GET ",4) ) {

			m_request->SetMethod(iHttpRequest::METHOD_E::HTTP_GET);

		} else if( 0 == strncasecmp(line,"POST ",5) ) {

			m_request->SetMethod(iHttpRequest::METHOD_E::HTTP_POST);
			uri_start = 5;

		} else {

			return -1;
		}
		m_request->SetRequestLine(line,line_len);

		char *uri = line + uri_start;
		char *ver = strchr(uri,' ');
		if( NULL == ver ) {

			return -1;
		}
		m_request->SetUri(uri,ver-uri);

		if( '0' == line[line_len-1] ) {

			m_request->SetVersion(0);

		} else {

			m_request->SetVersion(1);
		}
		return ParseHeader();

	} else {

		if( NULL == strstr(m_buf,"HTTP/") ) {

			return -1;
		}
	}
	return 0;
}

int iHttpReqDecoder::ParseHeader()
{
	while( 1 ) {

		int res = SearchEOL();

		if( 0 == res ) {

			int line_len = m_line_end - m_line_start;
			char *line = m_buf + m_line_start;
			m_line_start = m_parse_pos;

			if( 0 == line_len ) {

				const char *str = m_request->GetHeader("Transfer-Encoding");
				if( str && 0 == strcasecmp(str,"chunked") ) {

					m_status = kChunkSize;
					return ReadChunkSize();
				}

				str = m_request->GetHeader("Content-Length");
				if( str ) {

					m_request->SetContentLength(atoi(str));
				}

				int body_length = m_request->GetContentLength();
				if( 0 < body_length ) {

					m_body = new char[body_length + 4];
					m_body_length = body_length;
					m_body_read = 0;
					int left_length = m_buf_length - m_parse_pos;
					if( 0 > left_length || m_body_length < left_length ) {

						return -1;
					}
					memcpy(m_body,m_buf + m_parse_pos,left_length);
					m_body_read = left_length;
					m_status = kBody;
					return ReadBody();

				} else {

					return 1;
				}
				return 0;

			} else {

				m_buf[m_line_end] = '\0';
				char* val = strchr(line,':');
				if( NULL == val ) {

					return -1;
				}
				*val = '\0';
				++val;
				while( ' ' == *val ) {

					++val;
				}
				m_request->SetHeader(line,val);
			}

		} else {

			return 0;
		}
	}
}

int iHttpReqDecoder::ReadBody()
{
	if( m_body_read == m_body_length ) {

		m_body[m_body_read] = '\0';
		return 1;

	} else if( m_body_read > m_body_length ) {

		return -1;
	}
	return 0;
}

int iHttpReqDecoder::ReadChunkSize()
{
	int res = SearchEOL();
	if( 0 == res ) {

		char *line = m_buf + m_line_start;
		// chunk size
		m_chunk_size = static_cast<int>(strtol(line,NULL,16));
		if( 0 == m_chunk_size ) {

			return 1;
		}
		m_chunk_left = m_chunk_size;
		m_status = kChunkData;
		return ReadChunkData();

	} else {

		return 0;
	}
}

int iHttpReqDecoder::ReadChunkData()
{
	int left_length = m_buf_length - m_parse_pos;
	if( left_length >= m_chunk_left ) {

		AppendBody(m_buf + m_parse_pos,m_chunk_left);
		m_parse_pos += m_chunk_left;
		m_chunk_left = 0;
		m_status = kChunkTrailer;
		return ReadChunkTrailer();

	} else {

		AppendBody(m_buf + m_parse_pos,left_length);
		m_chunk_left -= left_length;
		m_parse_pos = 0;
		m_buf_length = 0;
		m_line_start = 0;
		return 0;
	}
}

int iHttpReqDecoder::ReadChunkTrailer()
{
	bool bRL = false;
	while( '\r' == m_buf[m_parse_pos] || '\n' == m_buf[m_parse_pos] ) {

		if( m_buf_length > m_parse_pos ) {

			++m_parse_pos;

		} else {

			bRL = true;
			break;
		}
	}
	if( m_buf_length == m_parse_pos ) {

		if( bRL ) {

			m_buf_length = 0;
			m_parse_pos = 0;
			m_line_start = 0;

		} else {

			m_buf[0] = m_buf[m_buf_length - 1];
			m_buf_length = 1;
	    		m_parse_pos = 0;
	    		m_line_start = 0;
		}
		return 0;
	}
	m_line_start = m_parse_pos;
	m_status = kChunkSize;
	return ReadChunkSize();
}

int iHttpReqDecoder::SearchEOL()
{
	while( 1 ) {

		while( m_parse_pos < m_buf_length && '\r' != m_buf[m_parse_pos] )  {

			++m_parse_pos;
		}

		if( m_parse_pos == m_buf_length ) {

			return 1;
		}

		if( m_parse_pos < m_buf_length - 1 ) {

			if( '\n' == m_buf[m_parse_pos + 1] ) {

				m_line_end = m_parse_pos;
				m_parse_pos += 2;
				return 0;

			} else {

				m_parse_pos += 2;
			}

		} else {

			return 1;
		}
	}
}

void iHttpReqDecoder::AppendBody(const void *data, int length)
{
	if( data ) {

		if( NULL == m_body ) {

			m_body_length = length;
			m_body = new char[m_body_length + 4];
			assert(m_body);
			m_body_read = 0;

		} else if( m_body_read + length > m_body_length ) {

			m_body_length = m_body_read + length;
			char *body = new char[m_body_length + 4];
			assert(body);
			memcpy(body,m_body,m_body_read);
			delete [] m_body;
			m_body = body;
		}
		memcpy(m_body + m_body_read,data,length);
		m_body_read += length;
		m_body[m_body_read] = '\0';
	}
}

} /* namespace blitz. */

