#include"http2.h"

using namespace PROJECT_NAME;

template<class Type>
bool read_bytes(Reader<std::string>& reader,Type& t,size_t expect=sizeof(Type)){
    return reader.read_byte(&t,sizeof(t),translate_byte_net_and_host,true)==expect;
}

bool HTTP2StreamLayer::parse_frame(){
    if(!manager.size())return false;
    switch(manager.frame().type){
    case DATA:
        return parse_data();
    case HEADER:
        return parse_header();
    case RST_STREAM:
        return parse_rst_stream();
    case PRIORITY:
        return parse_priority();
    case SETTINGS:
        return parse_settings();
    case PUSH_PROMISE:
        return parse_push_promise();
    case GOAWAY:
        return parse_goaway();
    case WINDOW_UPDATE:
        return parse_window_update();
    default:
        return false;
    }
}

bool HTTP2StreamLayer::parse_data(){
    auto& ref=manager.frame();
    auto& stream=manager.stream(ref.id);
    Reader<std::string> reader(ref.buf);
    auto payloadsize=ref.len;
    int ofs=0;
    if(ref.flag&PADDED){
        unsigned char padded=0;
        if(!read_bytes(reader,padded)){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        ofs++;
        payloadsize--;
        if(payloadsize<=padded){
            manager.send_error(HTTP2_FRAMESIZE_ERROR);
            return false;
        }
    }
    stream.databuf.append(&reader.ref().c_str()[ofs],payloadsize);
    stream.framesize-=payloadsize;
    manager.pop(true);
    return invoke_callback(DATA,&stream);
}

bool HTTP2StreamLayer::parse_header(){
    auto& ref=manager.frame();
    auto id=ref.id;
    auto& stream=manager.stream(id);
    Reader<std::string> reader(ref.buf);
    auto headersize=ref.len;
    unsigned char padded=0;
    bool continues=true;
    int ofs=0;
    if(ref.flag&PADDED){
        if(!read_bytes(reader,padded)){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        ofs++;
        headersize--;
    }
    if(ref.flag&PRIORITY_F){
        if(!read_dependency(reader,stream)){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        ofs+=5;
        headersize-=5;
    }
    if(headersize<=padded){
        manager.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    stream.headerbuf.append(&ref.buf.c_str()[ofs],headersize);
    if(ref.flag&END_HEADERS){
        continues=false;
    }
    manager.pop(true);
    if(continues){
        return read_continution(stream,id)&&invoke_callback(HEADER,&stream);
    }
    return invoke_callback(HEADER,&stream);
}

bool HTTP2StreamLayer::parse_rst_stream(){
    auto& ref=manager.frame();
    if(ref.len!=4){
        manager.send_error(HTTP2_FRAMESIZE_ERROR);
        return false;
    }
    Reader<std::string> reader(ref.buf);
    unsigned int code;
    if(!read_bytes(reader,code)){
        manager.send_error(HTTP2_INTERNAL_ERROR);
        return false;
    }
    auto& stream=manager.stream(ref.id);
    stream.errorcode=code;
    manager.pop(true);
    return invoke_callback(RST_STREAM,&stream);
}

bool HTTP2StreamLayer::parse_priority(){
    auto& ref=manager.frame();
    if(ref.len!=5){
        manager.send_error(HTTP2_FRAMESIZE_ERROR);
        return false;
    }
    auto& stream=manager.stream(ref.id);
    Reader<std::string> reader(ref.buf);
    if(!read_dependency(reader,stream)){
        manager.send_error(HTTP2_INTERNAL_ERROR);
        return false;
    }
    manager.pop(true);
    return invoke_callback(PRIORITY,&stream);
}


bool HTTP2StreamLayer::parse_settings(){
    auto& ref=manager.frame();
    if(ref.flag&ACK){
        if(ref.len!=0){
            manager.send_error(HTTP2_FRAMESIZE_ERROR);
            return false;
        }
        manager.pop(true);
        invoke_callback(SETTINGS,nullptr,ACK);
        return true;
    }
    if(ref.len%6){
        manager.send_error(HTTP2_FRAMESIZE_ERROR);
        return false;
    }
    Reader<std::string> reader(ref.buf);
    while(!reader.ceof()){
        unsigned short key=0;
        unsigned int value=0;
        if(!read_bytes(reader,key)||!read_bytes(reader,value)){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        if(key>=HEADER_TABLE_SIZE&&key<=MAX_HEADER_LIST_SIZE){
            if(!verify_settings_value(key,value)){
                if(key==INITIAL_WINDOW_SIZE){
                    manager.send_error(HTTP2_FLOW_CONTROL_ERROR);
                }
                else{
                    manager.send_error(HTTP2_PROTOCOL_ERROR);
                }
                return false;
            }
            if(key==MAX_FRAME_SIZE){
                manager.set_maxframesize(value);
            }
        }
        settings[key]=value;
    }
    manager.pop(true);
    invoke_callback(SETTINGS,nullptr);
    return manager.send_frame(SETTINGS,ACK,0,nullptr,0);
}

bool HTTP2StreamLayer::parse_push_promise(){
    if(settings[ENABLE_PUSH]==0){
        manager.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    auto& ref=manager.frame();
    auto& stream=manager.stream(ref.id);
    unsigned char padded=0;
    int ofs=0;
    int id=0;
    int headersize=ref.len;
    Reader<std::string> reader(ref.buf);
    if(ref.flag&PADDED){
        if(!read_bytes(reader,padded)){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        ofs++;
        headersize--;
    }
    if(!read_bytes(reader,id)){
        manager.send_error(HTTP2_INTERNAL_ERROR);
        return false;
    }
    if(id<=0){
        manager.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    ofs+=4;
    headersize-=4;
    if(headersize<=padded){
        manager.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    auto& promised=manager.stream(id);
    if(promised.status!=HTTP2Status::idle){
        manager.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    promised.status=HTTP2Status::reserved_remote;
    promised.headerbuf.append(&ref.buf.c_str()[ofs],headersize);
    if(ref.flag&END_HEADERS)return true;
    return read_continution(promised,id)&&invoke_callback(PUSH_PROMISE,&promised);
}

bool HTTP2StreamLayer::parse_goaway(){
    auto& ref=manager.frame();
    int id=0;
    unsigned int errorcode=0;
    Reader<std::string> reader(ref.buf);
    if(!read_bytes(reader,id)||!read_bytes(reader,errorcode)){
        manager.pop(false);
        return false;
    }
    if(ref.buf.size()<8)return true;
    std::string debuginfo;
    debuginfo.append(&ref.buf.c_str()[8],ref.buf.size()-8);
    manager.pop(!errorcode);
    if(errorcode){
        manager.error()=errorcode;
    }
    invoke_callback(GOAWAY,nullptr,NONEF,debuginfo.data(),debuginfo.size());
    return errorcode!=0;
}

bool HTTP2StreamLayer::parse_window_update(){
    auto& ref=manager.frame();
    if(ref.len!=4){
        manager.send_error(HTTP2_FRAMESIZE_ERROR);
        return false;
    }
    Reader<std::string> reader(ref.buf);
    int update=0;
    if(!read_bytes(reader,update)){
        manager.send_error(HTTP2_INTERNAL_ERROR);
        return false;
    }
    if(update<0)return false;
    if(ref.id==0){
        if(framesize+update<0){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        framesize+=update;
        manager.pop(true);
        return invoke_callback(WINDOW_UPDATE,nullptr);
    }
    else{
        auto& stream=manager.stream(ref.id);
        if(stream.framesize+update<0){
            manager.send_error(HTTP2_INTERNAL_ERROR);
            return false;
        }
        stream.framesize+=update;
        manager.pop(true);
        return invoke_callback(WINDOW_UPDATE,&stream);
    }
}

bool HTTP2StreamLayer::read_dependency(Reader<std::string>& reader,HTTP2StreamContext& stream){
    unsigned int dependency=0;
    unsigned char weight=0;
    if(!read_bytes(reader,dependency)||!read_bytes(reader,weight)){
        return false;
    }
    if(dependency&0x80000000){
        dependency&=0x7fffffff;
        stream.exclusive=true;
    }
    stream.dependency=(int)dependency;
    stream.weight=weight;
    return true;
}


bool HTTP2StreamLayer::read_continution(HTTP2StreamContext& stream,int id){
    bool ok=false;
    while(manager.size()){
        auto& ref2=manager.frame();
        if(ref2.type!=CONTINUATION||ref2.id!=id){
            manager.send_error(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        stream.headerbuf.append(ref2.buf.c_str(),ref2.len);
        bool end=ref2.flag&END_HEADERS;
        manager.pop(true);
        if(end){
            ok=true;
            break;
        }
    }
    if(!ok){
        manager.send_error(HTTP2_PROTOCOL_ERROR);
    }
    return ok;
}
