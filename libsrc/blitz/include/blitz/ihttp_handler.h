/********************************************************/
/*	Copyright (C) Meilishuo.com  			*/
/*	Project:	Http Process Handler		*/
/*	Author:		zhanglizhao			*/
/*	Date:		2015_10_20			*/
/*	File:		ihttp_handler.h			*/
/********************************************************/

#ifndef __BLITZ_IHTTP_HANDLER_H_
#define __BLITZ_IHTTP_HANDLER_H_

#include "stdio.h"

namespace blitz
{

/********************************************************/
/* iHttpProcHandler					*/
/********************************************************/
class iHttpProcHandler
{
public:
	iHttpProcHandler() { }
	virtual ~iHttpProcHandler() { }
	
	static void Delete(char *data)
	{
		if( data ) {

			delete [] data;
			data = NULL;
		}
	}

	virtual int Process(const char *option, const char *header, const char *content, char **out, int *outlen, char *error) { return 200; }
};

}

#endif /* __BLITZ_IHTTP_HANDLER_H_ */
