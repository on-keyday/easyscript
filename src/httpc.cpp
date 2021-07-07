#include"httpc/sockinterpreter.h"
#include"httpc/socket.h"
#include"httpc/refactering/httpc.h"
#include<argvlib.h>
#include<iostream>

int main(int argc,char** argv){
    //setlocale(LC_ALL,".UTF-8");
    WSAData data;
    if(WSAStartup(MAKEWORD(2,2),&data)){
        std::cout << "wsa init failed.";
        return -1;
    }
    SSL_load_error_strings();
    auto ret=netclient_str(GetCommandLineA());
    commonlib2::ArgChange _(argc,argv);
    ret=command_argv(argc,argv);
    //http2_test();
    WSACleanup();
    return ret;
}