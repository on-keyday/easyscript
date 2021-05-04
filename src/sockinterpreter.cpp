#include"socket.h"
#include"sockinterpreter.h"
#include<iostream>
#include<direct.h>
using namespace PROJECT_NAME;

bool parse_cmdline(Reader<std::string>& cmdline,int must,bool escape){
    return must>0?false:true;
}

template<class First,class... Args>
bool parse_cmdline(Reader<std::string>& cmdline,int must,bool escape,First& first,Args&... args){
    first="";
    if(cmdline.ahead("\"")||cmdline.ahead("'")){
        if(!cmdline.string(first))return false;
        if(first=="")return must>0?false:true;
        first.pop_back();
        first.erase(0,1);
        if(escape){
            First tmp;
            bool esc=false;
            for(auto& i:first){
                if(esc){
                    esc=false;
                }
                else if(i=='\\'){
                    esc=true;
                    continue;
                }
                tmp+=i;
            }
            first=std::move(tmp);
        }
    }
    else{
        cmdline.readwhile(first,untilincondition,[](auto c){return c!=' '&&c!='\t';});
        if(first=="")return must>0?false:true;
    }
    return parse_cmdline(cmdline,must-1,escape,args...);
}

template<class First>
bool orlink(First& first){
    return false;
}

template<class First,class Second,class... Args>
bool orlink(First& first,Second& second,Args&... args){
    return first==second||orlink(first,args...);
}

enum class Status{
    succeed,
    argnumnotmatch,
    argconditionnotmatch,
    httperror,
    fileinopenerror,
    notsucceedstatus,
    fileoutopenerr,
    filewriteerror,
    optionnotfound,
    quit,
    cdchanged
};

struct flags{
    bool auto_redirect=false;
    bool responseheader=false;
    bool onlybody=false;
    bool filepayload=false;
    bool urlencoded=false;
    bool useinfocb=false;
    std::string cacert="./cacert.pem";
    std::string default_path="/";
    bool allcerr=false;
    bool interactive=false;
    bool saveonlysucceed=false;
};

#if !_WIN32||defined __GNUC__
#define fopen_s(pfp,filename,mode) (*pfp=fopen(filename,mode))
#endif

Status read_from_file(std::string& body,std::string& file){
    FILE* fp=nullptr;
    fopen_s(&fp,body.c_str(),"rb");
    if(!fp){
        file=body;
        return Status::fileinopenerror;
    }
    std::string tmp;
    while(true){
        int i=fgetc(fp);
        if(i==EOF)break;
        tmp.push_back(i);
    }
    body=std::move(tmp);
    return Status::succeed;
}

Status write_to_file(HTTPClient& client,std::string& file){
    if(client.body()=="")return Status::succeed;
    FILE* fp=nullptr;
    fopen_s(&fp,file.c_str(),"wb");
    if(!fp)return Status::fileoutopenerr;
    size_t size=0;
    size=fwrite(client.body().data(),1,client.body().size(),fp);
    fclose(fp);
    if(size!=client.body().size())return Status::filewriteerror;
    return Status::succeed;
}

bool is_success_status(unsigned short statucode){
    return statucode>=200&&statucode<300;
}

Status unsafe_method(std::string& name,Reader<std::string>& cmdline,HTTPClient& client,flags& flag,std::string& file){
    Reader<std::string> checker(name);
    checker.readwhile(untilincondition_nobuf,is_alphabet<char>);
    if(!checker.ceof())return Status::argconditionnotmatch;
    bool nobody=false;
    if(cmdline.expect("no-body",is_c_id_usable)){
        nobody=false;
    }
    std::string url,body;
    if(!parse_cmdline(cmdline,1,true,url,body))return Status::argnumnotmatch;
    if(!parse_cmdline(cmdline,0,!flag.filepayload,file))return Status::argconditionnotmatch;
    if(body!=""&&flag.filepayload){
        auto loadf=read_from_file(body,file);
        if(loadf!=Status::succeed)return loadf;
    }
    if(!client.method(name.c_str(),url.c_str(),body.c_str(),body.size(),nobody))return Status::httperror;
    if(file!=""){
        if(flag.saveonlysucceed&&!is_success_status(client.statuscode()))return Status::notsucceedstatus;
        return write_to_file(client,file);
    }
    return Status::succeed;
}

