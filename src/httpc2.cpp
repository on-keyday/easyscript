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
    std::cout << "recv" << "(" << ctx.get_stream_id() << ")>";
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
        char header[]={
        0x00,                                                   // 圧縮情報
        0x07, 0x3a, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64,         // 7 :method
        0x03, 0x47, 0x45, 0x54,                                 // 3 GET
        0x00,                                                   // 圧縮情報
        0x05, 0x3a, 0x70, 0x61, 0x74, 0x68,                     // 5 :path
        0x01, 0x2f,                                             // 1 /
        0x00,                                                   // 圧縮情報
        0x07, 0x3a, 0x73, 0x63, 0x68, 0x65, 0x6d, 0x65,         // 7 :scheme
        0x05, 0x68, 0x74, 0x74, 0x70, 0x73,                     // 5 https
        0x00,                                                   // 圧縮情報
        0x0a, 0x3a, 0x61, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79,           // 10 :authority
        0x0b, 0x6e, 0x67, 0x68, 0x74, 0x74, 0x70, 0x32, 0x2e, 0x6f, 0x72, 0x67 };
        ctx.send_frame(HEADER,END_STREAM,3,header,sizeof(header));
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

//#include"samplecode.cpp"

int __stdcall http2_test(){
    HTTP2AppLayer app;
    app.register_settings("D:\\CommonLib\\netsoft\\cacert.pem");
    bool flag=true;
    app.client("google.com",&flag,OnApp,OnRecv,OnSend);
    //sample_code(0,nullptr);
    return true;
}