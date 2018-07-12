/********************************************************/
/*	Copyright (C) Meilishuo.com  			*/
/*	Project:	Http Common			*/
/*	Author:		zhanglizhao			*/
/*	Date:		2015_10_20			*/
/*	File:		ihttp_common.h			*/
/********************************************************/

#ifndef __BLITZ_IHTTP_COMMON_H_
#define __BLITZ_IHTTP_COMMON_H_

#include <unordered_map>
#include "msg.h"
#include "utils/compile_aux.h"

namespace blitz
{

/********************************************************/
/* iHttpBase						*/
/********************************************************/
class iHttpBase
{
public:
	iHttpBase()
	{
		m_content_length = 0;
		m_version = 1;
	}
	virtual ~iHttpBase() { }

	void SetHeader(const char *key, const char *val)
	{
		if( key && val ) {

			m_params[key] = val;
		}
	}
	const char * GetHeader(const char *key)
	{
		if( key ) {

			ParamHashIter it = m_params.find(key);
	    		if( m_params.end() == it ) {

				return NULL;
			}
			return it->second.c_str();
		}
		return NULL;
  	}

	void SetContent(const char *content)
	{
		if( content ) {

			m_content = content;
		}
	}
  	const std::string & GetContent() const { return m_content; }
	
	void SetContentLength(int len) { m_content_length = len; }
	int GetContentLength() const { return m_content_length; }

  	void SetVersion( int ver ) { m_version = ver;}
	int GetVersion() const { return m_version;}

protected:
	typedef std::unordered_map<std::string, std::string> ParamHash;
	typedef ParamHash::iterator ParamHashIter;

protected:
	ParamHash m_params;
	std::string m_content;
	int m_content_length;
	int m_version;	 
};

/********************************************************/
/* iHttpRequest						*/
/********************************************************/
class iHttpRequest : public iHttpBase
{
public:
	typedef enum eMethod
	{
		HTTP_GET = 1,
		HTTP_POST
	} METHOD_E;

	iHttpRequest() { }
	virtual ~iHttpRequest() { }

	void SetUri(const char *uri, int len)
	{
		if( uri ) {

			m_uri.assign(uri,len);
			{
				char *str = (char *)m_uri.c_str();
				if( '\0' != str[0] ) {

					char *pun = strrchr(str,'/');
					if( pun ) {

						len -= ++pun - str;
						str = pun;
						pun = strchr(str,'?');
						if( pun ) {

							int hlen = pun - str;
							if( 0 < hlen ) {

								m_uri_handle.assign(str,hlen);
								m_uri_params.assign(++pun,len-hlen-1);
							}

						} else {

							m_uri_handle = str;
						}
					}
				}
			}
		}
	}
	const std::string & GetUri() const { return m_uri; }

	void SetUriHandle(const char *handle, int len)
	{
		if( handle ) {

			m_uri_handle.assign(handle,len);
		}
	}
	const std::string & GetUriHandle() const { return m_uri_handle; }

	void SetUriParams(const char *params, int len)
	{
		if( params ) {

			m_uri_params.assign(params,len);
		}
	}
	const std::string & GetUriParams() const { return m_uri_params; }

	void SetRequestLine(const char *line, int len)
	{
		if( line ) {

			m_req_line.assign(line,len);
		}
	}
	const std::string & GetRequestLine() const { return m_req_line; }

	void SetMethod(METHOD_E method) { m_method = method; }
	METHOD_E get_method() const { return m_method; };
 
protected:
	std::string m_uri;
	std::string m_uri_handle;
	std::string m_uri_params;
	std::string m_req_line;
	METHOD_E m_method;
};

/********************************************************/
/* iHttpResponse					*/
/********************************************************/
class iHttpResponse : public iHttpBase
{
public:
	iHttpResponse()
	{
		m_code = 200;
	}
	virtual ~iHttpResponse() { }

	void SetCode(int code) { m_code = code; }
	int GetCode() const { return m_code; }
	const char * GetCodeStr(int code)
	{
		switch(code) {

			case 200:
				return "OK";
      			case 404:
				return "Not Found";
			case 500:
			default:
				return "Internal Server Error";
		}
	}

	void Write(OutMsg *out);

private:
	int m_code;
};

/********************************************************/
/* iHttpReqDecoder					*/
/********************************************************/
#define HTTP_BUF_SIZE		4 * 1024	

class iHttpReqDecoder : public MsgDecoder
{
public:
	iHttpReqDecoder()
	{
		m_buf = NULL;
		m_body = NULL;		
		m_request = NULL;
		Reset();
	}

	~iHttpReqDecoder()
	{
		if( m_buf ) {

			delete [] m_buf;
			m_buf = NULL;
		}
		if( m_body ) {

			delete [] m_body;
			m_body = NULL;
		}
		if( m_request ) {

			delete m_request;
			m_request = NULL;
		}
	}

	virtual int GetBuffer(void **buf, uint32_t *length); 
	virtual int Feed(void *buf, uint32_t length, std::vector<InMsg> *msg);

	static void DeleteRequest(void *data, void *context)
	{
		AVOID_UNUSED_VAR_WARN(context);
		delete reinterpret_cast<iHttpRequest *>(data);
	}

protected:
	enum {
		kFirstLine = 1,
		kHeader,
		kBody,
		kChunkSize,
		kChunkData,
		kChunkTrailer
	};

	int SearchBlank();
	int SkipBlank(char* str);

	int SearchEOL();
	int ParseFirstLine();
	int ParseHeader();
	int ReadBody();
	int ReadChunkSize();
	int ReadChunkData();
	int ReadChunkTrailer();
	void Reset()
	{
		m_status = kFirstLine;
		m_body_length = 0;
		m_body_read = 0;
		m_buf_size = 0;
		m_buf_length = 0;
		m_line_start = 0;
		m_line_end = 0;
		m_parse_pos = 0;
		m_chunk_size = 0;
		m_chunk_left = 0;

		if( m_buf ) {

			delete [] m_buf;
			m_buf = NULL;
		}
		if( m_body ) {

			delete [] m_body;
			m_body = NULL;
		}
		if( m_request ) {

			delete m_request;
			m_request = NULL;
		}
	}
	void AppendBody(const void *data, int length);

private:
	iHttpRequest *m_request;
	int m_status;
	char *m_body;
	int m_body_length;
	int m_body_read;

	char *m_buf;
	int m_buf_size;
	int m_buf_length;
	int m_line_start;
	int m_line_end;
	int m_parse_pos;

	int m_chunk_size;
	int m_chunk_left;
};

}

#endif /* __BLITZ_IHTTP_COMMON_H_ */

