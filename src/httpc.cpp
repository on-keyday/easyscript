#include"httpc/sockinterpreter.h"
#include"httpc/socket.h"
#include"httpc/refactering/httpc.h"
#include<argvlib.h>
#include<iostream>

int main(int argc,char** argv){
    //setlocale(LC_ALL,".UTF-8");

#ifdef _WIN32
    WSAData data;
    if(WSAStartup(MAKEWORD(2,2),&data)){
        std::cout << "wsa init failed.";
        return -1;
    }
#endif
    SSL_load_error_strings();
    int ret=0;
#ifdef _WIN32
    ret=netclient_str(GetCommandLineA());
#endif
    commonlib2::ArgChange _(argc,argv);
    ret=command_argv(argc,argv);
    //http2_test();
#ifdef _WIN32
    WSACleanup();
#endif
    return ret;
}