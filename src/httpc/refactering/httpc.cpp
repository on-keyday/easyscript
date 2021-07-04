#define DLL_EXPORT __declspec(dllexport)
#include"httpc.h"
#include"../http1.h"
#include<cmdline_ctx.h>
#include<windows.h>
#include<iostream>
#include<vector>
#include<map>
#include<extension_operator.h>
#include<fstream>
using namespace PROJECT_NAME;

struct Context{
    HTTPClient* client;
    std::string url;
    bool exit=false;
    bool on_interactive=false;
};

template<class Buf,class Str=std::string>
using CmdLine=CmdlineCtx<Buf,Str,std::map,std::vector,Context>;

using string=std::string;

template<class Buf>
using Arg=typename CmdLine<Buf>::arg_type;

template<class C>
using Argv=typename CmdLine<ArgVWrapper<C>>::arg_type;

template<class Buf>
using Cmd=typename CmdLine<Buf>::command_type;

auto& logger=std::cout;
auto& errorlog=std::cerr;

template<class Buf>
int get(const Arg<Buf>& arg,int,int){
    Context* ctx=arg.get_user(true);
    string url;
    if(!arg.get_arg(0,url)){
        arg.log(errorlog,"url required");
    }
    ctx->url=url;
    if(!ctx->client->get(url.c_str()))return -2;
    return 0;
}

template<class Buf>
void PrintSummay(const Arg<Buf>& arg,Context* ctx){
    auto msg=[&](const char* header){
        if(auto& head=ctx->client->header();head.count(header)){
            auto it=head.equal_range(header);
            for(auto i=it.first;i!=it.second;++i){
                arg.log(logger,header," is ",(*i).second);
            }
        }
    };
    arg.log(logger,"url:",ctx->url);
    arg.log(logger,"accessed address:",ctx->client->ipaddress());
    arg.log(logger,"status code:",ctx->client->statuscode());
    arg.log(logger,"reason phrase:",ctx->client->reasonphrase());
    arg.log(logger,"time:",
    std::chrono::duration_cast<std::chrono::milliseconds>(ctx->client->time()).count(),"ms");
    msg("server");
    msg("content-type");
    msg("accept");
}

template<class Buf>
int exit(const Arg<Buf>& arg,int prev,int){
    arg.get_user(true)->exit=true;
    string opt;
    if(arg.get_arg(0,opt)){
        Reader<string>(std::move(opt))>>prev;
    }
    return prev;
}

template<class Buf>
int interactive(const Arg<Buf>& arg,Context* ctx){
    ctx->on_interactive=true;
    auto cmdline=arg.get_cmdline();
    cmdline->register_subcommand(arg.get_this_command(),"exit",exit<Buf>);
    while(!ctx->exit){
        logger << ">> ";
        cmdline->execute_with_context(arg.get_this_command(),[](auto& in){
            std::wstring ws;
            std::getline(std::wcin,ws);
            in=Reader<Buf>(std::move(ws));
            return true;
        },LogFlag::none);
    }
    arg.get_cmdline()->remove_command("exit");
    ctx->on_interactive=false;
    return 0;
}

template<class Buf>
int http(const Arg<Buf>& arg,int prev,int pos);

template<class Buf>
Cmd<Buf>* SetCommand(CmdLine<Buf>& ctx){
    auto httpc=ctx.register_command("httpc",http<Buf>);
    ctx.register_by_optname(httpc,{
        {"-output-file","o",true,OptFlag::only_one,"set output file"},
        {"-summary","s",false,OptFlag::only_one,"summarize http header"},
        {"-interactive","i",false,OptFlag::only_one&OptFlag::hidden_from_child,"interactive mode"},
        {"-cacert-file","c",true,OptFlag::only_one,"set cacert file"}
    });
    auto got=ctx.register_subcommand(httpc,"get",get<Buf>,"GET method",CmdFlag::invoke_parent|CmdFlag::use_parentopt);
    return httpc;
}

template<class Buf>
bool invoke_interactive(const Arg<Buf>& arg,Context* ctx){
    CmdLine<ToUTF8<std::wstring>> ws;
    ws.get_user().client=ctx->client;
    auto httpc=SetCommand(ws);
    return ws.execute_with_context(httpc,[](auto& in){
        in.ref()=std::wstring(L"-i");
        return true;
    });
}

template<>
bool invoke_interactive<std::wstring>(const Arg<std::wstring>& arg,Context* ctx){
    return interactive<std::wstring>(arg,ctx);
}

template<class Buf>
int http(const Arg<Buf>& arg,int prev,int pos){
    Context* ctx=arg.get_user(true);
    if(pos==0){
        if(!ctx->on_interactive&&arg.exists_option("-interactive")){
            return invoke_interactive<Buf>(arg,ctx);
        }
        arg.log(errorlog,"input method!");
    }
    if(prev==-2){
        arg.log(logger,"error:",ctx->client->err());
        return -1;
    }
    string opt;
    if(arg.exists_option("-summary")){
        PrintSummay<Buf>(arg,ctx);
    }
    if(arg.get_optarg(opt,"-output-file",0)){
        std::wstring path;
        StrStream(opt) >> u16filter >> path;
        std::ofstream(path)<<ctx->client->body();
        arg.log(logger,"content was saved to ",opt);
    }
    return 0;
}

template<class Buf>
int command_impl(Buf&& input){
    HTTPClient client;
    CmdLine<Buf> ctx;
    ctx.get_user().client=&client;
    auto httpc=SetCommand(ctx);
    return ctx.execute_with_context(httpc,[&](auto& r){
        r=std::move(input);
        return true;
    });
}

template<class C>
int argv_command(ArgVWrapper<C>&& input){
    HTTPClient client;
    CmdLine<ArgVWrapper<C>> ctx;
    ctx.get_user().client=&client;
    auto httpc=SetCommand(ctx);
    return ctx.execute_with_context(httpc,[&](auto& r){
        r.ref()=std::move(input);
        r.increment();
        return true;
    });
}

template<>
int command_impl(ArgVWrapper<char>&& input){
   return argv_command(std::forward<ArgVWrapper<char>>(input));
}

/*
template<>
int command_impl(ArgVWrapper<wchar_t>&& input){
   return argv_command(std::forward<ArgVWrapper<wchar_t>>(input));
}*/

int command(const char* input){
    return command_impl<std::string>(std::string(input));
}

int command_wchar(const wchar_t* input){
    return command_impl<ToUTF8<std::wstring>>(std::wstring(input));
}

int command_argv(int argc,char** argv){
    return command_impl<ArgVWrapper<char>>(ArgVWrapper<char>(argc,argv));
}