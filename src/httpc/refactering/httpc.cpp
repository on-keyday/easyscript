#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#endif
#include"httpc.h"
#include"../http1.h"
#ifdef _WIN32
#include<windows.h>
#include<direct.h>
#else
#include<unistd.h>
#endif
#include<cmdline_ctx.h>
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

enum class LoggerMode{
    std_out=0x1,
    std_err=0x2,
    both=std_out|std_err,
};
DEFINE_ENUMOP(LoggerMode)

struct Logger{
    LoggerMode mode;

    Logger(LoggerMode mode){
        set_mode(mode);
    }

    void set_mode(LoggerMode in){
        mode=in;
    }

    template<class T>
    Logger& operator<<(const T& t){
        if(any(mode&LoggerMode::std_out)){
            std::cout << t;
        }
        if(any(mode&LoggerMode::std_err)){
            std::cerr << t;
        }
        return *this;
    }
};

auto logger=Logger(LoggerMode::std_out);
auto errorlog=Logger(LoggerMode::std_err);

template<class Buf,class Func>
int getting_method(const Arg<Buf>& arg,Func func){
    Context* ctx=arg.get_user(true);
    string url;
    if(!arg.get_arg(0,url)){
        arg.log(errorlog,"error:url required");
        return -1;
    }
    ctx->url=url;
    if(!func(ctx->client,url.c_str()))return -2;
    return 0;
}

template<class Buf>
int get(const Arg<Buf>& arg,int,int){
    return getting_method<Buf>(arg,[](HTTPClient* client,auto url){return client->get(url);});
}

