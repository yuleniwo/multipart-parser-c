/* Based on node-formidable by Felix Geisendörfer 
* Igor Afonov - afonov@gmail.com - 2012
* yuleniwo    - xzm2@qq.com - 2020
* MIT License - http://www.opensource.org/licenses/mit-license.php
*/
#ifndef _multipart_parser_h
#define _multipart_parser_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

typedef struct multipart_parser multipart_parser;
typedef struct multipart_parser_settings multipart_parser_settings;
typedef struct multipart_parser_state multipart_parser_state;

typedef int (*multipart_data_cb)(multipart_parser*, const char *at, size_t length);
typedef int (*multipart_notify_cb)(multipart_parser*);

struct multipart_parser_settings
{
	multipart_data_cb on_header_field;
	multipart_data_cb on_header_value;
	multipart_data_cb on_part_data;

	multipart_notify_cb on_part_data_begin;
	multipart_notify_cb on_headers_complete;
	multipart_notify_cb on_part_data_end;
	multipart_notify_cb on_body_end;
};

struct multipart_parser
{
	void* data;
	const multipart_parser_settings* settings;
	char* cache;
	int index;
	int boundary_len;
	unsigned char state;
	// Whether to call the callback function when the data length is 0.
	unsigned char call0;
	char buf[46];
};

int multipart_parser_init(multipart_parser*p, const char *boundary, 
	int boundary_len, const multipart_parser_settings* settings);

void multipart_parser_uninit(multipart_parser* p);

// reset boundary and state
int multipart_parser_reset(multipart_parser*p, const char *boundary, 
	int boundary_len);

size_t multipart_parser_execute(multipart_parser* p, const char *buf, size_t len);

void multipart_parser_set_data(multipart_parser* p, void* data);
void * multipart_parser_get_data(multipart_parser* p);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
