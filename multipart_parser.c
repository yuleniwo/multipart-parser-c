/* Based on node-formidable by Felix Geisendörfer
* Igor Afonov - afonov@gmail.com - 2012
* MIT License - http://www.opensource.org/licenses/mit-license.php
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "multipart_parser.h"

static void multipart_log(const char * format, ...)
{
#ifdef DEBUG_MULTIPART
	va_list args;
	va_start(args, format);

	fprintf(stderr, "[HTTP_MULTIPART_PARSER] %s:%d: ", __FILE__, __LINE__);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
#endif
}

#define NOTIFY_CB(FOR)                       \
	do {                                     \
		if (p->settings->on_##FOR != NULL && \
			p->settings->on_##FOR(p) != 0)   \
			return i;                        \
	} while (0)

#define EMIT_DATA_CB(FOR, ptr, len)                  \
	do {                                             \
		if (p->settings->on_##FOR != NULL &&         \
			p->settings->on_##FOR(p, ptr, len) != 0) \
			return i;                                \
	} while (0)


#define LF 10
#define CR 13

#define mp_boundary(mp) (&mp->cache[2])
#define mp_cache(mp) (mp->cache)

enum state
{
	s_uninitialized = 1,
	s_start,
	s_start_boundary,
	s_header_field_start,
	s_header_field,
	s_headers_almost_done,
	s_header_value_start,
	s_header_value,
	s_header_value_almost_done,
	s_part_data_start,
	s_part_data,
	s_part_data_almost_boundary,
	s_part_data_almost_end,
	s_part_data_end,
	s_part_data_final_hyphen,
	s_end
};

int multipart_parser_init(multipart_parser*p, const char *boundary,
        int boundary_len, const multipart_parser_settings* settings)
{
	if(boundary_len < 0)
		boundary_len = boundary != NULL ? (int)strlen(boundary) : 0;
	if(boundary_len <= (int)sizeof(p->buf) - 4)
		p->cache = p->buf;
	else
		p->cache = (char*)malloc(boundary_len + 4);
	if(NULL == p->cache)
		return -1;
	p->cache[0] = CR;
	p->cache[1] = LF;
	p->cache[2] = '-';
	p->cache[3] = '-';
	memcpy(&p->cache[4], boundary, boundary_len);
	p->boundary_len = boundary_len + 2;
	p->index = 0;
	p->state = s_start;
	p->settings = settings;

	return 0;
}

void multipart_parser_uninit(multipart_parser* p)
{
	if(p->cache != NULL && p->cache != p->buf)
	{
		free(p->cache);
		p->cache = p->buf;
	}
}

int multipart_parser_reset(multipart_parser*p, const char *boundary, 
						   int boundary_len)
{
	if(boundary_len < 0)
		boundary_len = boundary != NULL ? (int)strlen(boundary) : 0;
	if(boundary_len > (int)sizeof(p->buf) - 4)
	{
		if(p->cache != NULL && p->cache != p->buf)
			free(p->cache);
		p->cache = (char*)malloc(boundary_len + 4);
		if(NULL == p->cache)
			return -1;
		p->cache[0] = CR;
		p->cache[1] = LF;
		p->cache[2] = '-';
		p->cache[3] = '-';
	}
	memcpy(&p->cache[4], boundary, boundary_len);
	p->boundary_len = boundary_len + 2;
	p->index = 0;
	p->state = s_start;
	return 0;
}

void multipart_parser_set_data(multipart_parser *p, void *data)
{
	p->data = data;
}

void *multipart_parser_get_data(multipart_parser *p)
{
	return p->data;
}

size_t multipart_parser_execute(multipart_parser* p, const char *buf, size_t len)
{
	size_t i = 0, start = 0, end = 0, data_len;
	char c, cl;

	while(i < len)
	{
		c = buf[i];
		switch(p->state)
		{
		case s_start:
			multipart_log("s_start");
			p->index = 0;
			p->state = s_start_boundary;

			/* fallthrough */
		case s_start_boundary:
			multipart_log("s_start_boundary");
lbl_boundary:
			c = buf[i];
			if(p->index < p->boundary_len)
			{
				if(c != mp_boundary(p)[p->index])
					return i;
				p->index++;
				if(++i < len)
					goto lbl_boundary;
			}
			else if(p->index == p->boundary_len)
			{
				if(c != CR)
					return i;
				p->index++;
				if(++i < len)
					goto lbl_boundary;
			}
			else
			{
				if(c != LF)
					return i;
				p->index = 0;
				NOTIFY_CB(part_data_begin);
				p->state = s_header_field_start;
			}
			break;

		case s_header_field_start:
			multipart_log("s_header_field_start");
			start = i;
			p->call0 = 1;
			p->state = s_header_field;

			/* fallthrough */
		case s_header_field:
			multipart_log("s_header_field");
