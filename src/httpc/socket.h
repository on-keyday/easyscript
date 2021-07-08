/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"../commonlib/project_name.h"
#include"../commonlib/net_helper.h"
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
#include <arpa/inet.h>
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
        bool connect(unsigned short port=0,bool nodelay=false);
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
        int err=0;
#if USE_SSL
        SSL_CTX* ctx=nullptr;
        SSL* ssl=nullptr;
        bool no_shutdown=false;
        bool isconnected(){return ssl!=nullptr;}
        std::string cacert_file;
        void (*infocb)(const void*,int,int)=nullptr;
        int (*verifycb)(int, void*)=nullptr;
#endif
        bool failed(int res=0);
        bool init(const unsigned char* alpnstr,int len);
    public:
        bool open(const char* hostname,const char* service,int family=0,int type=0);
        bool open_if_differnet(const char* hostname,const char* service,int family=0,int type=0);
        bool connect(unsigned short port=0,bool nodelay=false,const char* alpnstr="\x8http/1.1",int len=9);
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
                    //case(SSL_ERROR_NONE):
					case(SSL_ERROR_WANT_READ):
					case(SSL_ERROR_WANT_WRITE):
						retry = true;
						break;
                    case(SSL_ERROR_SYSCALL):
#ifdef _WIN32
                        err=WSAGetLastError();
#else
                        err=errno;
#endif
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
            cacert_file=file;
            return true;
#endif      
            return false;
        }

        bool set_infocb(void (*cb)(const void*,int,int)){
#if USE_SSL
            infocb=cb;
            if(ctx)
                SSL_CTX_set_info_callback(ctx,(void(*)(const SSL*,int,int))infocb);
            return true;
#endif  
            return false;
        }

        bool set_verifycb(int(*cb)(int, void*)){
#if USE_SSL
            verifycb=cb;
            if(ctx)SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,(int(*)(int, X509_STORE_CTX *))verifycb);
            return true;
#endif  
            return false;
        } 

        ClientSocket& get_basesock(){return sock;}
#if USE_SSL
        SSL* get_basessl(){return ssl;}
        SSL_CTX* get_basesslctx(){return ctx;}
#endif
    };
}