template<class Buf>
int head(const Arg<Buf>& arg,int,int){
    return getting_method<Buf>(arg,[](HTTPClient* client,auto url){return client->head(url);});
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
    if(must&&!payload.size()){
        arg.log(errorlog,"error:0 size payload not allowed");
        return -3;
    }
    if(arg.exists_option("-payload-file")){
        string file;
        StrStream(payload)>>u16filter>>utffilter>>file;
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
    if(!arg.exists_option("-show-all-header")){
        msg("server");
        msg("content-type");
        msg("accept");
        msg("location");
    }
    else{
        for(auto& h:ctx->client->header()){
            arg.log(logger,h.first,":",h.second);
        }
    }
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
    return 1;
}

template<class Buf>
int cd(const Arg<Buf>& arg,int,int){
    string cwd;
    if(!arg.get_arg(0,cwd)){
        arg.log(errorlog,"error:one argument required");
        return -1;
    }
#ifdef _WIN32
    std::wstring tmp;
    StrStream(cwd)>>u16filter>>tmp;
    if(_wchdir(tmp.c_str())){
        std::string show;
        StrStream(tmp)>>utffilter>>show;
        arg.log(errorlog,"error: '",show,"':no such directory");
        return -2;
    }
#else
    if(chdir(cwd.c_str())){
        arg.log(errorlog,"error: '",cwd,"':no such directory");
        return -2;
    }
#endif
    return 1;
}

template<class Buf>
int echo(const Arg<Buf>& arg,int,int){
    for(auto& a:arg){
#ifdef _WIN32
        std::string e;
        StrStream(a)>>u16filter>>utffilter>> e;
        logger << e;
#else
        logger << a;
#endif
        logger << " ";
    }
    logger << "\n";
    return 1;
}

template<class Buf>
int hello(const Arg<Buf>& arg,int,int){
    logger << "Hello User!\n";
    logger << "I'm 'httpc', a HTTP(S) client software!\n";
    logger << "My job is the agent on the Internet of YOU!\n";
    logger << "Best Regards!\n";
    return 1;
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
    cmdline->register_subcommand(cmd,"cd",cd<Buf>,"",CmdFlag::no_option);
    cmdline->register_subcommand(cmd,"echo",echo<Buf>,"",CmdFlag::no_option);
    cmdline->register_subcommand(cmd,"hello",hello<Buf>,"",CmdFlag::no_option);
    int res=0;
    while(!ctx->exit){
        char* cd=_getcwd(NULL,0);
        if(cd)logger << cd;
        logger << ">>";
        res=cmdline->execute_with_context(cmd,[](auto& in){
            std::string ws;
            std::getline(std::cin,ws);
#ifdef _WIN32
            in=Reader<Buf>((StrStream(ws)>>nativefilter).ref());
#else
            in=Reader<Buf>(std::move(ws));
#endif
            return true;
        },LogFlag::none);
        free(cd);
    }
    logger << "Bye\n";
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
    if(arg.exists_option("-auto-redirect")){
        ctx->client->set_auto_redirect(true);
    }
    else{
        ctx->client->set_auto_redirect(false);
    }
    
    return 0;
}

void simple_err(ErrorKind kind,const string&,const char* err1,const string& err2,const char* err3){
    if(kind==ErrorKind::input)return;
    errorlog << err1 << err2 << err3 << "\n";
}

template<class Buf>
OptCbFlag logger_option(Reader<Buf>& r,string& arg,
const typename CmdLine<Buf>::option_maptype&,typename CmdLine<Buf>::arg_option_maptype&){
    if(arg=="-out-both-logger"){
        logger.set_mode(LoggerMode::both);
        errorlog.set_mode(LoggerMode::both);
        return OptCbFlag::succeed;
    }
    return OptCbFlag::redirect;
}

template<class Buf>
Cmd<Buf>* SetCommand(CmdLine<Buf>& ctx,bool set_logout=false){
    ctx.register_error(simple_err);
    auto httpc=ctx.register_command("httpc",http<Buf>,"",CmdFlag::ignore_invalid_option,common_option_get<Buf>);
    constexpr auto optf=OptFlag::only_one|OptFlag::effect_to_parent;
    ctx.register_by_optname(httpc,{
        {"-output-file","o",true,optf,"set output file"},
        {"-summary","s",false,optf,"summarize http header"},
        {"-interactive","i",false,OptFlag::only_one|OptFlag::hidden_from_child,"interactive mode"},
        {"-cacert-file","c",true,optf,"set cacert file"},
        {"-data-payload","d",true,optf,"set datapayload"},
        {"-payload-file","f",false,optf,"payload is file flag"},
        {"-nobody-show","n",false,optf,"not print body"},
        {"-custom-header","h",true,OptFlag::none,"set custom header"},
        {"-auto-redirect","a",false,optf,"set auto redirect"},
        {"-show-all-header","S",false,optf,"show all header"}
    });
    constexpr auto flag=CmdFlag::invoke_parent|CmdFlag::use_parentopt|CmdFlag::use_parentcb|
    CmdFlag::take_over_prev_run|CmdFlag::ignore_invalid_option;
    ctx.register_subcommand(httpc,"get",get<Buf>,"GET method",flag);
    ctx.register_subcommand(httpc,"head",head<Buf>,"HEAD method",flag);
    ctx.register_subcommand(httpc,"post",post<Buf>,"POST method",flag);
    ctx.register_subcommand(httpc,"put",put<Buf>,"PUT method",flag);
    ctx.register_subcommand(httpc,"patch",patch<Buf>,"PATCH method",flag);
    ctx.register_subcommand(httpc,"delete",_delete<Buf>,"DELETE method",flag);
    if(set_logout){
        ctx.register_callbacks(httpc,nullptr,logger_option,nullptr);
    }
    return httpc;
}

template<class Buf>
int invoke_interactive(const Arg<Buf>& arg,Context* ctx){
#ifdef _WIN32
    CmdLine<ToUTF8<std::wstring>> ws;
#else
    CmdLine<std::string> ws;
#endif
    ws.register_paramproc(defcmdlineproc_impl);
    Context& user=ws.get_user();
    user.client=ctx->client;
    user.dummy=false;
    user.client->set_requestadder(custom_header,&user);
    auto httpc=SetCommand(ws);
    return ws.execute_with_context(httpc,[](auto& in){
#ifdef _WIN32
        in.ref()=std::wstring(L"-i");
#else
        in.ref()=std::string("-i");
#endif
        return true;
    });
}

#ifdef _WIN32
template<>
int invoke_interactive<ToUTF8<std::wstring>>(const Arg<ToUTF8<std::wstring>>& arg,Context* ctx){
    return interactive<ToUTF8<std::wstring>>(arg,ctx);
}
#else
template<>
int invoke_interactive<std::string>(const Arg<std::string>& arg,Context* ctx){
    return interactive<std::string>(arg,ctx);
}
#endif

template<class Buf>
int http(const Arg<Buf>& arg,int prev,int pos){
    Context* ctx=arg.get_user(true);
    if(pos==0){
        if(!ctx->on_interactive&&arg.exists_option("-interactive")){
            return invoke_interactive<Buf>(arg,ctx);
        }
        if(ctx->on_interactive){
            std::string ws;
            if(arg.get_arg(0,ws)){
#ifdef _WIN32
                arg.log(errorlog,"unknown command:",(StrStream(ws)>>u16filter>>utffilter).ref());
#else  
                arg.log(errorlog,"unknown command:",ws.c_str());
#endif
            }
            else{
                arg.log(errorlog,"input method!");
            }
            return -1;
        }
        else{
            prev=get<Buf>(arg,0,0);
        }
    }
    if(ctx->exit)return prev;
    if(prev==-2){
        arg.log(logger,"error:",ctx->client->err().c_str());
        return -2;
    }
    if(prev){
        return prev;
    }
    bool anyshown=false;
    if(!arg.exists_option("-nobody-show",true)&&ctx->client->body().size()){
        logger << ctx->client->body() << "\n";
        anyshown=true;
    }
    string opt;
    if(arg.exists_option("-summary",true)){
        PrintSummay<Buf>(arg,ctx);
        anyshown=true;
    }
    if(arg.get_optarg(opt,"-output-file",0,true)){
#ifdef _WIN32
        std::wstring path;
        StrStream(opt) >> u16filter >> path;
#else
        std::string path=std::move(opt);
#endif
        std::ofstream ofs(path.c_str(),std::ios_base::binary);
        if(ofs.is_open()){
            ofs<<ctx->client->body();
            arg.log(logger,"content was saved to ",opt.c_str());
        }
        else{
            arg.log(logger,"warning:file '",opt.c_str(),"' not opened");
        }
        anyshown=true;
    }
    if(!anyshown){
        arg.log(logger,"operation succeeded");
    }
    if(ctx->client->statuscode()>=200&&ctx->client->statuscode()<300)return 0;
    return ctx->client->statuscode();
}

template<class Buf>
int command_impl(Buf&& input){
    HTTPClient client;
    CmdLine<Buf> ctx;
    ctx.register_paramproc(defcmdlineproc_impl);
    Context& user=ctx.get_user();
    user.client=&client;
    user.client->set_requestadder(custom_header,&user);
    auto httpc=SetCommand(ctx,true);
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
    auto httpc=SetCommand(ctx,true);
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

int command(const char* input){
    return command_impl<std::string>(std::string(input));
}

int command_wchar(const wchar_t* input){
    return command_impl<ToUTF8<std::wstring>>(std::wstring(input));
}

int command_argv(int argc,char** argv){
    return command_impl<ArgVWrapper<char>>(ArgVWrapper<char>(argc,argv));
}