## Multipart form data parser

### Features
* No dependencies
* Works with chunks of a data - no need to buffer the whole request
* Almost no internal buffering. Buffer size doesn't exceed the size of the boundary (~60-70 bytes)

Implementation based on [node-formidable](https://github.com/felixge/node-formidable) by [Felix Geisendörfer](https://github.com/felixge).

Inspired by [http-parser](https://github.com/joyent/http-parser) by [Ryan Dahl](https://github.com/ry).

### Usage (C)
See example [main.c](https://github.com/yuleniwo/multipart-parser-c/blob/master/main.c)

This parser library works with several callbacks, which the user may set up at application initialization time.

```c
multipart_parser_settings callbacks;

memset(&callbacks, 0, sizeof(multipart_parser_settings));

callbacks.on_header_field = read_header_name;
callbacks.on_header_value = read_header_value;
```

These functions must match the signatures defined in the multipart-parser header file.  For this simple example, we'll just use two of the available callbacks to print all headers the library finds in multipart messages.

Returning a value other than 0 from the callbacks will abort message processing.

```c
int read_header_name(multipart_parser* p, const char *at, size_t length)
{
   printf("%s(%d) [%.*s]\n", __FUNCTION__, (int)length, (int)length, at);
   return 0;
}

int read_header_value(multipart_parser* p, const char *at, size_t length)
{
   printf("%s(%d) [%.*s]\n", __FUNCTION__, (int)length, (int)length, at);
   return 0;
}
```

When a message arrives, callers must parse the multipart boundary from the **Content-Type** header (see the [RFC](http://tools.ietf.org/html/rfc2387#section-5.1) for more information and examples), and then execute the parser.

```c
multipart_parser mp;
multipart_parser_init(&mp, boundary, boundary_len, &callbacks);
multipart_parser_execute(&mp, body, length);
multipart_parser_uninit(&mp);
```

### Contributors
* [Daniel T. Wagner](http://www.danieltwagner.de/)
* [James McLaughlin](http://udp.github.com/)
* [Jay Miller](http://www.cryptofreak.org)

© 2012 [Igor Afonov](http://iafonov.github.com)

© 2020 [yuleniwo](https://github.com/yuleniwo)
