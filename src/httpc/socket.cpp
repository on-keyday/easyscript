/*license*/
#include"socket.h"
using namespace PROJECT_NAME;

bool ClientSocket::open(const char* hostname,const char* service,int family,int type){
    if(isopened()||!hostname)return false;
    addrinfo hint={0};
    if(family)hint.ai_family=family;
    if(type)hint.ai_socktype=type;
    if(getaddrinfo(hostname,service,&hint,&infolist)!=0)
        return false;
    this->hostname=hostname;
    if(service)this->service=service;
    else this->service="";
    return true;
}

 bool ClientSocket::open_if_differnet(const char* hostname,const char* service,int family,int type){
     if(this->hostname!=hostname||this->service!=service){
         close();
         return open(hostname,service,family,type);
     }
     return true;
 }

bool ClientSocket::connect(unsigned short port,bool nodelay){
    if(!isopened())return false;
    if(isconnected())return true;
    auto port_net=translate_byte_net_and_host<unsigned short>((char*)&port);
    for(auto info=infolist;info;info=info->ai_next){
        sockaddr_in* addr=(sockaddr_in*)info->ai_addr;
        if(port_net){
            addr->sin_port=port_net;
        }
        auto tmp=::socket(info->ai_family,info->ai_socktype,info->ai_protocol);
        if(tmp<0)continue;
        if(::connect(tmp,info->ai_addr,info->ai_addrlen)==0){
            sock=tmp;
            selected=info;
            break;
        }
        ::closesocket(sock);
    }
    if(isconnected()&&nodelay){
        char opt=nodelay;
        if(setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,&opt,1)<0){
            disconeect();
            return false;
        }
    }
    return isconnected();
}

bool ClientSocket::disconeect(){
    if(!isconnected())return false;
    shutdown(sock,SD_SEND);
    shutdown(sock,SD_RECEIVE);
    ::closesocket(sock);
    sock=invalid;
    return true;
}

bool ClientSocket::close(){
    disconeect();
    if(infolist){
        freeaddrinfo(infolist);
        infolist=nullptr;
        selected=nullptr;
    }
    hostname="";
    service="";
    return true;
}

bool ClientSocket::sync(bool val){
    if(isconnected()){
        u_long i=(u_long)!val;
        ioctlsocket(sock,FIONBIO,(u_long*)&i);
        return true;
    }
    return false;
}

bool SecureSocket::failed(int res){
#if USE_SSL
    auto reason = SSL_get_error(ssl, res);
    switch (reason)
    {
    case(SSL_ERROR_WANT_READ):
    case(SSL_ERROR_WANT_WRITE):
        return false;
    default:
        return true;
    }
#endif
    return true; 
}

bool SecureSocket::open(const char* hostname,const char* service,int family,int type){
    return sock.open(hostname,service,family,type);
}

bool SecureSocket::open_if_differnet(const char* hostname,const char* service,int family,int type){
    if(sock.get_hostname()!=hostname||sock.get_service()!=service){
        close();
        return open(hostname,service,family,type);
    }
    return true;
}

bool SecureSocket::init(const unsigned char* alpnstr,int len){
#if USE_SSL
    if (!ctx) {
		ctx = SSL_CTX_new(TLS_method());
		if (!ctx)return false;
		SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
		//unsigned char proto[] = "\x8http/1.1";
		SSL_CTX_set_alpn_protos(ctx, alpnstr, len);
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, (int(*)(int, X509_STORE_CTX *))verifycb);
		SSL_CTX_set_info_callback(ctx,(void(*)(const SSL*,int,int))infocb);
	}
    return (bool)SSL_CTX_load_verify_locations(ctx, cacert_file.c_str(), NULL);
#endif
    return false;
}

bool SecureSocket::connect(unsigned short port,bool nodelay,const char* alpnstr,int len){
    if(sock.get_service()=="http")return sock.connect(port,nodelay);
#if USE_SSL
    if(isconnected())return true;
    if(!init((const unsigned char*)alpnstr,len))return false;
    if(!sock.connect(port,nodelay))return false;
    ssl=SSL_new(ctx);
    if (!ssl)return false;
	SSL_set_fd(ssl, sock.getsocket());
	SSL_set_tlsext_host_name(ssl, sock.get_hostname().c_str());
	auto param = SSL_get0_param(ssl);
	if (!X509_VERIFY_PARAM_add1_host(param,sock.get_hostname().c_str(), 0)){
        no_shutdown = true;
		return false;
    }
    if (SSL_connect(ssl) != 1) {
		//invoke_errcb();
		no_shutdown = true;
		return false;
	}
    return true;
#endif
    return false;
}

bool SecureSocket::close(){
#if USE_SSL
    if(ssl){
        while (true) {
			auto res = SSL_shutdown(ssl);
			if (res < 0) {
				if(failed(res))break;
                continue;
			}
			else if (!res) {
				continue;
			}
			break;
		}
        SSL_free(ssl);
        ssl=nullptr;
    }
    no_shutdown=false;
#endif
    sock.close();
    return true;
}

SecureSocket::~SecureSocket(){
#if USE_SSL
    if(ssl){
        SSL_free(ssl);
    }
    if(ctx){
        SSL_CTX_free(ctx);
    }
#endif
}
