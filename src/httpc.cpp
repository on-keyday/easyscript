#include"httpc/sockinterpreter.h"
#include"httpc/socket.h"
#include"httpc/refactering/httpc.h"
#include<iostream>

int main(int argc,char** argv){
    WSAData data;
    if(WSAStartup(MAKEWORD(2,2),&data)){
        std::cout << "wsa init failed.";
        return -1;
    }
    SSL_load_error_strings();
    auto ret=netclient_str(GetCommandLineA());
    command_wchar(GetCommandLineW());
    //http2_test();
    WSACleanup();
    return ret;
}