lbl_field:
			cl = (char)((unsigned char)c | 0x20);
			if('-' == c || (cl >= 'a' && cl <= 'z'))
			{
				if(++i < len)
				{
					c = buf[i];
					goto lbl_field;
				}
				else
				{
					p->call0 = 0;
					EMIT_DATA_CB(header_field, buf + start, i - start);
				}
			}
			else if(':' == c)
			{
				if((data_len = i - start) > 0 || p->call0)
					EMIT_DATA_CB(header_field, buf + start, data_len);
				p->state = s_header_value_start;
			}
			else if(CR != c)
			{
				multipart_log("invalid character in header name");
				return i;
			}
			else
				p->state = s_headers_almost_done;
			break;

		case s_headers_almost_done:
			multipart_log("s_headers_almost_done");
			if(c != LF)
				return i;
			p->call0 = 1;
			p->state = s_part_data_start;
			break;

		case s_header_value_start:
			multipart_log("s_header_value_start");
			if(' ' == c || '\t' == c)
				break;

			start = i;
			p->call0 = 1;
			p->state = s_header_value;

			/* fallthrough */
		case s_header_value:
			multipart_log("s_header_value");
lbl_value:
			if(CR != c)
			{
				if(++i < len)
				{
					c = buf[i];
					goto lbl_value;
				}
				else
				{
					p->call0 = 0;
					EMIT_DATA_CB(header_value, buf + start, i - start);
				}
			}
			else
			{
				if((data_len = i - start) > 0 || p->call0)
				{
					p->call0 = 0;
					EMIT_DATA_CB(header_value, buf + start, data_len);
				}
				p->state = s_header_value_almost_done;
			}
			break;

		case s_header_value_almost_done:
			multipart_log("s_header_value_almost_done");
			if(c != LF)
				return i;
			
			p->state = s_header_field_start;
			break;

		case s_part_data_start:
			multipart_log("s_part_data_start");
			NOTIFY_CB(headers_complete);
			start = i;
			end = i;
			p->state = s_part_data;
			p->call0 = 1;

			/* fallthrough */
		case s_part_data:
			multipart_log("s_part_data");
lbl_data:
			if(c != CR)
			{
				end = i + 1;
				if(++i < len)
				{
					c = buf[i];
					goto lbl_data;
				}
				else
				{
					p->call0 = 0;
					EMIT_DATA_CB(part_data, buf + start, end - start);
				}
			}
			else
			{
				p->state = s_part_data_almost_boundary;
				p->index = 1;
				if((i == (len - 1)) && ((data_len = end - start) > 0 || p->call0))
				{
					p->call0 = 0;
					EMIT_DATA_CB(part_data, buf + start, data_len);
				}
			}
			break;

		case s_part_data_almost_boundary:
			multipart_log("s_part_data_almost_boundary");
lbl_boundary2:
			if(mp_cache(p)[p->index] == c)
			{
				if(++p->index > p->boundary_len + 1)
				{
					if((data_len = end - start) > 0 || p->call0)
					{
						p->call0 = 0;
						EMIT_DATA_CB(part_data, buf + start, data_len);
					}
					NOTIFY_CB(part_data_end);
					p->state = s_part_data_almost_end;
					break;
				}
				if(++i < len)
				{
					c = buf[i];
					goto lbl_boundary2;
				}
				else if((data_len = end - start) > 0 || p->call0)
				{
					p->call0 = 0;
					EMIT_DATA_CB(part_data, buf + start, data_len);
				}
			}
			else
			{
				if(p->index > i)
				{
					p->call0 = 0;
					EMIT_DATA_CB(part_data, mp_cache(p), p->index - i);
				}
				if(c != CR)
				{
					end = i + 1;
					p->state = s_part_data;
					p->index = 0;
				}
				else
				{
					end = i;
					p->state = s_part_data_almost_boundary;
					p->index = 1;
				}
				if(i == (len - 1) && ((data_len = end - start) > 0 || p->call0))
				{
					p->call0 = 0;
					EMIT_DATA_CB(part_data, buf + start, end - start);
				}
			}
			break;

		case s_part_data_almost_end:
			multipart_log("s_part_data_almost_end");
			if(CR == c)
			{
				p->state = s_part_data_end;
				break;
			}
			else if('-' == c)
			{
				p->state = s_part_data_final_hyphen;
				break;
			}
			else
				return i;

		case s_part_data_final_hyphen:
			multipart_log("s_part_data_final_hyphen");
			if('-' == c)
			{
				NOTIFY_CB(body_end);
				p->state = s_end;
				break;
			}
			return i;

		case s_part_data_end:
			multipart_log("s_part_data_end");
			if(LF == c)
			{
				p->state = s_header_field_start;
				NOTIFY_CB(part_data_begin);
				break;
			}
			return i;

		case s_end:
			multipart_log("s_end: %02X", (int)c);
			break;

		default:
			multipart_log("Multipart parser unrecoverable error");
			return 0;
		}
		++ i;
	}

	return len;
}
