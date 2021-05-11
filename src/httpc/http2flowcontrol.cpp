#include"http2.h"

using namespace PROJECT_NAME;

bool HTTP2FlowControlLayer::change_flow(bool recv,int id,H2FType type,H2Flag flag){
    auto update=[&,this]{streams[id].updated_at=std::chrono::system_clock::now();};
    if(id<0){
        if(recv)frames.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    if((type==GOAWAY||type==SETTINGS)&&id!=0){
        if(recv)frames.send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    if(id==0){
        if(type!=WINDOW_UPDATE){
            if(recv)frames.send_error(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        return true;
    }
    auto& st=stream(id).status;
    if(type==PRIORITY){
        update();
        return true;
    }
    if(st!=HTTP2Status::idle&&type==RST_STREAM){
        st=HTTP2Status::closed;
        update();
        return true;
    }
    if(recv?change_flow_recv(id,st,type,flag):change_flow_send(st,type,flag)){
        if(st!=HTTP2Status::closed)update();
        return true;
    }
    return false;
}

bool HTTP2FlowControlLayer::change_flow_recv(int id,HTTP2Status& st,H2FType type,H2Flag flag){
    if(st==HTTP2Status::idle&&type==HEADER){
        st=HTTP2Status::open;
        maxid=id>=maxid?id:maxid;
    }
    if(st==HTTP2Status::reserved_local&&type==WINDOW_UPDATE){
    }
    else if(st==HTTP2Status::reserved_remote&&type==HEADER){
        st=HTTP2Status::half_closed_local;
    }
    else if(st==HTTP2Status::open){
        if((type==DATA||type==HEADER)&&(flag&END_STREAM)){
            st=HTTP2Status::half_closed_remote;
        }
    }
    else if(st==HTTP2Status::half_closed_local){
        if((type==DATA||type==HEADER)&&(flag&END_STREAM)){
            st=HTTP2Status::closed;
        }
    }
    else if(st==HTTP2Status::half_closed_remote){
        if(type!=WINDOW_UPDATE){
            send_rst(id,HTTP2_STREAM_CLOSED);
            pop(false);
            return true;
        }
    }
    else if(st==HTTP2Status::closed&&on_allow_delta(id)&&
    (type==WINDOW_UPDATE||type==RST_STREAM)){
        pop(true);
        return true;
    }
    else{
        send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    return true;
}

bool HTTP2FlowControlLayer::change_flow_send(HTTP2Status& st,H2FType type,H2Flag flag){
    if(st==HTTP2Status::idle&&type==HEADER){
        st=HTTP2Status::open;
    }
    if(st==HTTP2Status::reserved_remote&&type==WINDOW_UPDATE){
    }
    else if(st==HTTP2Status::reserved_local&&type==HEADER){
        st=HTTP2Status::half_closed_remote;
    }
    else if(st==HTTP2Status::open){
        if((type==DATA||type==HEADER)&&(flag&END_STREAM)){
            st=HTTP2Status::half_closed_local;
        }
    }
    else if(st==HTTP2Status::half_closed_remote){
        if((type==DATA||type==HEADER)&&(flag&END_STREAM)){
            st=HTTP2Status::closed;
        }
    }
    else if(st==HTTP2Status::half_closed_local&&type==WINDOW_UPDATE){
    }
    else{
        return false;
    }
    return true;
}

bool HTTP2FlowControlLayer::send_frame(H2FType type,H2Flag flag,int id,char* data,int size){
    if(!change_flow(false,id,type,flag))return false;
    return frames.send_frame(type,flag,id,data,size);
}

bool HTTP2FlowControlLayer::recv_frame(Reader<std::string>& reader,int framesize){
    if(!frames.read_set_of_frames(reader,framesize))return false;
    if(frames.size()){
        auto& ref=frames.current();
        return change_flow(true,ref.id,(H2FType)ref.type,(H2Flag)ref.flag);
    }
    return true;
}

HTTP2StreamContext& HTTP2FlowControlLayer::stream(int id){
    auto& ret=streams[id];
    if(ret.status==HTTP2Status::idle&&id<maxid)ret.status=HTTP2Status::closed;
    return ret;
}

bool HTTP2FlowControlLayer::send_rst(int id,unsigned int errorcode){
    auto codep=(char*)&errorcode;
    errorcode=translate_byte_net_and_host<unsigned int>(codep);
    return frames.send_frame(RST_STREAM,NONEF,id,codep,4);
}