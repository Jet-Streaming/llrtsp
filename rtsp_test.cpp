// rtsp_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "llrtsp.h"


llrtsp_t parser;
llrtsp_settings_t settings;

int on_message_begin(llrtsp_t*)
{
    printf("%s\n", __FUNCTION__);
    return 0;
}

int on_url(llrtsp_t*, const char *at, size_t length)
{
    printf("%s: %.*s\n", __FUNCTION__, length, at);
    return 0;
}

int on_status(llrtsp_t*, const char *at, size_t length)
{
    printf("%s: %.*s\n", __FUNCTION__, length, at);
    return 0;
}

int on_header_field(llrtsp_t*, const char *at, size_t length)
{
    printf("%s: %.*s\n", __FUNCTION__, length, at);
    return 0;
}

int on_header_value(llrtsp_t*, const char *at, size_t length)
{
    printf("%s: %.*s\n", __FUNCTION__, length, at);
    return 0;
}

int on_headers_complete(llrtsp_t*)
{
    printf("%s\n", __FUNCTION__);
    return 0;
}

int on_body(llrtsp_t*, const char *at, size_t length)
{
    printf("%s: %.*s\n", __FUNCTION__, length, at);
    return 0;
}

int on_message_complete(llrtsp_t*)
{
    printf("%s\n", __FUNCTION__);
    return 0;
}


int main()
{
    //std::cout << "Hello World!\n";

    const char *str =
        //"PLAY rtsp://foo/twister RTSP/1.0\r\n"
        //"CSeq: 4\r\n"
        //"Range: npt = 0 - \r\n"

        //"RTSP/1.0 200 OK\r\n"
        //"CSeq: 4\r\n"
        //"Session : 12345678\r\n"
        //"RTP - Info : url = rtsp://foo/twister/video;seq = 9810092; rtptime = 3450012\r\n"

        //"SETUP rtsp://foo/twister RTSP/1.0\r\n"
        //"CSeq: 7\r\n"
        //"Transport : RTP / AVP; unicast; client_port = 10000\r\n"

        "RTSP/1.0 200 OK\r\n"
        "CSeq: 1\r\n"
        "Content - Type : application/sdp\r\n"
        "Content - Length : 40\r\n"
        "\r\n"
        "v = 0\r\n"
        "o = -2890844526 2890842807 IN IP4 192.16.24.202\r\n"
        "s = RTSP Session\r\n"
        "m = audio 3456 RTP/AVP 0\r\n"
        "a = control:rtsp://live.example.com/concert/audio\r\n"
        "c = IN IP4 224.2.0.1/16\r\n"

        "\r\n";

    printf("%s\n", str);

    llrtsp_settings_init(&settings);
    settings.on_message_begin = &on_message_begin;
    settings.on_url = &on_url;
    settings.on_status = &on_status;
    settings.on_header_field = &on_header_field;
    settings.on_header_value = &on_header_value;
    settings.on_headers_complete = &on_headers_complete;
    settings.on_body = &on_body;
    settings.on_message_complete = &on_message_complete;
    llrtsp_init(&parser, RTSP_BOTH, &settings);
    llrtsp_errno_t rc = llrtsp_execute(&parser, str, strlen(str));
    printf("result: %s\n", llrtsp_errno_name(rc));
    return 0;
}
