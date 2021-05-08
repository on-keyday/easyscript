#include"http1.h"
#if USE_SSL
#include<openssl/err.h>
#endif
using namespace PROJECT_NAME;

bool HTTPClient::parseurl(std::string url,URLContext<std::string>& ctx,unsigned short& port){
    ctx.needend=true;
    Reader<std::string> check(url);
    if(!check.readwhile(parse_url,ctx)||!ctx.succeed){
        set_err("syntax of url invalid:"+url+"(pos:"+std::to_string(check.readpos()+1)+")");
        return false;
    }
    if(ctx.host.length()==0){
        set_err("no host name exist:"+url);
        return false;
    }
    if(ctx.scheme.length()==0){
        ctx.scheme="http";
    }
    if(ctx.scheme!="http"&&ctx.scheme!="https"){
        set_err("expected scheme is http or https, but "+ctx.scheme);
        return false;
    }
    if(ctx.path.length()==0){
        ctx.path=default_path;
    }
    if(ctx.port.length()>0){
        if(ctx.port.length()>5){
            set_err("too large port number:"+ctx.port);
            return false;
        }
        unsigned long t=0;
        try{
            t=std::stoul(ctx.port);
        }
        catch(...){
            set_err("port number:not number:"+ctx.port);
            return false;
        }
        if(t>0xffff){
            set_err("too large port number:"+ctx.port);
            return false;
        }
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
    if(request_adder){
        if(!request_adder(reqctx,ret,this))return false;
    }
    if(body&&size){
        ret.append("content-length: ");
        ret.append(std::to_string(size));
        ret.append("\r\n");
    }
    ret.append("\r\n");
    if(body&&size){
        ret.append(body,size);
    }
    _request=ret;
    return true;
}

bool HTTPClient::parse_response(Reader<std::string>& response,bool ishead){
    HTTPResponse<HeaderMap,std::string> ctx;
    while(true){
        response.readwhile(httpresponse,ctx);
        if(!ctx.succeed){
            if(ctx.header.synerr){
                set_err("header syntax error");
                return false;
            }
            ctx.reason="";
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
    if(!make_request(request,method,ctx,body,size)){
        set_err("make request failed.");
        return false;
    }
    std::string openhost=ctx.host;
    if(openhost[0]=='['){
        openhost.erase(0,1);
        openhost.pop_back();
    }
    if(!sock.open_if_differnet(openhost.c_str(),ctx.scheme.c_str())){
        set_err("host name was not resolved:"+ctx.host);
        return false;
    }
    if(!sock.connect(port,true)){
        set_err("connet error");
#if USE_SSL
        std::string got;
        ERR_print_errors_cb([](const char* str,size_t len,void* to){
            std::string* g=(std::string*)to;
            g->append(str,len);
            return 1;
        },&got);
        if(got!=""){
            set_err("connect err:"+got);
        }
#endif
        sock.close();
        return false;
    }
    if(!sock.send(request)){
        set_err("send error");
        sock.close();
        return false;
    }
    Reader<std::string> response;
    if(!sock.recv(response.ref())){
        set_err("recv error");
        sock.close();
        return false;
    }
    if(!parse_response(response,nobody)){
        sock.close();
        return false;
    }
    set_ip_addr();
    if(!keep_alive)sock.close();
    set_err("no error");
    return true;
}

bool HTTPClient::set_ip_addr(){
    auto info=sock.get_basesock().getinfo();
    char addrbuf[70]={0};
    if(info->ai_family==AF_INET){
        auto addr=(sockaddr_in*)info->ai_addr;
        auto tmp=inet_ntop(AF_INET,&addr->sin_addr,addrbuf,70);
        if(!tmp)return false;
    }
    else if(info->ai_family==AF_INET6){
        auto addr=(sockaddr_in6*)info->ai_addr;
        auto tmp=inet_ntop(AF_INET6,&addr->sin6_addr,addrbuf,70);
        if(!tmp)return false;
    }
    else{
        _ipaddr="";
        return false;
    }
    _ipaddr=addrbuf;
    return true;
}

bool HTTPClient::method(const char* method,const char* url,const char* body,size_t size,bool nobody){
    if(depth>100)return false;
    bool res=false;
    auto begin=std::chrono::system_clock::now();
    if(method_detail(method,url,body,size,nobody)){
        if(auto_redirect&&resinfo.statuscode>=301&&resinfo.statuscode<=308){
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
    _time=end-begin;
    return res;
}

bool HTTPClient::set_requestadder(bool(*add)(void*,std::string&,const HTTPClient*),void* ctx){
    if(add==request_adder)return true;
    if(!add){
        request_adder=add;
        return true;
    }
    std::string teststr;
    add(ctx,teststr,this);
    if(teststr=="")return false;
    teststr+="\r\n";
    HTTPHeaderContext<std::multimap<std::string,std::string>,std::string> headertest;
    Reader<std::string> check(teststr);
    check.readwhile(httpheader,headertest);
    if(!headertest.succeed)return false;
    if(!check.ceof())return false;
    if(headertest.buf.count("content-length")||headertest.buf.count("host"))return false;
    request_adder=add;
    reqctx=ctx;
    return true;
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
        return trace(url);
    }
    else if(check("DELETE")){
        return _delete(url,body,size);
    }
    set_err("method '"+std::string(method)+"' not found");
    return false;
}