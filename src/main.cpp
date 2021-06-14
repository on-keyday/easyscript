/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include"commonlib/reader.h"
#include"commonlib/basic_helper.h"
#include<string>
#include<vector>
#include<map>
#include<iostream>
#include"easyscript/script.h"
#include"hardscript/hardtest.h"
#include"bunkai/ast.h"
#include<fileio.h>

using namespace PROJECT_NAME;
using strreader=Reader<std::string>;

#ifdef _WIN32
#include<Windows.h>
struct DLL{
    void* handle=nullptr;
};

int User32Proxy(void* ctx,const char* membname,ArgContext* arg){
    std::string name=membname;
    if(name=="__init__"){
        MessageBoxA(nullptr,std::to_string(get_instance_id(arg)).c_str(),"test",MB_ICONINFORMATION);
        return 0;
    }
    if(name=="MessageBoxA"){
        auto msg=get_arg_index(arg,1);
        auto cap=get_arg_index(arg,2);
        if(!msg||!cap){
            set_result_error(arg,"arg not matched");
            return -1;
        }
        MessageBoxA(nullptr,msg,cap,MB_ICONINFORMATION);
        return 0;
    }
    if(name=="__delete__"){
        MessageBoxA(nullptr,"delete object!","test",MB_ICONWARNING);
    }
    return 0;
}
#endif
int ConsoleProxy(void* ctx,const char* membname,ArgContext* arg){
    std::string name=membname;
    if(name=="println"){
        if(get_arg_index(arg,0)){
            std::cout<< get_arg_index(arg,0) << "\n";
        }
    }

    return 0;
}

int WaitProxy(void* ctx,const char* membname,ArgContext* arg){
    std::string name=membname;
    if(name=="pause"){
        ::system("pause");
    }
#ifdef _WIN32
    else if(name=="sleep"){
        Sleep(1000);
    }
#endif
    return 0;
}

int main(int argc,char** argv){
    auto script=make_script();
    if(!script)return -1;
    //add_sourece_from_file(script,"D:\\CommonLib\\CommonLib2\\src\\easyscript\\easytest.ess");
#ifdef _WIN32
    add_builtin_object(script,"User32",User32Proxy,&argc);
    add_builtin_object(script,"Wait",WaitProxy,&argc);
#endif
    add_builtin_object(script,"Console",ConsoleProxy,&argc);
    execute(script,1);
    //std::cout << get_result(script)<< "\n";
    delete_script(script);
    //hardtest();
    //auto ret=netclient_argv(argc,argv);
    ast::type::TypePool pool;
    ast::AstReader r(FileInput(R"(D:\CommonLib\CommonLib2\src\bunkai_src\define_asm.asd)"),pool);
    ast::AstToken* tok=nullptr;
    const char* err=nullptr;
    r.parse(tok,&err);
    ast::delete_token(tok);
    ast::check_assert();
    return 0;
}