Status httpmethod(Reader<std::string>& cmdline,HTTPClient& client,flags& flag,std::string& file){
    std::string method,url,body;
    if(!parse_cmdline(cmdline,2,true,method,url))return Status::argnumnotmatch;
    if(method=="method")return unsafe_method(url,cmdline,client,flag,file);
    auto meteq=[&method](auto... args){
        return orlink(method,args...);
    }; 
    std::transform(method.begin(),method.end(),method.begin(),
    [](auto c){return std::tolower((unsigned char)c);});
    if(meteq("post","put","patch")){
        if(!parse_cmdline(cmdline,1,true,body))return Status::argnumnotmatch;
    }
    else if(meteq("delete")){
        if(!parse_cmdline(cmdline,0,true,body))return Status::argconditionnotmatch;
    }
    if(!parse_cmdline(cmdline,0,!flag.filepayload,file))return Status::argnumnotmatch;
    if(body!=""&&flag.filepayload){
        auto loadf=read_from_file(body,file);
        if(loadf!=Status::succeed)return loadf;
    }
    if(!client.method_if_exist(method.c_str(),url.c_str(),body.c_str(),body.size()))return Status::httperror;
    if(file!=""){
        if(flag.saveonlysucceed&&!is_success_status(client.statuscode()))return Status::notsucceedstatus;
        return write_to_file(client,file);
    }
    return Status::succeed;
}

template<class Stream>
void print(Stream&){}

template<class Stream,class First,class... Args>
void print(Stream& st,First& first,Args... args){
    st << first;
    print(st,args...);
}

bool cerrf=false;

