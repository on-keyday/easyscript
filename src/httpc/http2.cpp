#include"http2.h"
using namespace PROJECT_NAME;

template<class Type>
bool read_bytes(Reader<std::string>& reader,Type& t,size_t expect=sizeof(Type)){
    return reader.read_byte(&t,sizeof(t),translate_byte_net_and_host,true)==expect;
}

HTTP2StreamContext& HTTP2::find_or_make_stream(int id){
    return streams[id];
}


bool HTTP2::is_idle_stream(int id){
    if(id<0)return false;
    return get_status_of_stream(id)==HTTP2Status::idle;
}

bool HTTP2::set_stream_status(int id,HTTP2Status status,int code){
    if(!is_stream_related(id))return false;
    streams[id].status=status;
    streams[id].errorcode=code;
    return true;
}

bool HTTP2::set_stream_half_or_closed(int id,bool is_recv){
    if(is_idle_stream(id))return false; 
    auto& st=streams[id];
    if(is_recv&&st.status==HTTP2Status::half_closed_local){
        st.status=HTTP2Status::closed;
    }
    else if(!is_recv&&st.status==HTTP2Status::half_closed_remote){
        st.status=HTTP2Status::closed;
    }
    
    else if(!is_recv&&st.status==HTTP2Status::reserved_local){
        st.status=HTTP2Status::half_closed_remote;
        
    }
    else if(is_recv&&st.status==HTTP2Status::reserved_remote){
        st.status=HTTP2Status::half_closed_local;
    }
    else if(st.status==HTTP2Status::open){
        if(is_recv){
            st.status=HTTP2Status::half_closed_remote;
        }
        else{
            st.status=HTTP2Status::half_closed_local;
        }
    }
    else{
        return false;
    }
    return true;
}

HTTP2Status HTTP2::get_status_of_stream(int id){
    if(!streams.count(id))return HTTP2Status::idle;
    return streams[id].status;
}

bool HTTP2::select_recv(){
    return false;
}

bool HTTP2::read_a_frame(Reader<std::string>& reader){
    HTTP2Frame<std::string> frame;
    while(true){
        reader.readwhile(http2frame_reader,frame);
        if(frame.continues){
            if(!select_recv())return false;
            continue;
        }
        break;
    }
    if(!frame.succeed)return false;
    frames.push_back(std::move(frame));
    return true;
}

bool HTTP2::read_set_of_frames(Reader<std::string>& reader){
    if(!read_a_frame(reader)){
        return false;
    }
    if(frames.back().type==0x1){
        auto id=frames.back().id;
        while(true){
            if(frames.back().flag&0x4)break;
            if(!read_a_frame(reader)){
                return false;
            }
            if(frames.back().type!=0x9||frames.back().id!=id){
                return false;
            }
        }
    }
    else if(frames.back().type==0x6){
        send_ping();
    }
    return true;
}

bool HTTP2::send_ping(){
    return false;
}

bool HTTP2::send_error(unsigned int errorcode,const char* additional,size_t size){
    return false;
}

bool HTTP2::send_setting_ack(){
    return false;
}

bool HTTP2::send_rst_and_pop(int id,unsigned int errorcode){
    return false;
}

bool HTTP2::send_error_and_pop(unsigned int errorcode,const char* additional,size_t size){
    send_error(errorcode,additional,size);
    frames_pop(false);
    return true;
}

