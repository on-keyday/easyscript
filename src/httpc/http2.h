#pragma once
#include"socket.h"
#include<queue>

#define HTTP2_PROTOCOL_ERROR 1
#define HTTP2_INTERNAL_ERROR 2
#define HTTP2_STREAM_CLOSED 5
#define HTTP2_FRAMESIZE_ERROR 6

namespace PROJECT_NAME{

    enum class HTTP2Status{
        idle,
        open,
        reserved_local,
        reserved_remote,
        half_closed_local,
        half_closed_remote,
        closed
    };

    struct HTTP2StreamContext{
        int id=0;
        HTTP2Status status=HTTP2Status::idle;
        int errorcode=0;
        int dependency=0;
        unsigned char weight=0;
        bool exclusive=false;
        std::string headerbuf;
        std::string databuf;
    };

    struct HTTP2{
        SecureSocket sock;
        std::deque<HTTP2Frame<std::string>> frames;
        std::map<unsigned short,unsigned int> settings;
        std::map<int,HTTP2StreamContext> streams;
        int last_succeed=0;
        HTTP2StreamContext& find_or_make_stream(int id);
        HTTP2Status get_status_of_stream(int id);
        bool is_idle_stream(int id);
        bool is_stream_related(int id){return id>0;}
        bool set_stream_status(int id,HTTP2Status status,int code=0);
        bool set_stream_half_or_closed(int id,bool is_recv=true);
        bool select_recv();
        bool read_a_frame(Reader<std::string>& reader);
        bool read_set_of_frames(Reader<std::string>& reader);
        bool send_setting_ack();
        bool send_ping();
        bool send_error(unsigned int errorcode,const char* additional=nullptr,size_t size=0);
        bool send_rst(int id,unsigned int errorcode);
        bool frames_pop(bool succeed);
        bool send_error_and_pop(unsigned int errorcode,const char* additional=nullptr,size_t size=0);
        bool send_rst_and_pop(int id,unsigned int errorcode);
        bool set_setting_if_exist(unsigned short key,unsigned int& value);
        bool parse_setting();
        bool parse_rst_stream();
        bool parse_priority();
        bool read_dependency(Reader<std::string>& reader,HTTP2StreamContext& stream);
        bool parse_data();
        bool parse_header();
        bool read_continution(HTTP2StreamContext& stream);
    };
}