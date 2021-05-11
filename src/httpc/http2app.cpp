#include"http2.h"

using namespace PROJECT_NAME;

bool HTTP2AppLayer::sendable(HTTP2Status status){
    return status==HTTP2Status::idle||
           status==HTTP2Status::reserved_local||
           status==HTTP2Status::open||
           status==HTTP2Status::half_closed_remote;
}

bool HTTP2AppLayer::stream_callback(bool on_recv,char* data,int size,H2FType type,
                                    H2Flag flag,HTTP2StreamContext* stream,void* user){
    HTTP2AppLayer* app=(HTTP2AppLayer*)user;
    HTTP2AppContext ctx(app);
    if(on_recv){
        if(!app->on_recv)return true;
        ctx.set_settings(true,nullptr,0,NONEF,type,stream);
        ctx.change_permission(app,false);
        app->on_recv(ctx,user);
    }
    else{
        if(!app->on_send)return false;
        ctx.set_settings(true,data,size,flag,type,stream);
        ctx.change_permission(app,false);
        app->on_send(ctx,user);
    }
    return true;
}

bool HTTP2AppLayer::send_connection_preface(){
    return streams.send("PRI * HTTP/2.0\r\nSM\r\n\r\n")&&
           streams.send_frame(SETTINGS,NONEF,0,nullptr,0);
}

bool HTTP2AppLayer::parseurl(const std::string& url,URLContext<std::string>& urlctx,unsigned short& port){
    std::string u(url);
    Reader<std::string> url_parse(u);
    urlctx.needend=true;
    url_parse.readwhile(parse_url,urlctx);
    if(!urlctx.succeed){
        return false;
    }
    if(urlctx.host==""){
        return false;
    }
    if(urlctx.path==""){
        urlctx.path="/";
    }
    if(urlctx.port!=""){
        try{
            auto tmp=std::stoi(urlctx.port);
            if(tmp>0xffff){
                return false;
            }
            port=(unsigned short)tmp;
        }
        catch(...){
            return false;
        }
    }
    return true;
}

bool HTTP2AppLayer::connection_negotiate(URLContext<std::string>& urlctx,unsigned short port){
    auto& sock=streams.get_socket();
    if(!sock.open(urlctx.host.c_str(),"https")){
        return false;
    }
    if(!sock.connect(port,true,"\2h2",3)){
        return false;
    }
    auto ssl=sock.get_basessl();
    if(!ssl){
        sock.close();
        return false;
    }
    unsigned char* st;
    unsigned int len=0;
    SSL_get0_alpn_selected(ssl,&st,&len);
    if(len==0||memcmp(st,"h2",2)!=0){
        sock.close();
        return false;
    }
    return true;
}

bool HTTP2AppLayer::start(const std::string& url,void* user,OnCallback app,OnCallback recv,OnCallback send=nullptr){
    if(!app||!recv)return false;
    on_app=app;
    on_recv=recv;
    on_send=send;
    URLContext<std::string> urlctx;
    unsigned short port=0;
    if(!parseurl(url,urlctx,port)){
        return false;
    }
    if(!connection_negotiate(urlctx,port)){
        return false;
    }
    if(!send_connection_preface())return false;
    Reader<std::string> reader;
    HTTP2AppContext appctx(this);
    while(streams.do_a_proc(reader,stream_callback,this)){
        app(appctx,user);
    }
    return true;
}