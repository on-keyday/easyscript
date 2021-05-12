#include"httpc/http2.h"
#define DLL_EXPORT __declspec(dllexport)
#include"httpc/sockinterpreter.h"
#include<iostream>
using namespace PROJECT_NAME;

void print_Frame(H2FType type){
    switch (type)
    {
    case HEADER:
        std::cout<<"HEADER\n";
        break;
    case SETTINGS:
        std::cout << "SETTINGS\n";
        break;
    case GOAWAY:
        std::cout << "GOAWAY\n";
        break;
    case PUSH_PROMISE:
        std::cout << "PUSH_PROMISE\n";
        break;
    case UNKNOWN:
        std::cout << "UNKNOWN(FF)\n";
        break;
    case PING:
        std::cout << "PING\n";
        break;
    case WINDOW_UPDATE:
        std::cout << "WINDOW_UPDATE\n";
        break;
    default:
        std::cout << "UNKNOWN\n";
        break;
    }
}

bool OnRecv(HTTP2AppContext& ctx,void* user){
    std::cout << "recv>";
    print_Frame(ctx.get_frametype());
    if(ctx.get_frametype()==GOAWAY){
        auto& data=ctx.get_data();
        std::cout << data << "\n";
    }
    if(ctx.get_frametype()==SETTINGS&&ctx.get_frameflag()&ACK){
        std::cout << "ACK\n";
        return true;
    }
    return true;
}

bool OnApp(HTTP2AppContext& ctx,void* user){
    bool* flag=(bool*)user;
    if(*flag){
        //ctx.send_frame(PING,NONEF,0,"testdata",8);
        *flag=false;
    }
    return true;
}

bool OnSend(HTTP2AppContext& ctx,void* user){
    std::cout << "send<";
    print_Frame(ctx.get_frametype());
    if(ctx.get_frametype()==SETTINGS&&ctx.get_frameflag()&ACK){
        std::cout << "ACK\n";
        return true;
    }
    if(ctx.get_frametype()==PING&&ctx.get_frameflag()==ACK){
        if(ctx.get_data()!="testdata"){
            return false;
        }
    }
    if(ctx.get_frametype()==GOAWAY){
        return false;
    }
    return true;
}

#include"samplecode.cpp"

int __stdcall http2_test(){
    HTTP2AppLayer app;
    app.register_settings("D:\\CommonLib\\netsoft\\cacert.pem");
    bool flag=true;
    app.client("nghttp2.org",&flag,OnApp,OnRecv,OnSend);
    sample_code(0,nullptr);
    return true;
}