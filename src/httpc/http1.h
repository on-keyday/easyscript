#pragma once
#include"socket.h"
namespace PROJECT_NAME{
    struct HTTPClient{
    private:
        SecureSocket sock;
        //std::string proxy;
        bool keep_alive=false;
        bool auto_redirect=false;
        std::string default_path="/";
        bool encoded=false;
        std::string _body;
        std::string _raw;
        int depth=0;
        using HeaderMap=std::multimap<std::string,std::string>;
        HTTPResponse<HeaderMap,std::string> resinfo;
        std::chrono::system_clock::duration _time;
        //long long _time=0;
        std::string _err;
        std::string _ipaddr;
        std::string _request;
        void* reqctx=nullptr;
        bool (*request_adder)(void*,std::string&,const HTTPClient*)=nullptr;
        bool set_err(std::string reason){_err=reason;return true;}
        bool parseurl(std::string url,URLContext<std::string>& ctx,unsigned short& port);
        bool urlencode(URLContext<std::string>& ctx);
        bool make_request(std::string& ret,const char* method,URLContext<std::string>& ctx,const char* body,size_t size);
        bool parse_response(Reader<std::string>& response,bool ishead);
        bool read_body(Reader<std::string>& response,HTTPResponse<HeaderMap,std::string>& ctx);
        bool set_ip_addr();
        bool method_detail(const char* method,const char* url,const char* body,size_t size,bool nobody);
    public:
        bool method(const char* method,const char* url,const char* body=nullptr,size_t size=0,bool nobody=false);
        const std::string& raw()const{return _raw;}
        const std::string& body()const{return _body;}
        const std::chrono::system_clock::duration& time()const{return _time;}
        const std::string& err()const{return _err;}
        const std::string& ipaddress() const{return  _ipaddr;}
        const std::string& requesst() const {return _request;}
        bool set_cacert(const std::string& file){return sock.set_cacert(file);}
        bool set_infocb(void(*cb)(const void*,int,int)){return sock.set_infocb(cb);}
        bool set_verifycb(int(*cb)(int, void*)){return sock.set_verifycb(cb);}
        bool set_default_path(const std::string& path){default_path=path;return true;}
        bool set_encoded(bool val){encoded=val;return true;}
        bool set_auto_redirect(bool val){auto_redirect=val;return true;}
        bool set_requestadder(bool(*add)(void*,std::string&,const HTTPClient*),void* ctx);
        bool get(const char* url){return method("GET",url);}
        bool head(const char*url){return method("HEAD",url,nullptr,0,true);}
        bool post(const char* url,const char* body,size_t size){
            if(!body||!size){
                set_err("no body is not allowed for POST");
                return false; 
            }
            return method("POST",url,body,size);
        }
        bool put(const char* url,const char* body,size_t size){
            if(!body||!size){
                set_err("no body is not allowed for PUT");
                return false; 
            }
            return method("PUT",url,body,size);
        }
        bool patch(const char* url,const char* body,size_t size){
            if(!body||!size){
                set_err("no body is not allowed for PATCH");
                return false; 
            }
            return method("PATCH",url,body,size);
        }
        bool options(const char* url){return method("OPTIONS",url);}
        bool trace(const char* url){return method("TRACE",url);}
        bool _delete(const char* url,const char* body=nullptr,size_t size=0){return method("DELETE",url,body,size);}
        unsigned short statuscode()const{return resinfo.statuscode;}
        const std::string& reasonphrase()const{return resinfo.reason;}
        int version()const{return resinfo.version;}
        auto header()->
        decltype(resinfo.header.buf)const{return resinfo.header.buf;}
        bool method_if_exist(const char* method,const char* url,const char* body=nullptr,size_t size=0);
    };
}