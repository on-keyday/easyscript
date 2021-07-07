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
#include<fileio.h>
using namespace PROJECT_NAME;


struct Context{
    HTTPClient* client;
    std::string url;
    bool exit=false;
    bool on_interactive=false;
    bool dummy=true;
    std::multimap<std::string,std::string> custom_header;
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
        arg.log(errorlog,"error:url required");
        return -1;
    }
    ctx->url=url;
    if(!ctx->client->get(url.c_str()))return -2;
    return 0;
}

template<class Buf,class Func>
int sending_method(const Arg<Buf>& arg,Func func,bool must){
    Context* ctx=arg.get_user(true);
    string url,payload;
    if(!arg.get_arg(0,url)){
        arg.log(errorlog,"error:url required");
        return -1;
    }
    ctx->url=url;
    if(!arg.get_optarg(payload,"-data-payload",0)&&must){
        arg.log(errorlog,"error:set data payload by option --data-payload or -d");
        return -3;
    }
    if(payload.size()&&arg.exists_option("-payload-file")){
        auto file=(StrStream(payload)>>u16filter>>utffilter).ref();
        Reader<FileMap> r(file.c_str());
        if(!r.ref().size()){
            arg.log(errorlog,"error:file '",file,"' is not opend");
            return -4;
        }
        if(!func(ctx->client,url.c_str(),r.ref().c_str(),r.ref().size()))return -2;
    }
    else{
        if(!func(ctx->client,url.c_str(),payload.c_str(),payload.size()))return -2;
    }
    return 0;
}

template<class Buf>
int post(const Arg<Buf>& arg,int,int){
    return sending_method<Buf>(arg,[](HTTPClient* client,auto url,auto payload,auto size){
        return client->post(url,payload,size);
    },true);
}

template<class Buf>
int put(const Arg<Buf>& arg,int,int){
    return sending_method<Buf>(arg,[](HTTPClient* client,auto url,auto payload,auto size){
        return client->put(url,payload,size);
    },true);
}

template<class Buf>
int patch(const Arg<Buf>& arg,int,int){
    return sending_method<Buf>(arg,[](HTTPClient* client,auto url,auto payload,auto size){
        return client->patch(url,payload,size);
    },true);
}

template<class Buf>
int _delete(const Arg<Buf>& arg,int,int){
    return sending_method<Buf>(arg,[](HTTPClient* client,auto url,auto payload,auto size){
        return client->_delete(url,payload,size);
    },true);
}

template<class Buf>
void PrintSummay(const Arg<Buf>& arg,Context* ctx){
    auto msg=[&](const char* header){
        if(auto& head=ctx->client->header();head.count(header)){
            auto it=head.equal_range(header);
            for(auto i=it.first;i!=it.second;++i){
                arg.log(logger,header,":",(*i).second.c_str());
            }
        }
    };
    arg.log(logger,"url:",ctx->url);
    arg.log(logger,"accessed address:",ctx->client->ipaddress());
    arg.log(logger,"status code:",ctx->client->statuscode());
    arg.log(logger,"reason phrase:",ctx->client->reasonphrase());
    arg.log(logger,"time:",
    std::chrono::duration_cast<std::chrono::milliseconds>(ctx->client->time()).count(),"ms");
    if(auto size=ctx->client->body().size()){
        arg.log(logger,"content-length:",size);
    }
    else{
        arg.log(logger,"no content");
    }
    msg("server");
    msg("content-type");
    msg("accept");
}

template<class Buf>
int exit(const Arg<Buf>& arg,int prev,int){
    arg.get_user(true)->exit=true;
    string opt;
    prev=0;
    if(arg.get_arg(0,opt)){
        Reader<string>(std::move(opt))>>prev;
    }
    return prev;
}

template<class Buf>
int clear(const Arg<Buf>& arg,int,int){
    ::system("cls");
    return 0;
}

template<class Buf>
int interactive(const Arg<Buf>& arg,Context* ctx){
    ctx->on_interactive=true;
    CmdLine<Buf>* cmdline=arg.get_cmdline();
    auto httpc=cmdline->get_command("httpc");
    cmdline->remove_option(httpc,"-interactive");
    cmdline->remove_option(httpc,"i");
    auto cmd=arg.get_this_command();
    cmdline->register_subcommand(cmd,"exit",exit<Buf>,"",CmdFlag::no_option);
    cmdline->register_subcommand(cmd,"quit",exit<Buf>,"",CmdFlag::no_option);
    cmdline->register_subcommand(cmd,"clear",clear<Buf>,"",CmdFlag::no_option);
    int res=0;
    while(!ctx->exit){
        logger << "command?>>";
        res=cmdline->execute_with_context(cmd,[](auto& in){
            std::string ws;
            std::getline(std::cin,ws);
            std::wstring tmp;
            in=Reader<Buf>((StrStream(ws)>>nativefilter).ref());
            return true;
        },LogFlag::none);
    }
    return res;
}

template<class Buf>
int http(const Arg<Buf>& arg,int prev,int pos);


bool custom_header(void* pctx,string& add,const HTTPClient* client){
    Context* ctx=(Context*)pctx;
    if(ctx->dummy){
        add.append("content-type: text/html\r\n");
        ctx->dummy=false;
        return true;
    }
    for(auto& h:ctx->custom_header){
        add.append(h.first);
        add.append(": ");
        add.append(h.second);
        add.append("\r\n");
    }
    return true;
}

