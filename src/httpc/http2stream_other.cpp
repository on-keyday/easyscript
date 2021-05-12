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

bool HTTP2StreamLayer::send_settings(){
    std::string framedata;
    for(auto& setting:settings){
        unsigned short key=setting.first;
        unsigned int value=setting.second;
        auto keyp=(char*)&key,valp=(char*)&value;
        key=translate_byte_net_and_host<unsigned short>(keyp);
        value=translate_byte_net_and_host<unsigned int>(valp);
        framedata.append(keyp,2);
        framedata.append(valp,4);
    }
    return send_frame(SETTINGS,NONEF,0,framedata.data(),framedata.size());
}

bool HTTP2StreamLayer::invoke_callback(H2FType type,HTTP2StreamContext* stream,H2Flag flag,const char* data,size_t datasize){
    if(!callback)return true;
    return callback(true,data,datasize,type,flag,stream,user);
}

bool HTTP2StreamLayer::do_a_proc(Reader<std::string>& reader,bool initial){
    //manager.register_cb(frame_callback,this);
    if(!manager.recv_frame(reader,settings[MAX_FRAME_SIZE])){
        if(manager.error()!=0){
            manager.send_error(HTTP2_INTERNAL_ERROR);
        }
        return false;
    }
    bool ok=false;
    while(manager.size()){
        if(initial){
            if(manager.frame().type!=SETTINGS){
                manager.send_error(HTTP2_PROTOCOL_ERROR);
                return false;
            }
        }
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
    settings[MAX_CONCURRENT_STREAMS]=100;
    settings[INITIAL_WINDOW_SIZE]=0xffff;
    settings[MAX_FRAME_SIZE]=16384;
    settings[MAX_HEADER_LIST_SIZE]=~0;
    manager.register_cb(frame_callback,this);
}