Status set_options(const std::string& opt,Reader<std::string>& cmdline,flags& flag,bool enable){
    auto print_hook=[&flag](auto... args){
        print(flag.allcerr?std::cerr:std::cout,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    if(opt=="")return Status::argnumnotmatch;
    if(opt=="all"){
        return set_options("hafuei",cmdline,flag,enable);
    }
    if(opt=="header"){
        flag.responseheader=enable;
    }
    else if(opt=="body"){
        flag.onlybody=enable;
    }
    else if(opt=="url-encoded"){
        flag.urlencoded=enable;
    }
    else if(opt=="file-payload"){
        flag.filepayload=enable;
    }
    else if(opt=="auto-redirect"){
        flag.auto_redirect=enable;
    }
    else if(opt=="interactive"){
        flag.interactive=enable;
    }
    else if(opt=="cacert-file"||opt=="cafile"){
        if(!parse_cmdline(cmdline,1,false,flag.cacert)){
            print_hookln("option: one argument required");
            return Status::argnumnotmatch;
        }
    }
    else if(opt=="default-path"){
        if(!parse_cmdline(cmdline,1,false,flag.default_path)){
            print_hookln("option: one argument required");
            return Status::argnumnotmatch;
        }
    }
    else if(opt=="use-infocb"){
        flag.useinfocb=enable;
    }
    else if(opt=="stderr"){
        flag.allcerr=enable;
        cerrf=enable;
    }
    else{
        for(auto c:opt){
            if(c=='A'){
                set_options("hafuei",cmdline,flag,enable);
            }
            else if(c=='h'){
                flag.responseheader=enable;
            }
            else if(c=='a'){
                flag.auto_redirect=enable;
            }
            else if(c=='v'){
                if(!parse_cmdline(cmdline,1,false,flag.cacert)){
                    print_hookln("option: one argument required");
                    return Status::argnumnotmatch;
                }
            }
            else if(c=='f'){
                flag.filepayload=enable;
            }
            else if(c=='d'){
                if(!parse_cmdline(cmdline,1,false,flag.default_path)){
                    print_hookln("option: one argument required");
                    return Status::argnumnotmatch;
                }
            }
            else if (c=='b'){
                flag.onlybody=enable;
            }
            else if(c=='u'){
                flag.urlencoded=enable;
            }
            else if(c=='i'){
                flag.useinfocb=enable;
            }
            else if(c=='e'){
                flag.allcerr=enable;
            }
            else if(c=='t'){
                flag.interactive=enable;
            }
            else{
                print_hookln("option '",c,"' not found");
                return Status::optionnotfound;
            }
        }
    }
    return Status::succeed;
}

Status parse_option(Reader<std::string>& cmdline,flags& flag,bool enable){
    auto optionfailed=[]{print(std::cerr,"option parse failed\n");};
    while(cmdline.ahead("-")){
        std::string opt;
        if(!parse_cmdline(cmdline,1,false,opt)){
            return Status::argnumnotmatch;
        }
        opt.erase(0,1);
        bool enable=true;
        if(opt[0]=='!'){
            enable=false;
            opt.erase(0,1);
        }
        auto res=set_options(opt,cmdline,flag,enable);
        if(res!=Status::succeed)return res;
    }  
    return Status::succeed;
}

void info_callback(const void* c,int type,int ret){
    auto println=[](auto... arg){print(cerrf?std::cerr:std::cout,arg...,"\n");};
    auto fmt=[&println,&ret](auto type){println("info:",type," ret:",ret);};
#if USE_SSL
    if(SSL_CB_HANDSHAKE_START==type){
        fmt("ssl handshak start");
        return;
    }
    else if(SSL_CB_HANDSHAKE_DONE==type){
        fmt("ssl handshake done");
        return;
    }
    std::string msg="at ";
    bool hasprev=false;
    auto addmsg=[&msg,&hasprev](auto ty){
        if(hasprev)msg+=",";
        msg+=ty;
        hasprev=true;
    };
    if(SSL_CB_LOOP&type){
        addmsg("loop");
    }
    if(SSL_CB_EXIT&type){
        addmsg("exit");
    }
    if(SSL_ST_CONNECT&type){
        addmsg("connect");
    }
    if(SSL_CB_READ&type){
        addmsg("read");
    }
    if(SSL_CB_WRITE&type){
        addmsg("write");
    }
    if(SSL_CB_ALERT&type){
        addmsg("alert");
    }
    fmt(msg);
#endif
}

Status run_httpmethod(Reader<std::string>& cmdline,HTTPClient& client,flags& flag){
    auto print_hook=[&flag](auto... args){
        print(flag.allcerr?std::cerr:std::cout,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    client.set_auto_redirect(flag.auto_redirect);
    client.set_cacert(flag.cacert);
    client.set_default_path(flag.default_path);
    client.set_encoded(flag.urlencoded);
    client.set_infocb(flag.useinfocb?info_callback:nullptr);
    cerrf=flag.allcerr;
    std::string savedfile;
    auto res=httpmethod(cmdline,client,flag,savedfile);
    if(res==Status::succeed||res==Status::notsucceedstatus||res==Status::fileoutopenerr||res==Status::filewriteerror){
        if(flag.onlybody){
            print_hookln(client.body());
        }
        else if(flag.responseheader){
            print_hookln(client.raw());
        }
        print_hookln("status code:",client.statuscode());
        print_hookln("reason phrase:",client.reasonphrase());
        auto delta=std::chrono::duration_cast<std::chrono::microseconds>(client.time()).count();
        print_hookln("time:",delta/1000.0," msec");
        if(client.body().size()==0){
            print_hookln("no content");
        }
        else{
            print_hookln("content length is ",client.body().length());
        }
        auto header_show=[&client,&print_hookln](auto name,auto key,bool are=false){
            if(client.header().count(key)==1){
                print_hookln(name,are?" are ":" is ",(*client.header().equal_range(key).first).second);
            }
        };
        header_show("server","server");
        header_show("content type","content-type");
        header_show("location","location");
        header_show("allow method","allow",true);
        if(res==Status::fileoutopenerr){
            print_hookln("file '"+savedfile+"' was selected to save but couldn't open.");
        }
        else if(res==Status::filewriteerror){
            print_hookln("content was saved to '"+savedfile+"' but maybe broken.");
        }
        else if(res==Status::notsucceedstatus){
            print_hookln("content was not saved because status code is ",client.statuscode());
        }
        else if(savedfile!=""){
            print_hookln("content was saved to '"+savedfile+"'");
        }
    }
    else if(res==Status::httperror){
        print_hookln(client.err());
    }
    else if(res==Status::fileinopenerror){
        print_hookln("file '"+savedfile+"' was selected for payload but couldn't open.");
    }
    else if(res==Status::argnumnotmatch){
        print_hookln("arg number not matched.");
    }
    else if(res==Status::argconditionnotmatch){
        print_hookln("arg condition not matched.");
    }
    else{
        print_hookln("fatal error.");
    }
    return res;
}

template<class Func>
bool print_cmd_status(Func& println,flags& flag){
    println("stderr=",flag.allcerr);
    println("auto-redirect=",flag.auto_redirect);
    println("cacert-file=",flag.cacert);
    println("default-path=",flag.default_path);
    println("file-payload=",flag.filepayload);
    println("interactive=",flag.interactive);
    println("body=",flag.onlybody);
    println("header=",flag.responseheader);
    println("url-encoded=",flag.urlencoded);
    println("use-infocb=",flag.useinfocb);
    return true;
}

template<class Func>
bool show_flag_help(Func& println,bool withm){
    auto mprintln=[&println,withm](auto longname,auto shortname,auto... caption){
        println(withm?"-":"",longname,",",withm?"-":"",shortname,":",caption...);
    };
    mprintln("header","h","show all response");
    mprintln("body","b","show body");
    mprintln("url-encoded","u","not encode url");
    mprintln("file-payload","f","interpret <body> as filename");
    mprintln("auto-redirect","a","redirect automatically");
    mprintln("cacert-file","v","set cacert file location. default:./cacert.pem");
    mprintln("default-path","d","set default path. default:/");
    mprintln("interactive","t","use interactive mode");
    mprintln("use-infocb","i","use ssl info callback");
    mprintln("stderr","e","all output to stderr");
    mprintln("all","A","set option",withm?"-":"","hafuei");
    return true;
}

template<class Func>
bool show_http_help(Func& println){
    println("get <url> [<savefile>]");
    println("head <url>");
    println("post <url> <body> [<savefile>]");
    println("put <url> <body> [<savefile>]");
    println("patch <url> <body> [<savefile>]");
    println("options <url> [<savefile>]");
    println("trace <url> [<savefile>]");
    println("delete <url> [<savefile>]");
    println("method <method> [no-body] <url> [<body>] [<savefile>] (not recommended)");
    return true;
}

template<class Func>
bool show_cmdline_help(Func& println,const std::string& usage){
    println("Usage:");
    println(usage);
    println("http method:");
    show_http_help();
    println("other method:");
    println("help:show this help");
    println("flags:");
    show_flag_help(println,true);
    return true;
}

template<class Func>
bool show_interpreter_help(Func& println,const std::string& usage){
    println("Usage:");
    println(usage);
    println("http method:");
    show_http_help();
    println("other method:");
    println("flag")
    println("flags:");
    show_flag_help(println,false);
}

Status command_one(Reader<std::string>& cmdline,HTTPClient& client,flags& flag){
    auto print_hook=[&flag](auto... args){
        print(flag.allcerr?std::cerr:std::cout,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    if(cmdline.expect("flag",is_c_id_usable)){
        bool enable=true;
        int expected=0;
        if(cmdline.expect("true",is_c_id_usable)){
            expected=1;
        }
        else if(cmdline.expect("false",is_c_id_usable)){
            expected=1;
            enable=false;
        }
        std::string opt;
        if(!parse_cmdline(cmdline,expected,false,opt)){
            print_hookln("arg number not matched"); 
            return Status::argnumnotmatch;  
        }
        Status ret;
        if(opt!=""){
            ret=set_options(opt,cmdline,flag,enable);
        }
        print_cmd_status(print_hookln,flag);
        return ret;
    }
    else if(cmdline.expect("exit",is_c_id_usable)||cmdline.expect("quit",is_c_id_usable)){
        return Status::quit;
    }
    else if(cmdline.expect("cd",is_c_id_usable)){
        std::string dir;
        if(!parse_cmdline(cmdline,1,false,dir)){
            print_hookln("arg number not matched"); 
            return Status::argnumnotmatch;  
        }
        if (_chdir(dir.c_str())) {
			print_hookln(dir, ":no such dirctry");
			return Status::argconditionnotmatch;
		}
        return Status::cdchanged;
    }
    else if(cmdline.expect("help",is_c_id_usable)){
        show_interpreter_help(print_hookln,"<method> <args>");
    }
    else{
        return run_httpmethod(cmdline,client,flag);
    }
}

bool interactive_mode(HTTPClient& client,flags& flag){
    auto print_hook=[&flag](auto... args){
        print(flag.allcerr?std::cerr:std::cout,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    print_hookln("interactive mode");
    char* cd=_getcwd(nullptr,1000);
    if(!cd){
        print_hookln("warning: can't get cwd");
    }
    while(true){
        if(cd)print_hook(cd);
        print_hook(">>");
        std::string input;
        std::getline(std::cin,input);
        Reader<std::string> cmdline(std::move(input),ignore_space);
        auto res=command_one(cmdline,client,flag);
        if(res==Status::quit){
            break;
        }
        else if(res==Status::cdchanged){
            std::free(cd);
            cd=_getcwd(nullptr,1000);
            if(!cd){
                print_hookln("warning: can't get cwd");
            }
        }
    }
    return true;
}

int netclient_start(const char* str){
    if(!str)return -1;
    Reader<std::string> cmdline(str,ignore_space);
    std::string procname;
    if(!parse_cmdline(cmdline,1,false,procname))return -1;
    flags gflag;
    auto s=parse_option(cmdline,gflag,true);
    if(s!=Status::succeed){
        return (int)s;
    }
    if(cmdline.expect("help",is_c_id_usable)){
        show_cmdline_help([&gflag](auto... arg){
            print(gflag.allcerr?std::cerr,std::cout,arg...,"\n");
        },procname+" <option> <http method>");
        return 0;
    }
    HTTPClient client;
    int ret=0;
    if(gflag.interactive){
        interactive_mode(client,gflag);
    }
    else{
        ret=(int)run_httpmethod(cmdline,client,gflag);
    }
    return ret;
}