#include"sockinterpreter.h"
#include"socket.h"
#include<iostream>

int main(int argc,char** argv){
    WSAData data;
    if(WSAStartup(MAKEWORD(2,2),&data)){
        std::cout << "wsa init failed.";
        return -1;
    }
    auto ret=netclient_argv(argc,argv);
    WSACleanup();
    return ret;
}