#include"http2.h"
using namespace PROJECT_NAME;

bool HTTP2FrameLayer::send_frame(H2FType type,H2Flag flag,int id,char* data,int size){
    if(id<0)return false;
    if(size<0||size>0xffffff)return false;
    if(size&&!data)return false;
    size<<=8;
    int len=size,ids=id;
    auto sizep=(char*)&len,idp=(char*)&ids;
    len=translate_byte_net_and_host<int>(sizep);
    ids=translate_byte_net_and_host<int>(idp);
    std::string frame;
    frame.append(sizep,3);
    frame.push_back(type);
    frame.push_back(flag);
    frame.append(idp,4);
    if(size)frame.append(data,size);
    if(callback)callback(type,flag,id,data,size,user);
    return send(frame);
}

bool HTTP2FrameLayer::select_recv(std::string& buf){
    auto sock_base=sock.get_basesock().getsocket();
    if(sock_base==~0)return false;
    timeval timeout;
    timeout.tv_sec=10;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock_base,&set);
    ::select(sock_base+1,&set,nullptr,nullptr,&timeout);
    if(FD_ISSET(sock_base,&set)){
        sock.recv(buf);
    }
    else{
        return false;
    }
    return true;
}

bool HTTP2FrameLayer::read_a_frame(Reader<std::string>& reader,int framesize){
    HTTP2Frame<std::string> frame;
    frame.maxlen=framesize;
    if(!select_recv(reader.ref()))return false;
    while(true){
        reader.readwhile(http2frame_reader,frame);
        if(frame.continues){
            if(!select_recv(reader.ref()))return false;
            continue;
        }
        break;
    }
    if(!frame.succeed){
        if(frame.len>frame.maxlen){
            send_error(HTTP2_FLOW_CONTROL_ERROR);
        }
        send_error(HTTP2_PROTOCOL_ERROR);
        return false;
    }
    frames.push_back(std::move(frame));
    return true;
}

bool HTTP2FrameLayer::read_set_of_frames(Reader<std::string>& reader,int framesize){
    if(!read_a_frame(reader,framesize)){
        return false;
    }
    if(frames.back().type==HEADER){
        auto id=frames.back().id;
        while(true){
            if(frames.back().flag&END_HEADERS)break;
            if(!read_a_frame(reader,framesize)){
                return false;
            }
            if(frames.back().type!=CONTINUATION||frames.back().id!=id){
                send_error(HTTP2_PROTOCOL_ERROR);
                return false;
            }
        }
    }
    else if(frames.back().type==PING){
        send_ping(true);
        frames.pop_back();
    }
    return true;
}

bool HTTP2FrameLayer::pop(bool succeed){
    if(!frames.size())return false;
    if(succeed){
        if(frames.front().id>0){
            auto n=frames.front().id;
            last_succeed=last_succeed>n?last_succeed:n;
        }
    }
    frames.pop_front();
    return true;
}

bool HTTP2FrameLayer::send_ping(bool back){
    if(back){
        auto& ref=frames.back();
        if(ref.id!=0){
            send_error(HTTP2_PROTOCOL_ERROR);
            return false;
        }
        if(ref.flag&ACK){
            return true;   
        }
        return send_frame(PING,ACK,0,ref.buf.data(),8);
    }
    return false;
}

bool HTTP2FrameLayer::send_error(unsigned int errorcode,const char* additional,size_t size){
    int last=last_succeed;
    char* lastp=(char*)&last;
    char* codep=(char*)&errorcode;
    last=translate_byte_net_and_host<unsigned int>(lastp);
    errorcode=translate_byte_net_and_host<unsigned int>(codep);
    std::string payloadbuf;
    payloadbuf.append(lastp,4);
    payloadbuf.append(codep,4);
    if(additional&&size){
        payloadbuf.append(additional,size);
    }
    if(payloadbuf.size()>0xffffff)return false;
    on_error=errorcode!=0;
    return send_frame(GOAWAY,NONEF,0,payloadbuf.data(),(int)payloadbuf.size());
}

