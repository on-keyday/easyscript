/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "project_name.h"
#include"reader_helper.h"
#include<string>
#include<map>
#include<chrono>
#ifdef _WIN32
#ifdef __MINGW32__
#define _WIN32_WINNT 0x0501
#endif
#include<WinSock2.h>
#include<WS2tcpip.h>
#else
#include<unistd.h>
#include<sys/ioctl.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define closesocket close
#define ioctlsocket ioctl
#define SD_SEND SHUT_WR
#define SD_RECEIVE SHUT_RD
#endif
#define USE_SSL 1
#if USE_SSL
#include<openssl/ssl.h>
#endif
namespace PROJECT_NAME{

    struct ClientSocket{
    private:
        const int invalid=~0;
        addrinfo* infolist=nullptr;
        addrinfo* selected=nullptr;
        int sock=invalid;
        std::string hostname;
        std::string service;
        bool isopened(){return infolist!=nullptr;};
        bool isconnected(){return sock!=invalid;};
    public:
        bool open(const char* hostname,const char* service,int family=0,int type=0);
        bool open_if_differnet(const char* hostname,const char* service,int family=0,int type=0);
        bool connect(unsigned short port=0);
        int getsocket(){return sock;}
        const addrinfo* getinfo(){return selected;}
        const std::string& get_service(){return service;}
        const std::string& get_hostname(){return hostname;}
        template<class Buf>
        bool send(Buf& buf){
            if(!isconnected())return false;
            auto remain=buf.size();
            size_t ofs=0;
            while(true){
                auto sendsize=remain>0x7fffffff?0x7fffffff:remain;
                auto i=::send(sock,&buf.data()[ofs*0x7fffffff],sendsize,0);
                if(i<0)return false;
                if(sendsize<0x7fffffff)break;
                ofs++;
                remain-=0x7fffffff;
            }
            return true;
        }

        template<class Buf>
        bool recv(Buf& buf){
            if(!isconnected())return false;
            char tmp[1024]={0};
            while(true){
                auto size=::recv(sock,tmp,1024,0);
                if(size<0)return false;
                for(auto i=0;i<size;i++)buf.push_back(tmp[i]);
                if(size<1024)break;
            }
            return true;
        }
        bool disconeect();
        bool close();
        bool sync(bool val);
        ~ClientSocket(){
            close();
        }
    };

    
    struct SecureSocket{
    private:
        ClientSocket sock;
#if USE_SSL
        SSL_CTX* ctx=nullptr;
        SSL* ssl=nullptr;
        bool no_shutdown=false;
        bool isconnected(){return ssl!=nullptr;}
        std::string cacert_file;
        void (*infocb)(const void*,int,int)=nullptr;
#endif
        bool failed(int res=0);
        bool init();
    public:
        bool open(const char* hostname,const char* service,int family=0,int type=0);
        bool open_if_differnet(const char* hostname,const char* service,int family=0,int type=0);
        bool connect(unsigned short port=0);
        template<class Buf>
        bool send(Buf& buf){
            if(sock.get_service()=="http")return sock.send(buf);
#if USE_SSL
            if (!isconnected())return false;
            size_t written = 0;
            if (!SSL_write_ex(ssl, buf.data(), buf.size(), &written)) {
                return false;
            }
            return true;
#endif
            return false;
        }
        template<class Buf>
        bool recv(Buf& buf){
            if(sock.get_service()=="http")return sock.recv(buf);
#if USE_SSL
            if (!isconnected())return false;
			char tmp[1024]={0};
            while (true) {
                size_t size=0;
				bool retry = false;
				if (!SSL_read_ex(ssl, tmp, 1024, &size)) {
					auto reason = SSL_get_error(ssl, 0);
					switch (reason)
					{
					case(SSL_ERROR_WANT_READ):
					case(SSL_ERROR_WANT_WRITE):
						retry = true;
						break;
					default:
						break;
					}
					if (retry)continue;
					return false;
				}
				for(auto i=0;i<size;i++)buf.push_back(tmp[i]);
                if(size<1024)break;
			}
            return true;
#endif
            return false;
        }
        bool close();
        ~SecureSocket();
        bool set_cacert(const std::string& file){
#if USE_SSL
            cacert_file=file;return true;
#endif      
            return false;
        }

        bool set_infocb(void (*cb)(const void*,int,int)){
#if USE_SSL
            infocb=cb;
            if(ctx)SSL_CTX_set_info_callback(ctx,(void(*)(const SSL*,int,int))infocb);
            return true;
#endif  
            return false;
        }

        ClientSocket& get_basesock(){return sock;}
    };

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
        bool set_cacert(const std::string& file){return sock.set_cacert(file);}
        bool set_infocb(void(*cb)(const void*,int,int)){return sock.set_infocb(cb);}
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