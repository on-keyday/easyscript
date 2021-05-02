/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include"socket.h"
#include<chrono>
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

bool ClientSocket::connect(unsigned short port){
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

bool SecureSocket::init(){
#if USE_SSL
    if (!ctx) {
		ctx = SSL_CTX_new(TLS_method());
		if (!ctx)return false;
		SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
		unsigned char proto[] = "\x8http/1.1";
		SSL_CTX_set_alpn_protos(ctx, proto, 9);
		SSL_CTX_load_verify_locations(ctx, cacert_file.c_str(), NULL);
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
		//SSL_CTX_set_info_callback(ctx,infocb);
	}
    return true;
#endif
    return false;
}

bool SecureSocket::connect(unsigned short port){
    if(sock.get_service()=="http")return sock.connect(port);
#if USE_SSL
    if(isconnected())return true;
    if(!init())return false;
    if(!sock.connect())return false;
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
    }
    no_shutdown=false;
#endif
    sock.close();
    return true;
}

SecureSocket::~SecureSocket(){
#if USE_SSL
    if(ctx){
        SSL_CTX_free(ctx);
    }
#endif
}

bool HTTPClient::parseurl(std::string url,URLContext<std::string>& ctx,unsigned short& port){
    ctx.needend=true;
    if(!Reader<std::string>(std::move(url)).readwhile(parse_url,ctx))return false;
    if(!ctx.succeed)return false;
    if(ctx.host.length()==0)return false;
    if(ctx.scheme.length()==0){
        ctx.scheme="http";
    }
    if(ctx.scheme!="http"&&ctx.scheme!="https")return false;
    if(ctx.path.length()==0){
        ctx.path=default_path;
    }
    if(ctx.port.length()>0){
        if(ctx.port.length()>5)return false;
        auto t=std::stoul(ctx.port);
        if(t>0xffff)return false;
        port=(unsigned short)t;
    }
    return true;
}

bool HTTPClient::urlencode(URLContext<std::string>& ctx){
    std::string path,query;
    URLEncodingContext<std::string> uctx;
    uctx.path=true;
    Reader<std::string>(std::move(ctx.path)).readwhile(path,url_encode,&uctx);
    ctx.path=std::move(path);
    uctx.path=false;
    uctx.query=true;
    Reader<std::string>(std::move(ctx.query)).readwhile(query,url_encode,&uctx);
    ctx.query=std::move(query);
    return true;
}

bool HTTPClient::make_request(std::string& ret,const char* method,URLContext<std::string>& ctx,const char* body,size_t size){
    ret.append(method);
    ret.push_back(' ');
    ret.append(ctx.path);
    ret.append(ctx.query);
    ret.append(" HTTP/1.1\r\nhost: ");
    ret.append(ctx.host);
    if(ctx.port.size()){
        ret.push_back(':');
        ret.append(ctx.port);
    }
    ret.append("\r\n");
    if(body&&size){
        ret.append("content-length: ");
        ret.append(std::to_string(size));
        ret.append("\r\n");
    }
    ret.append("\r\n");
    if(body&&size){
        ret.append(body,size);
    }
    return true;
}

bool HTTPClient::parse_response(Reader<std::string>& response,bool ishead){
    HTTPResponse<HeaderMap,std::string> ctx;
    while(true){
        response.readwhile(httpresponse,ctx);
        if(!ctx.succeed){
            if(ctx.header.synerr)return false;
            sock.recv(response.ref());
            response.seek(0);
            continue;
        }
        break;
    }
    if(!ishead)
        read_body(response,ctx);
    resinfo=std::move(ctx);
    _raw=std::move(response.ref());
    return true;
}