bool HTTP2::parse_setting(){
    if(frames.front().type==0x4){
        auto& ref=frames.front();
        if(is_stream_related(ref.id)){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        if(ref.flag&0x1){
            if(ref.len!=0){
                send_error_and_pop(HTTP2_FRAMESIZE_ERROR);
                return false;
            }
            frames_pop(true);
            return true;
        }
        if(ref.len%6){
            send_error_and_pop(HTTP2_FRAMESIZE_ERROR);
            return false;
        }
        Reader<std::string> reader(ref.buf);
        unsigned short key;
        unsigned int value;
        while(!reader.ceof()){
            if(!read_bytes(reader,key)){
                send_error_and_pop(HTTP2_INTERNAL_ERROR);
                return false;
            }
            if(read_bytes(reader,value)){
                send_error_and_pop(HTTP2_INTERNAL_ERROR);
                return false;
            }
            settings[key]=value;
        }
        frames_pop(true);
        return send_setting_ack();
    }
    return true;
}

bool HTTP2::parse_rst_stream(){
    if(frames.front().type==0x3){
        auto& ref=frames.front();
        if(!is_stream_related(ref.id)){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        if(is_idle_stream(ref.id)){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        if(ref.len!=4){
            send_error_and_pop(HTTP2_FRAMESIZE_ERROR);
            return false;
        }
        Reader<std::string> reader(ref.buf);
        unsigned int code;
        if(!read_bytes(reader,code)){
            send_error_and_pop(HTTP2_INTERNAL_ERROR);
            return false;
        }
        set_stream_status(ref.id,HTTP2Status::closed,code);
        frames_pop(true);
    }
    return true;
}

bool HTTP2::parse_priority(){
    if(frames.front().type==0x2){
        auto& ref=frames.front();
        if(!is_stream_related(ref.id)){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        if(ref.len!=5){
            send_error_and_pop(HTTP2_FRAMESIZE_ERROR);
            return false;
        }
        auto& stream=find_or_make_stream(ref.id);
        Reader<std::string> reader(ref.buf);
        if(!read_dependency(reader,stream)){
            send_error_and_pop(HTTP2_INTERNAL_ERROR);
            return false;
        }
        frames_pop(true);
    }
    return true;
}

bool HTTP2::read_dependency(Reader<std::string>& reader,HTTP2StreamContext& stream){
    unsigned int dependency=0;
    unsigned char weight=0;
    if(!read_bytes(reader,dependency)){
        return false;
    }
    if(!read_bytes(reader,weight)){
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

bool HTTP2::parse_data(){
    if(frames.front().type==0x0){
        auto& ref=frames.front();
        if(!is_stream_related(ref.id)){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        auto status=get_status_of_stream(ref.id);
        if(status!=HTTP2Status::open&&status!=HTTP2Status::half_closed_local){
            send_rst_and_pop(ref.id,HTTP2_STREAM_CLOSED);
            return false;
        }
        Reader<std::string> reader(ref.buf);
        auto payload=ref.len;
        if(ref.flag&0x8){
            unsigned char padded=0;
            if(!read_bytes(reader,padded)){
                send_error_and_pop(HTTP2_INTERNAL_ERROR);
                return false;
            }
            payload--;
            if(payload<=padded){
                send_error_and_pop(HTTP2_FRAMESIZE_ERROR);
                return false;
            }
        }
        auto& stream=find_or_make_stream(ref.id);
        stream.databuf.append(reader.ref().c_str(),payload);
        if(ref.flag&0x1){
            set_stream_half_or_closed(ref.id);
        }
        frames_pop(true);
    }
    return true;
}

bool HTTP2::parse_header(){
    if(frames.front().type==0x1){
        auto& ref=frames.front();
        if(!is_stream_related(ref.id)){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        auto status=get_status_of_stream(ref.id);
        if(status==HTTP2Status::closed||status==HTTP2Status::half_closed_local){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        auto& stream=find_or_make_stream(ref.id);
        Reader<std::string> reader(ref.buf);
        auto willread=ref.len;
        unsigned char padded=0;
        bool continues=true;
        if(ref.flag&0x8){
            if(!read_bytes(reader,padded)){
                send_error_and_pop(HTTP2_INTERNAL_ERROR);
                return false;
            }
            willread--;
        }
        if(ref.flag&0x20){
            if(!read_dependency(reader,stream)){
                send_error_and_pop(HTTP2_INTERNAL_ERROR);
                return false;
            }
            willread-=5;
        }
        if(willread<=padded){
            send_error_and_pop(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        stream.headerbuf.append(ref.buf.c_str(),willread);
        if(ref.flag&0x1){
            set_stream_half_or_closed(ref.id);
        }
        if(ref.flag&0x4){
            continues=false;
        }
        frames_pop(true);
        if(continues){
            if(!read_continution(stream))return false;
        }
    }
    return true;
}

bool HTTP2::read_continution(HTTP2StreamContext& stream){
    bool ok=false;
    while(frames.size()){
        auto& ref2=frames.front();
        if(ref2.type!=0x9||ref2.id!=stream.id){
            send_error(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        stream.headerbuf.append(ref2.buf.c_str(),ref2.len);
        bool end=ref2.flag&0x4;
        frames_pop(true);
        if(end){
            ok=true;
            break;
        }
    }
    if(!ok){
        send_error(HTTP2_PROTOCOL_ERROR);
    }
    return ok;
}

bool HTTP2::frames_pop(bool succeed){
    if(!frames.size())return false;
    if(succeed){
        if(frames.front().id>0)last_succeed=frames.front().id;
    }
    frames.pop_front();
    return true;
}

