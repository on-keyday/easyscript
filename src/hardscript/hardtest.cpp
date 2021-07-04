/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#define DLL_EXPORT __declspec(dllexport)
#include"hardtest.h"
#include"parser/control.h"
#include<json_util.h>
#include<basic_helper.h>
#include<fileio.h>
#include"parser/node.h"
#include<iostream>
#include<fstream>
#include<iterator>
#include<memory>
#include<assert.h>

using namespace PROJECT_NAME;
using namespace node;
using namespace control;

struct Input{
private:
    std::string buffer;
    bool exited=false;
public:
    bool noread=false;
    char operator[](size_t pos) const{
        if(exited)return char();
        return buffer[pos];
    }

    char operator[](size_t pos){
        size_t line=0;
        while(!noread&&!exited&&pos>=buffer.size()-line){
            std::cout << ">>>";
            std::string tmp;
            std::getline(std::cin,tmp);
            if(tmp=="exit"){
                exited=true;
            }
            else{
                buffer+=tmp+"\n";
                line++;
            }
        }
        if(exited)return char();
        return buffer[pos];
    }

    bool clear(){
        buffer="";
        noread=false;
        return true;
    }

    size_t size()const{
        return exited?0:noread?buffer.size():buffer.size()+1;
    }

    bool exit(){
        return exited;
    }
};

void interpret1(){
    std::string filename;
    while(true){
        Reader<std::string> cmdline(ignore_space);
        std::cout << ">>>";
        std::getline(std::cin,cmdline.ref());
        std::cin.clear();
        if(cmdline.expect("exit",is_c_id_usable)){
            break;
        }
        else if(cmdline.expect("clear",is_c_id_usable)){
            ::system("cls");
        }
        else if(cmdline.expect("file",is_c_id_usable)){
            std::string tmp;
            int res;
            cmdline.readwhile(tmp,cmdline_read,&res);
            if(res==0){
                std::cout << "invalid argument\n";
                continue;
            }
            else if(res<0){
                tmp.erase(0,1);
                tmp.pop_back();
            }
            filename=tmp;
            std::cout << "file " << filename << " set\n";
        }
        else if(cmdline.expect("run")){
            std::ifstream input(filename);
            if(!input.is_open()){
                std::cout << "file " << filename << " not opened\n";
            }
            else{
                Reader<std::string> read(ignore_c_comments);
                read.ref()=std::string(std::istreambuf_iterator<char>(input),
                                       std::istreambuf_iterator<char>());
                                       input.close();
                NodesT globalvec;
                auto t=parse_all(read,globalvec);
                if(t){
                    std::cout << "valid\n";
                    std::cout << JSON(globalvec).to_string();
                }
                else{
                    LinePosContext ctx;
                    read.readwhile(linepos,ctx);
                    std::cout << "invalid at (" << ctx.line+1 << "," << ctx.pos+1 << ")\n";
                }
            }
        }
        else{
            std::cout << "unknown command:" << cmdline.ref() << "\n";
        }
        
    }
}

int STDCALL hardtest(){
    auto json=R"(
        {
            "kindnum":1,
            "symbol":"if",
            "cond":{
                "kindnum":0,
                "symbol":"true"
            },
            "blockpos":0,
            "block":[
                {
                    "kindnum":1,
                    "symbol":"calcable"
                }
            ]
        }
    )"_json;

    FileInput fp(R"(D:\CommonLib\CommonLib2\garbage\test.json)");
    Reader<ThreadSafe<FileInput&>> ps(fp);
    auto& f=ps.ref().get();
    
    ps.ref().release(f);
    json["test"].parse_assign(ps);
    
    return 0;
}
