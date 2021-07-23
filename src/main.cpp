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
#include"bunkai/ast.h"
#include<fileio.h>
#include<charconv>
#include<json_util.h>
#include"bunkai/eval.h"
#include<utf_helper.h>
#include<utfreader.h>
#include<extension_operator.h>
#include<set>
#include<extutil.h>
#include<learnstd.h>

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

void test2(){
    JSON js;
    Reader<FileMap> num(R"(c:/users/ukfco/downloads/canada.json)");
    std::cout << num.ref().size() << "\n";
    auto start=std::chrono::system_clock::now();
    js.parse_assign(num);
    auto end=std::chrono::system_clock::now();
    long v=0;
    auto begin=std::chrono::system_clock::now();
    auto jstr=js.to_string();
    auto fin=std::chrono::system_clock::now();
    //std::cout << jstr;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(fin-begin).count() << "\n";
}
void ownermap(JSON& js,ast::SAstToken& tok);
void ownermap(JSON& js,ast::type::SObject& tok);

void ownermap(JSON& js,ast::type::SType& tok,bool nobase=true){
    if(tok->name!="")js["name"]=tok->name;
    js["user"]=tok.use_count();
    js["pointer"]=(uintptr_t)tok.get();
    if(tok->token){
        ownermap(js["token"],tok->token);
    }
    foreach_node(p,tok->param){
        JSON tmp;
        ownermap(tmp,p);
        js["param"].push_back(std::move(tmp));
    }
    if(tok->vecof){
        ownermap(js["vecof"],tok->vecof);
    }
    if(tok->ptrto){
        ownermap(js["ptrto"],tok->ptrto);
    }
    if(tok->refof){
        ownermap(js["refof"],tok->refof);
    }
    if(tok->tmpof){
        ownermap(js["tmpof"],tok->tmpof);
    }
    if(!nobase&&tok->base){
        ownermap(js["base"],tok->base,false);
    }
}

void ownermap(JSON& js,ast::type::SObject& tok){
    js["user"]=tok.use_count();
    js["pointer"]=(uintptr_t)tok.get();
    if(tok->init){
        ownermap(js["token"],tok->init);
    }
    if(tok->type){
        ownermap(js["type"],tok->type);
    }
}


void ownermap(JSON& js,ast::SAstToken& tok){
    js["user"]=tok.use_count();
    js["pointer"]=(uintptr_t)tok.get();
    if(tok->left)ownermap(js["left"],tok->left);
    if(tok->right)ownermap(js["right"],tok->right);
    if(tok->cond)ownermap(js["cond"],tok->cond);
    if(tok->block.node){
        size_t c=0;
        auto& ref=js["block"];
        foreach_node(p,tok->block){
            JSON tmp;
            ownermap(tmp,p);
            ref.push_back(std::move(tmp));
            c++;
        }
    }
    if(tok->type){
        ownermap(js["type"],tok->type,false);
    }
}

auto test1(){
    ast::type::TypePool pool;
    ast::AstReader r(FileMap(R"(../../../src/bunkai_src/define_asm.asd)"),pool);
    ast::SAstToken tok=nullptr;
    const char* err=nullptr;
    LinePosContext ctx;
    auto start=std::chrono::system_clock::now();
    auto f=r.parse(tok,&err);
    auto end=std::chrono::system_clock::now();
    r.linepos(ctx);
    if(f){
        /*ast::delete_token(tok);
        pool.delall();*/
    }
    //std::wcout<<r.bufctrl().c_str();
    //eval::Evaluator p(pool);
    JSON js;
    ownermap(js,tok);
    std::ofstream("ownermap.json") << js.to_string(4);
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
    return tok;
}

Reader<Order<FileMap,char16_t>> test3(){
    Reader<FileMap> r(FileMap("../../../src/bunkai_src/test.txt"));
    if(r.expect("\xFE\xFF")){
        Reader<Order<FileMap,char16_t>> read(std::move(r.ref()));
        read.seek(1);
        read.ref().set_reverse(true);
        return read;
    }
    else if(r.expect("\xFF\xFE")){
        Reader<Order<FileMap,char16_t>> read(std::move(r.ref()));
        read.seek(1);
        return read;
    }
    exit(0);
}

void test4(){
    Reader<ToUTF8<std::u32string>> str(std::u32string(U"𠮷!太郎"));
    str.expect(u8"𠮷");
    str.expect(u8"𠮷");
    str.seek(0);
    bool res=str.expect(u8"𠮷");
    bool res2=str.expect("!");
    str.decrement();
    bool res3=str.expect(u8"!太");
    str.seek(0);
    bool res4=str.expect(u8"𠮷");

    Reader<ToUTF16<std::u32string>> v(std::u32string(U"𠮷!太郎"));
    bool ch=v.expect(u"𠮷!太郎");
    v.seek(0);
    bool res5=v.expect(u"𠮷");
    bool res6=v.expect("!");
    v.decrement();
    bool res7=v.expect("!");
    v.seek(0);
    bool res8=v.expect(u"𠮷");   
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


    Reader<Reverse<ToUTF32<std::string>>> s=Reverse(ToUTF32(std::string((const char*)u8"adakaraしんぶんし")));
    auto exp=s.expect(U"しんぶんし");
    auto exp2= s.expect("arakada");
    std::cout << exp << exp2;
    return 0;
}

