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

using namespace PROJECT_NAME;
using strreader=Reader<std::string>;

int TestProxy(char* result,void* ctx,const char* membname,int argc,const char** argv){
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
    add_sourece_from_file(script,"D:\\CommonLib\\CommonLib2\\src\\easyscript\\easytest.inn");
    add_builtin_object(script,"MsgInvoke",TestProxy,&argc);
    execute(script,1);
    delete_script(script);
    return 0;
}