bool HTTPClient::read_body(Reader<std::string>& response,HTTPResponse<HeaderMap,std::string>& ctx){
    auto recver=[this](std::string& buf){return sock.recv(buf);};
    HTTPBodyContext<std::string,HeaderMap,decltype(recver)> bctx(ctx.header.buf,recver);
    response.readwhile(_body,httpbody,&bctx);
    if(!bctx.succeed){
        auto beginpos=response.readpos();
        auto it=ctx.header.buf.equal_range("content-type");
        if(it.first!=it.second){
            auto& str=(*it.first).second;
            if(str.find("json")!=std::string::npos){
                response.set_ignore(ignore_space_line);
                if(response.ahead("{")){
                    BasicBlock<std::string> check(true,false,false);
                    while(!response.readwhile(depthblock,check)){
                        if(!sock.recv(response.ref()))return false;
                        response.seek(beginpos);
                    }
                }
                else if(response.ahead("[")){
                    /*while(!response.depthblock([](auto self,bool begin)->bool{
                        if(begin){
                            return self->expect("[");
                        }
                        else{
                            return self->expect("]");
                        }
                    }))*/
                    BasicBlock<std::string> check(false,true,false);
                    while(response.readwhile(depthblock,check)){
                        if(!sock.recv(response.ref()))return false;
                        response.seek(beginpos);
                    }
                }
            }
            else if(str.find("html")!=std::string::npos){
                bool html = false;
                response.set_ignore(ignore_space_line);
                while (!response.eof()) {
                    char tmp = 0;
                    response.read_byte(&tmp);
                    if (tmp == '<') {
                        if (response.expect("html")||response.expect("HTML")) {
                            html = true;
                            break;
                        }
                    }
                }
                while (html) {
                    if (response.eof()) {
                        if (!sock.recv(response.ref()))return false;
                    }
                    char tmp = 0;
                    response.read_byte(&tmp);
                    if (tmp == '<') {
                        if (response.eof()) {
                            if(!sock.recv(response.ref()))return false;
                        }
                        if (response.expect("/")) {
                            if (response.eof()) {
                                if(!sock.recv(response.ref()))return false;
                            }
                            if (response.expect("html") || response.expect("HTML")) {
                                break;
                            }
                        }
                    }
                }
            }
        }
        response.set_ignore(nullptr);
        response.seek(beginpos);
        _body="";
        while(!response.ceof()){
            _body.push_back(response.achar());
            response.increment();
        }
    }
    return true;
}

bool HTTPClient::method_detail(const char* method,const char* url,const char* body,size_t size,bool nobody){
    if(!method||!url)return false;
    URLContext<std::string> ctx;
    unsigned short port=0;
    if(!parseurl(url,ctx,port))return false;
    if(!encoded)urlencode(ctx);
    std::string request;
    if(!make_request(request,method,ctx,body,size))return false;
    if(!sock.open_if_differnet(ctx.host.c_str(),ctx.scheme.c_str()))return false;
    if(!sock.connect(port)){
        sock.close();
        return false;
    }
    if(!sock.send(request)){
        sock.close();
        return false;
    }
    Reader<std::string> response;
    if(!sock.recv(response.ref())){
        sock.close();
        return false;
    }
    if(!parse_response(response,nobody)){
        sock.close();
        return false;
    }
    if(!keep_alive)sock.close();
    return true;
}

bool HTTPClient::method(const char* method,const char* url,const char* body,size_t size,bool nobody){
    if(depth>100)return false;
    bool res=false;
    auto begin=std::chrono::system_clock::now();
    if(method_detail(method,url,body,size,nobody)){
        if(resinfo.statuscode>=301&&resinfo.statuscode<=308){
            if(resinfo.header.buf.count("location")){
                std::string locale=(*resinfo.header.buf.equal_range("location").first).second;
                auto tmphold=encoded;
                encoded=true;
                depth++;
                res=this->method(method,locale.c_str(),body,size,nobody);
                depth--;
                encoded=tmphold;
            }
        }
        else{
            res=true;
        }
    }
    auto end=std::chrono::system_clock::now();
    _time=std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
    return res;
}

bool HTTPClient::method_if_exist(const char* method,const char* url,const char* body,size_t size){
    Reader<std::string> reader(method);
    auto cmp=[](char c1,char c2){
        auto a=tolower((unsigned char)c1);
        auto b=tolower((unsigned char)c2);
        return a==b;
    };
    auto check=[&reader,&cmp](const char* m){return reader.expect(m,nullptr,cmp);};
    if(check("GET")){
        return get(url);
    }
    else if(check("HEAD")){
        return head(url);
    }
    else if(check("POST")){
        return post(url,body,size);
    }
    else if(check("PUT")){
        return put(url,body,size);
    }
    else if(check("PATCH")){
        return patch(url,body,size);
    }
    else if(check("OPTIONS")){
        return options(url);
    }
    else if(check("TRACE")){
        return trace(url,body,size);
    }
    else if(check("DELETE")){
        return _delete(url);
    }
    return false;
}