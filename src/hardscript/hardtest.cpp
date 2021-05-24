/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#define DLL_EXPORT __declspec(dllexport)
#include"hardtest.h"
#include"control.h"
#include"../commonlib/json_util.h"
#include"../commonlib/utf_helper.h"
#include<iostream>
#include<fstream>
using namespace PROJECT_NAME;
using namespace control;
/*
namespace PROJECT_NAME{
    void to_json(JSON& to,const std::filesystem::perms& permission){
        using perms=std::filesystem::perms;
        unsigned int flag=(unsigned int)permission;
        auto others=flag&(unsigned int)perms::others_all;
        auto group=flag&(unsigned int)perms::group_all;
        auto owner=flag&(unsigned int)perms::owner_all;
        group>>=3;
        owner>>=6;
        to["owner"]=owner;
        to["group"]=group;
        to["others"]=others;
    }

    void to_json(JSON& to,const std::filesystem::file_type& type){
        using filetype=std::filesystem::file_type;
        switch (type)
        {
        case filetype::block:
            to="block";
            break;
        case filetype::character:
            to="character";
            break;
        case filetype::fifo:
            to="fifo";
            break;
        case filetype::junction:
            to="junction";
            break;
        case filetype::none:
            to="none";
            break;
        case filetype::not_found:
            to="not_found";
            break;
        case filetype::regular:
            to="regular";
            break;
        case filetype::socket:
            to="socket";
            break;
        case filetype::symlink:
            to="symlink";
            break;
        default:
            to="unknown";
            break;
        }
    }

    void to_json(JSON& to,const std::filesystem::path& dir){
        std::filesystem::directory_iterator it(dir),end;
        std::error_code code;
        to="{}"_json;
        for(;it!=end&&!code;it.increment(code)){
            auto& file=*it;
            auto name=file.path().filename().u8string();;
            auto& ref=to[name];
            if(file.is_directory()){
                ref=file;
            }
            else{
                ref["filetype"]=file.status().type();
                ref["size"]=file.file_size();
                ref["permissins"]=file.status().permissions();
            }
        }
        if(code){
            to=JSON();
        }
    }

}*/



int STDCALL hardtest(){
    const char* code=
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
    return num<=1?num:$$(num-1)+$$(num-2)?true:false;
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
    auto json=JSON(globalvec);
    std::ofstream fs("dump.js");
    fs << "const obj=";
    fs << json.to_string(0,JSONFormat::noendline);
    fs << ";\n";
    fs << "console.log(obj);";
    std::ofstream("dump.json") << json.to_string(4);
    const char* s=u8"ユー⑧";
    Reader<std::string> utf8(s);
    std::u32string str;
    int errpos=0;
    utf8.readwhile(str,utf8toutf32,errpos);
    return 0;
}
