#include"http2.h"

using namespace PROJECT_NAME;

bool HTTP2StreamLayer::frame_callback(H2FType type,H2Flag flag,int id,char* data,int size,void* user){
    auto self=(HTTP2StreamLayer*)user;
    if(!self->callback)return true;
    auto stream=id==0?nullptr:&self->manager.stream(id);
    return self->callback(false,data,size,type,flag,stream,self->user);
}

bool HTTP2StreamLayer::send_rst_and_pop(int id,unsigned int errorcode){
    return send_rst(id,errorcode)&&manager.pop(!errorcode);
}

bool HTTP2StreamLayer::invoke_callback(H2FType type,HTTP2StreamContext* stream){
    if(!callback)return true;
    return callback(true,nullptr,0,type,NONEF,stream,user);
}

bool HTTP2StreamLayer::do_a_proc(Reader<std::string>& reader,StreamCallback cb,void* user){
    callback=cb;
    this->user=user;
    manager.register_cb(frame_callback,this);
    if(!manager.recv_frame(reader,settings[MAX_FRAME_SIZE])){
        if(!manager.error()){
            manager.send_error(HTTP2_INTERNAL_ERROR);
        }
        return false;
    }
    while(manager.size()){
        if(!parse_frame()){
            if(!manager.error()){
                manager.send_error(HTTP2_INTERNAL_ERROR);
            }
            return false;
        }
    }
    return true;
}

bool HTTP2StreamLayer::verify_settings_value(unsigned short key,unsigned int value){
    switch (key)
    {
    case HEADER_TABLE_SIZE:
        return true;
    case ENABLE_PUSH:
        return value==1||value==0;
    case MAX_CONCURRENT_STREAMS:
        return true;
    case INITIAL_WINDOW_SIZE:
        return value<=0xffffff;
    case MAX_FRAME_SIZE:
        return 16384<=value&&value<=0xffffff;
    case MAX_HEADER_LIST_SIZE:
        return true;
    default:
        return false;
    }
}

HTTP2StreamLayer::HTTP2StreamLayer(){
    settings[HEADER_TABLE_SIZE]=4096;
    settings[ENABLE_PUSH]=1;
    settings[MAX_CONCURRENT_STREAMS]=~0;
    settings[INITIAL_WINDOW_SIZE]=0xffff;
    settings[MAX_FRAME_SIZE]=16384;
    settings[MAX_HEADER_LIST_SIZE]=~0;
}

