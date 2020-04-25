#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "multipart_parser.h"

typedef int tb_int32;

static tb_int32 on_mp_field(multipart_parser* mp, const char *at, size_t length)
{
	printf("%s(%d) [%.*s]\n", __FUNCTION__, (int)length, (int)length, at);
	return 0;
}

static tb_int32 on_mp_value(multipart_parser* mp, const char *at, size_t length)
{
	printf("%s(%d) [%.*s]\n", __FUNCTION__, (int)length, (int)length, at);
	return 0;
}

static tb_int32 on_mp_data(multipart_parser* mp, const char *at, size_t length)
{
	static const char* tab = "0123456789ABCDEF";
	char hex[1024];
	size_t i;
	int offset = 0;
	printf("%s(%d) [%.*s]\n", __FUNCTION__, (int)length, (int)length, at);
	for(i=0; i<length && i < sizeof(hex) / 3; i++)
	{
		hex[offset++] = tab[((unsigned char*)at)[i] >> 4];
		hex[offset++] = tab[((unsigned char*)at)[i] & 0xF];
		hex[offset++] = ' ';
	}
	hex[offset] = '\0';
	if(length > 0)
		printf("%s\n", hex);
	return 0;
}

static tb_int32 on_mp_part_data_begin(multipart_parser* mp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static tb_int32 on_mp_headers_complete(multipart_parser* mp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static tb_int32 on_mp_part_data_end(multipart_parser* mp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static tb_int32 on_mp_body_end(multipart_parser* mp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static const multipart_parser_settings mps = {on_mp_field, on_mp_value, on_mp_data,
on_mp_part_data_begin, on_mp_headers_complete, on_mp_part_data_end, on_mp_body_end};

int main(int argc, char* argv[])
{
	multipart_parser mp;
	size_t i, remain, pos, n;
	const char* boundarys[] = {"----WebKitFormBoundarye78wFSqwnXPNls2b",
		"---------------------------18467633426500"};
	const char* datas[] = {
		"------WebKitFormBoundarye78wFSqwnXPNls2b\r\n"
		"Content-Disposition: form-data; name=\"fname\"\r\n\r\n"
		"\r\n"
		"------WebKitFormBoundarye78wFSqwnXPNls2b\r\n"
		"Content-Disposition: form-data; name=\"lna%22me\"\r\n\r\n"
		"*-._ RES;/?:@&=+$, MARK-_.!~*'()\r\n"
		"------WebKitFormBoundarye78wFSqwnXPNls2b\r\n"
		"Content-Disposition: form-data; name=\"file1\"; filename=\"test.dat\"\r\n"
		"Content-Type: application/octet-stream\r\n\r\n"
		"------WebKitFormBoundary\r\n\r\n"
		"------WebKitFormBoundary\r\n\r\n"
		"------WebKitFormBoundarye78wFSqwnXPNls2b--\r\n",
		"-----------------------------18467633426500\r\n"
		"Content-Disposition: form-data; name=\"fname\"\r\n\r\n"
		"\r\n"
		"-----------------------------18467633426500\r\n"
		"Content-Disposition: form-data; name=\"lname\"\r\n\r\n"
		"'Last\"Name\"\r\n"
		"-----------------------------18467633426500\r\n"
		"Content-Disposition: form-data; name=\"te st\\\"te+st\"\r\n\r\n"
		"test\\string\r\n"
		"-----------------------------18467633426500\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"desktop.ini\"\r\n"
		"Content-Type: application/octet-stream\r\n\r\n"
		"[ExtShellFolderViews]\r\n"
		"{BE098140-A513-11D0-A3A4-00C04FD706EC}={BE098140-A513-11D0-A3A4-00C04FD706EC}\r\n"
		"[{BE098140-A513-11D0-A3A4-00C04FD706EC}]\r\n"
		"Attributes=1\r\n"
		"IconArea_Text=0x0000ff\r\n"
		"IconArea_Image=\"bg.jpg\"\r\n"
		"[.ShellClassInfo]\r\n"
		"ConfirmFileOp=0\r\n"
		"-----------------------------18467633426500--\r\n",
	};
	srand((unsigned)time(NULL));
	multipart_parser_init(&mp, NULL, 0, &mps);
	for(i=0; i<sizeof(datas)/sizeof(datas[0]); i++)
	{
		multipart_parser_reset(&mp, boundarys[i], (int)strlen(boundarys[i]));
		remain = strlen(datas[i]);
		pos = 0;
		while(remain > 0)
		{
			n = (rand() % remain) + 1;
			//n = remain;
			if(multipart_parser_execute(&mp, &datas[i][pos], n) != n)
			{
				printf("*****parser error: %u %u******\n", (int)i, (int)pos);
				break;
			}
			remain -= n;
			pos += n;
		}
		printf("####################################################\n\n");
	}
	multipart_parser_uninit(&mp);
	getchar();
	return 0;
}