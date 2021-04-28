/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include"reader.h"
#include"reader_helper.h"
#include"socket.h"
#include<string>
#include<vector>
#include<map>
#include<iostream>
#include"easyscript/script.h"

#include<Windows.h>

using namespace PROJECT_NAME;
using strreader=Reader<std::string>;

struct DLL{
    void* handle=nullptr;
};

int User32Proxy(void* ctx,const char* membname,ArgContext* arg){
    std::string name=membname;
    if(name=="__init__")return 0;
    if(name=="MessageBoxA"){
        auto msg=get_arg_index(arg,1);
        auto cap=get_arg_index(arg,2);
        if(!msg||!cap)return -1;
        MessageBoxA(nullptr,msg,cap,MB_ICONINFORMATION|MB_OKCANCEL);
    }
    else if(name=="sleep"){
        Sleep(1000);
    }
    return 0;
}

int ConsoleProxy(void* ctx,const char* membname,ArgContext* arg){
    std::string name=membname;
    if(name=="println"){
        if(get_arg_index(arg,0)){
            std::cout<< get_arg_index(arg,0) << "\n";
        }
    }
    else if(name=="pause"){
        ::system("pause");
    }
    return 0;
}

int main(int argc,char** argv){
    /*if(argc<=1){
        
    }
    WSAData data;
    WSAStartup(MAKEWORD(2,2),&data);
    HTTPClient client;
    client.set_auto_redirect(true);
    client.get("http://google.com/");
    std::cout << client.raw() << "\n" << client.time() << "\n";
    WSACleanup();
    sizeof(HTTPClient);*/
    auto script=make_script();
    if(!script)return -1;
    add_sourece_from_file(script,"D:\\CommonLib\\CommonLib2\\src\\easyscript\\easytest.ess");
    add_builtin_object(script,"User32",User32Proxy,&argc);
    add_builtin_object(script,"Console",ConsoleProxy,&argc);
    execute(script,1);
    delete_script(script);
    return 0;
}