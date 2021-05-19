/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#define DLL_EXPORT __declspec(dllexport)
#include"hardtest.h"
#include"control.h"
#include"../commonlib/json_util.h"
#include<iostream>
#include<filesystem>
using namespace PROJECT_NAME;
using namespace control;
namespace PROJECT_NAME{
    void to_json(JSON& to,const std::filesystem::directory_entry& dir){
        to[dir.path().filename().string()]=dir.file_size();
    }

    void to_json(JSON& to,const std::filesystem::path& dir){
        std::filesystem::directory_iterator it(dir),end;
        std::error_code code;
        auto& cd=to[dir.filename().string()];
        cd="{}"_json;
        for(;it!=end&&!code;it.increment(code)){
            to_json(cd,*it);
        }
        if(code){
            to=JSON();
        }
    }
}

int STDCALL hardtest(){
    auto code=
R"(
    decl printf(string,...)->int @(cdecl);
    //decl call([]string)->int;
    func main(argv string...)  {
        if argv.len() < 1 {
            return 0;
        }
        printf(argv[0]);
        return $$;
    }

    /*
    call:=main("aho","hage");

    a:=[0,1,2,3,4][4];

    call();*/
    
    decl RunFunctionOnThread(*void,...);

    func th(f,arg ...){
        return RunFunctionOnThread(f,arg);
    }

    th($(num){
        return num<=1?num:$$(num-1)+$$(num-2);
    },20);
    i()()();
)";
    Reader<std::string> test(code,ignore_c_comments);
    std::vector<Control> globalvec;
    auto t=parse_all(test,globalvec);
    LinePosContext ctx;
    test.readwhile(linepos,ctx);
    JSON json(globalvec);
    std::cout << json.to_string(4);
    std::filesystem::path cd;
    cd=std::filesystem::current_path();
    json=JSON(cd);
    auto m=json.path(R"(/)");
    std::cout << m->to_string(4);
    return 0;
}

