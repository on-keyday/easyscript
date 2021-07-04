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
#include<charconv>
#include<json_util.h>
#include"bunkai/eval.h"
#include<utf_helper.h>
#include<utfreader.h>
#include<extension_operator.h>
#include<cmdline_ctx.h>
#include<set>

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
    //setlocale(LC_ALL,".UTF-8");
    //SetConsoleCP(CP_UTF8);
    /*Reader<std::string> op("84672937");
    int t=0;
    op >> t;
    std::u8string str,inter;
    std::string in;
    std::getline(std::cin,in);
    StrStream(in) >> nativefilter >> u8filter >> igws >> is_c_id_usable<char> >> str >> inter;
    */
    CmdlineCtx<std::string,std::string,std::map,std::vector> ctx;
    ctx.register_paramproc(defcmdlineproc_impl<std::string>);
    ctx.register_error(
        [](auto& name,auto in1,auto& in2,auto in3){std::cout << in1 << in2 << in3;});
    auto testcmd=ctx.register_command("test",[](const auto& ctx,int,int){
        ctx.log(std::cout,"test called");
        return 0;
    });
    auto forcmd=ctx.register_subcommand(testcmd,"for",/*[](const auto& ctx){
        ctx.printlog(std::cout,"test for called");
        ctx.printlog(std::cout,ctx.exists_option("-long"));
        return 0;
    }*/nullptr);
    ctx.register_by_optname(forcmd,
    {{"-long","l"}});
    ctx.execute([](auto& input){
        std::getline(std::cin,input.ref());
        return true;
    });
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

    //test1();
    //test2();
    //auto r=test3();
    //r.expect(u"下");
    test4();
    ast::check_assert();
    return 0;
}