template<class Buf>
int common_option_get(const Arg<Buf>& arg,int prev,int pos){
    Context* ctx=arg.get_user(true);
    ctx->custom_header.clear();
    if(arg.exists_option("-custom-header",true)){
        for(auto&& header:arg.get_alloptarg("-custom-header")){
            Reader<Refer<string>> parse(header);
            string name,value;
            parse >> igws >> +[](char c){return is_alpha_or_num(c)||c=='-';} >> name;
            if(!parse.expect(":")){
                arg.log(logger,"warning:ignore param '",header,"'");
                continue;
            }
            parse >> +[](char c){return c>=0&&c>=0x20;} >> value;
            if(!parse.ceof()){
                arg.log(logger,"warning:ignore param '",header,"'");
                continue;
            }
            ctx->custom_header.emplace(name,value);
            header="";
        }
    }
    string cacert;
    if(arg.get_optarg(cacert,"-cacert-file",0,true)){
        ctx->client->set_cacert(cacert);
    }
    /*else{
        ctx->client->set_cacert("./cacert.pem");
    }*/
    return 0;
}

template<class Buf>
Cmd<Buf>* SetCommand(CmdLine<Buf>& ctx){
    auto httpc=ctx.register_command("httpc",http<Buf>,"",common_option_get<Buf>);
    constexpr OptFlag optf=OptFlag::only_one|OptFlag::effect_to_parent;
    ctx.register_by_optname(httpc,{
        {"-output-file","o",true,optf,"set output file"},
        {"-summary","s",false,optf,"summarize http header"},
        {"-interactive","i",false,OptFlag::only_one|OptFlag::hidden_from_child,"interactive mode"},
        {"-cacert-file","c",true,optf,"set cacert file"},
        {"-data-payload","d",true,optf,"set datapayload"},
        {"-payload-file","f",false,optf,"payload is file flag"},
        {"-nobody-show","n",false,optf,"not print body"},
        {"-custom-header","h",true,OptFlag::none,"set custom header"},
    });
    auto flag=CmdFlag::invoke_parent|CmdFlag::use_parentopt|CmdFlag::take_over_prev_run;
    ctx.register_subcommand(httpc,"get",get<Buf>,"GET method",flag);
    ctx.register_subcommand(httpc,"post",post<Buf>,"POST method",flag);
    ctx.register_subcommand(httpc,"put",put<Buf>,"PUT method",flag);
    ctx.register_subcommand(httpc,"patch",patch<Buf>,"PATCH method",flag);
    ctx.register_subcommand(httpc,"delete",_delete<Buf>,"DELETE method",flag);
    return httpc;
}

template<class Buf>
int invoke_interactive(const Arg<Buf>& arg,Context* ctx){
    CmdLine<ToUTF8<std::wstring>> ws;
    ws.register_paramproc(defcmdlineproc_impl);
    Context& user=ws.get_user();
    user.client=ctx->client;
    user.dummy=false;
    user.client->set_requestadder(custom_header,&user);
    auto httpc=SetCommand(ws);
    return ws.execute_with_context(httpc,[](auto& in){
        in.ref()=std::wstring(L"-i");
        return true;
    });
}

template<>
int invoke_interactive<ToUTF8<std::wstring>>(const Arg<ToUTF8<std::wstring>>& arg,Context* ctx){
    return interactive<ToUTF8<std::wstring>>(arg,ctx);
}

template<class Buf>
int http(const Arg<Buf>& arg,int prev,int pos){
    Context* ctx=arg.get_user(true);
    if(pos==0){
        if(!ctx->on_interactive&&arg.exists_option("-interactive")){
            return invoke_interactive<Buf>(arg,ctx);
        }
        std::string ws;
        if(arg.get_arg(0,ws)){
            arg.log(errorlog,"unknown command:",(StrStream(ws)>>u16filter>>utffilter).ref());
        }
        else{
            arg.log(errorlog,"input method!");
        }
        return -1;
    }
    if(ctx->exit)return prev;
    if(prev==-2){
        arg.log(logger,"error:",ctx->client->err().c_str());
        return -2;
    }
    if(prev<0){
        return prev;
    }
    if(!arg.exists_option("-nobody-show",true)){
        logger << ctx->client->body();
    }
    string opt;
    if(arg.exists_option("-summary",true)){
        PrintSummay<Buf>(arg,ctx);
    }
    if(arg.get_optarg(opt,"-output-file",0,true)){
        std::wstring path;
        StrStream(opt) >> u16filter >> path;
        std::ofstream(path)<<ctx->client->body();
        arg.log(logger,"content was saved to ",opt.c_str());
    }
    return 0;
}

template<class Buf>
int command_impl(Buf&& input){
    HTTPClient client;
    CmdLine<Buf> ctx;
    ctx.register_paramproc(defcmdlineproc_impl);
    Context& user=ctx.get_user();
    user.client=&client;
    user.client->set_requestadder(custom_header,&user);
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
    //ctx.register_paramproc(defcmdlineproc_impl);
    Context& user=ctx.get_user();
    user.client=&client;
    user.client->set_requestadder(custom_header,&user);
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
void command_common_init(){
    
}

int command(const char* input){
    command_common_init();
    return command_impl<std::string>(std::string(input));
}

int command_wchar(const wchar_t* input){
    command_common_init();
    return command_impl<ToUTF8<std::wstring>>(std::wstring(input));
}

int command_argv(int argc,char** argv){
    command_common_init();
    return command_impl<ArgVWrapper<char>>(ArgVWrapper<char>(argc,argv));
}