#pragma once
#include"socket.h"
#include<queue>

#define HTTP2_PROTOCOL_ERROR 1
#define HTTP2_INTERNAL_ERROR 2
#define HTTP2_FLOW_CONTROL_ERROR 3
#define HTTP2_STREAM_CLOSED 5
#define HTTP2_FRAMESIZE_ERROR 6


namespace PROJECT_NAME{

    enum H2FType:unsigned char{
        DATA,
        HEADER,
        PRIORITY,
        RST_STREAM,
        SETTINGS,
        PUSH_PROMISE,
        PING,
        GOAWAY,
        WINDOW_UPDATE,
        CONTINUATION,
        UNKNOWN=0xff
    };

    enum H2Flag:unsigned char{
        END_STREAM=0x1,
        END_HEADERS=0x4,
        PADDED=0x8,
        PRIORITY_F=0x20,
        ACK=0x1,
        NONEF=0x0
    };

    enum H2SETTINGS{
        HEADER_TABLE_SIZE=0x1,
        ENABLE_PUSH=0x2,
        MAX_CONCURRENT_STREAMS=0x3,
        INITIAL_WINDOW_SIZE=0x4,
        MAX_FRAME_SIZE=0x5,
        MAX_HEADER_LIST_SIZE =0x6
    };

#define MAKE_H2FLAG(x) ((H2Flag)(x))

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
        long long framesize=65535;
        std::chrono::system_clock::time_point updated_at;
    };


    struct HTTP2FrameLayer{
        using FrameCallback=bool(*)(H2FType type,H2Flag flag,int id,char* data,int size,void*);
    private:
        SecureSocket sock;
        std::deque<HTTP2Frame<std::string>> frames;
        int last_succeed=0;
        unsigned int on_error=0;
        FrameCallback callback=nullptr;
        void* user=nullptr;
    public:
        unsigned int& error(){return on_error;}
        bool send(const std::string& buf);
        bool send_frame(H2FType type,H2Flag flag,int id,char* data,int size);
        bool select_recv(std::string& buf);
        bool read_a_frame(Reader<std::string>& reader,int framesize);
        bool read_set_of_frames(Reader<std::string>& reader,int framesize);
        bool pop(bool succeed);
        bool reverse_ping();
        bool send_error(unsigned int errorcode,const char* additional=nullptr,size_t size=0);
        HTTP2Frame<std::string>& current(){return frames.front();}
        size_t size(){return frames.size();}
        void register_cb(FrameCallback cb,void* user){
            callback=cb;
            this->user=user;
        }

        SecureSocket& get_socket(){return sock;}
    };

    struct HTTP2FlowControlLayer{
    private:
        HTTP2FrameLayer frames;
        std::map<int,HTTP2StreamContext> streams;
        long long delta=20;
        int maxid=0;
        int max_framesize=65535;
        bool on_allow_delta(int id){
            auto diff=std::chrono::system_clock::now()-stream(id).updated_at;
            return std::chrono::duration_cast<std::chrono::microseconds>(diff).count()<delta;
        }
        bool change_flow(bool recv,int id,H2FType type,H2Flag flag);
        bool change_flow_recv(int id,HTTP2Status& st,H2FType type,H2Flag flag);
        bool change_flow_send(HTTP2Status& st,H2FType type,H2Flag flag);

        int verify_promise(char* data,H2Flag flag);
    public:
        bool send_frame(H2FType type,H2Flag flag,int id,char* data,int size);
        bool recv_frame(Reader<std::string>& reader,int framesize);
        HTTP2Frame<std::string>& frame(){return frames.current();}
        HTTP2StreamContext& stream(int id);
        bool send(const std::string& buf){return frames.send(buf);}
        bool send_rst(int id,unsigned int errorcode);
        bool send_error(unsigned int errorcode,const char* additional=nullptr,size_t size=0){
            return frames.send_error(errorcode,additional,size);
        }
        bool pop(bool succeed){return frames.pop(succeed);}
        size_t size(){return frames.size();}
        unsigned int& error(){return frames.error();}
        void register_cb(HTTP2FrameLayer::FrameCallback cb,void* user){
            frames.register_cb(cb,user);
        }
        SecureSocket& get_socket(){return frames.get_socket();}
        void set_maxframesize(int size){
            max_framesize=size;
        }
    };

    struct HTTP2StreamLayer{
        using StreamCallback=bool(*)(bool,const char*,int,H2FType,H2Flag,HTTP2StreamContext*,void*);
    private:
        HTTP2FlowControlLayer manager;
        std::map<unsigned short,unsigned int> settings;
        long long framesize=65535;

        bool parse_frame();
        bool parse_data();
        bool parse_header();
        bool parse_priority();
        bool parse_rst_stream();
        bool parse_settings();
        bool parse_push_promise();
        bool parse_goaway();
        bool parse_window_update();
        bool read_dependency(Reader<std::string>& reader,HTTP2StreamContext& stream);
        bool read_continution(HTTP2StreamContext& stream,int id);


        bool verify_settings_value(unsigned short key,unsigned int value);
        bool send_rst_and_pop(int id,unsigned int errorcode);

        StreamCallback callback=nullptr;
        void* user=nullptr;
        bool invoke_callback(H2FType type,HTTP2StreamContext* stream,H2Flag flag=NONEF,const char* data=nullptr,size_t datasize=0);
        static bool frame_callback(H2FType type,H2Flag flag,int id,char* data,int size,void* user);
    public:
        HTTP2StreamLayer();

        bool send_rst(int id,unsigned int errorcode){
            return manager.send_rst(id,errorcode);
        }
        bool send(const std::string& buf){return manager.send(buf);}
        bool send_frame(H2FType type,H2Flag flag,int id,char* data,int size){
            if(size<0||size>settings[MAX_FRAME_SIZE])return false;
            return manager.send_frame(type,flag,id,data,size);
        }
        
        bool send_settings();

        bool do_a_proc(Reader<std::string>& reader,bool initial);

        void register_cb(StreamCallback cb,void* user){callback=cb;this->user=user;}
        SecureSocket& get_socket(){return manager.get_socket();}
        std::map<unsigned short,unsigned int>& get_settings(){return settings;}
    };

    struct HTTP2AppContext;

    struct HTTP2AppLayer{
    private:
        bool in_member=false;
        void* user=nullptr;
        HTTP2StreamLayer streams;
        using OnCallback=bool(*)(HTTP2AppContext& ctx,void* user);
        //using OnReceiveCallback=bool(*)();
        OnCallback on_send=nullptr;
        OnCallback on_recv=nullptr;
        OnCallback on_app=nullptr;
        static bool stream_callback(bool on_recv,const char* data,int size,H2FType type,H2Flag flag,HTTP2StreamContext* stream,void* user);
        bool send_connection_preface();
        bool sendable(HTTP2Status status);
        bool parseurl(const std::string& url,URLContext<std::string>& ctx,unsigned short& port);
        bool connection_negotiate(URLContext<std::string>& ctx,unsigned short port);
    public:
        bool register_settings(const std::string& cafile);
        bool client(const std::string& url,void* user,OnCallback app,OnCallback recv,OnCallback onsend=nullptr);
        HTTP2StreamLayer* get_streams(HTTP2AppContext* v);
    };

    struct HTTP2AppContext{
    private:
        HTTP2AppLayer* self=nullptr;
        HTTP2StreamContext* stream=nullptr;
        std::string data;
        H2Flag flag=NONEF;
        H2FType frame=UNKNOWN;
        bool no_change=true;
        bool master=true;
        
    public:
        HTTP2AppContext(HTTP2AppLayer* owner):self(owner){}

        bool set_settings(bool nochange,const char* data,int size,H2Flag flag,H2FType type,HTTP2StreamContext* stream){
            if(!master)return false;
            this->stream=stream;
            if(data&&size>0){
                this->data.append(data,size);
            }
            this->flag=flag;
            this->frame=type;
            no_change=nochange;
            return true;
        }

        bool change_permission(HTTP2AppLayer* v,bool mode){
            if(!verify(v))return false;
            master=mode;
            return true;
        }

        bool verify(HTTP2AppLayer* v){return v==self;}

        H2FType get_frametype(){return frame;}

        H2Flag get_frameflag(){return flag;}

        std::string& get_data(){return data;}

        bool send_frame(H2FType type,H2Flag flag,int id,char* data,int size);
        
        int send_header(const std::string& data,unsigned char padding=0);
        int send_data(const std::string& data,unsigned char padding=0);
    };
}