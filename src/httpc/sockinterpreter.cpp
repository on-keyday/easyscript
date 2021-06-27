#define DLL_EXPORT __declspec(dllexport)
#include"http1.h"
#include"sockinterpreter.h"
#include<iostream>
#include<direct.h>
#if USE_SSL
#include <openssl/evp.h>
#endif
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
    cdchanged,
    optionerror
};

struct flags{
    bool auto_redirect=false;
    bool header=false;
    bool onlybody=false;
    bool filepayload=false;
    bool urlencoded=false;
    bool useinfocb=false;
    std::string cacert="./cacert.pem";
    std::string default_path="/";
    bool allcerr=false;
    bool interactive=false;
    bool saveonlysucceed=false;
    bool noinfo=false;
    std::multimap<std::string,std::string> customheader;
    bool add_reqmake=false;
    bool out_and_err=false;
    bool show_request=false;
};


template<class Stream>
void print(Stream&){}

template<class Stream,class First,class... Args>
void print(Stream& st,First first,Args... args){
    st << first;
    print(st,args...);
}

bool cerrf=false;
bool out_err=false;

template<class... Args>
void print_with_flag(flags& flag,Args... args){
    if(flag.out_and_err){
        print(std::cout,args...);
        print(std::cerr,args...);
    }
    else{
        print(flag.allcerr?std::cerr:std::cout,args...);
    }
}


Status set_custom_header(Reader<std::string>& cmdline,flags& flag,bool enable){
    std::string header,key,value;
    if(!parse_cmdline(cmdline,1,false,header))return Status::argnumnotmatch;
    Reader<std::string> check(header);
    check.readwhile(key,untilincondition,[](char c){
        return is_alphabet(c)||c=='-';
    });
    if(enable){
        if(!check.expect(":"))return Status::argconditionnotmatch;
        check.expect(" ");
        check.readwhile(value,untilincondition,[](char c){return true;});
        flag.customheader.insert({{key,value}});
    }
    else{
        flag.customheader.erase(key);
    }
    return Status::succeed;
}

Status set_options(const std::string& opt,Reader<std::string>& cmdline,flags& flag,bool enable){
    auto print_hook=[&flag](auto... args){
        print_with_flag(flag,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    if(opt=="")return Status::argnumnotmatch;
    if(opt=="all"){
        return set_options("hafueisn",cmdline,flag,enable);
    }
    if(opt=="header"){
        flag.header=enable;
    }
    else if(opt=="body"){
        flag.onlybody=enable;
    }
    else if(opt=="custom-header"){
        auto tmp=set_custom_header(cmdline,flag,enable);
        if(tmp!=Status::succeed){
            print_hookln("arg not matched.");
        }
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
    else if(opt=="out-and-err"){
        flag.out_and_err=enable;
        out_err=enable;
    }
    else if(opt=="save-if-succeed"){
        flag.saveonlysucceed=enable;
    }
    else if(opt=="no-info"){
        flag.noinfo=enable;
    }
    else if(opt=="show-request"){
        flag.show_request=enable;
    }
    else{
        for(auto c:opt){
            if(c=='A'){
                set_options("hafueisn",cmdline,flag,enable);
            }
            else if(c=='h'){
                flag.header=enable;
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
                cerrf=enable;
            }
            else if(c=='t'){
                flag.interactive=enable;
            }
            else if(c=='s'){
                flag.saveonlysucceed=enable;
            }
            else if(c=='n'){
                flag.noinfo=enable;
            }
            else if(c=='c'){
                auto tmp=set_custom_header(cmdline,flag,enable);
                if(tmp!=Status::succeed){
                    print_hookln("arg not matched.");
                    return tmp;
                }
            }
            else if(c=='r'){
                flag.out_and_err=enable;
                out_err=enable;
            }
            else if(c=='l'){
                flag.show_request=enable;
            }
            else{
                print_hookln("option '",c,"' not found");
                return Status::optionnotfound;
            }
        }
    }
    return Status::succeed;
}

template<class Func>
bool show_flag_help(Func& println,bool withm){
    auto mprintln=[&println,withm](auto longname,auto shortname,auto... caption){
        println("    ",withm?"-":"",longname,",",withm?"-":"",shortname,":",caption...);
    };
    mprintln("header","h","show all response");
    mprintln("show-request","l","show request");
    mprintln("body","b","show response body");
    mprintln("url-encoded","u","not encode url");
    mprintln("file-payload","f","interpret <body> as filename");
    mprintln("auto-redirect","a","redirect automatically");
    mprintln("cacert-file","v","set cacert file location. default:./cacert.pem");
    mprintln("default-path","d","set default path. default:/");
    mprintln("interactive","t","use interactive mode");
    mprintln("use-infocb","i","use ssl info callback");
    mprintln("stderr","e","all output to stderr");
    mprintln("save-if-suceed","s","save file if status code succeed");
    mprintln("no-info","n","not show access infomation");
    mprintln("custom-header","c","edit custom header");
    mprintln("out-and-err","r","output both stdout and stderr");
    mprintln("all","A","set option '",withm?"-":"","hafueisn'");
    return true;
}

template<class Func>
bool print_cmd_status(Func& println,flags& flag){
    auto printlnf=[&println](auto name,bool f){
        println(name,f?"true":"false");
    };
    printlnf("stderr=",flag.allcerr);
    printlnf("auto-redirect=",flag.auto_redirect);
    println("cacert-file=",flag.cacert);
    println("default-path=",flag.default_path);
    printlnf("file-payload=",flag.filepayload);
    printlnf("interactive=",flag.interactive);
    printlnf("body=",flag.onlybody);
    printlnf("header=",flag.header);
    printlnf("url-encoded=",flag.urlencoded);
    printlnf("use-infocb=",flag.useinfocb);
    printlnf("save-if-succeed=",flag.saveonlysucceed);
    printlnf("no-info=",flag.noinfo);
    printlnf("out-and-err=",flag.out_and_err);
    printlnf("show-request=",flag.show_request);
    println("custom-header={",flag.customheader.size()?"":"}");
    if(flag.customheader.size()){
        for(auto& s:flag.customheader){
            println("    ",s.first,": ",s.second);
        }
        println("}");
    }
    return true;
}

Status parse_option(Reader<std::string>& cmdline,flags& flag){
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
        if(opt=="")print_with_flag(flag,"not enable option");
        auto res=set_options(opt,cmdline,flag,enable);
        if(res!=Status::succeed)return res;
    }  
    return Status::succeed;
}

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


Status unsafe_method(std::string& name,Reader<std::string>& cmdline,HTTPClient& client,flags& flag,std::string& url,std::string& file){
    Reader<std::string> checker(name);
    checker.readwhile(untilincondition_nobuf,is_alphabet<char>);
    if(!checker.ceof())return Status::argconditionnotmatch;
    bool nobody=false;
    if(cmdline.expect("no-body",is_c_id_usable)){
        nobody=true;
    }
    std::string body;
    if(!parse_cmdline(cmdline,1,true,url,body))return Status::argnumnotmatch;
    if(!parse_cmdline(cmdline,0,!flag.filepayload,file))return Status::argconditionnotmatch;
    if(body!=""&&flag.filepayload){
        auto loadf=read_from_file(body,file);
        if(loadf!=Status::succeed)return loadf;
    }
    if(!client.method(checker.ref().c_str(),url.c_str(),body.c_str(),body.size(),nobody))return Status::httperror;
    if(file!=""){
        if(flag.saveonlysucceed&&!is_success_status(client.statuscode()))return Status::notsucceedstatus;
        return write_to_file(client,file);
    }
    return Status::succeed;
}

Status httpmethod(std::string& method,Reader<std::string>& cmdline,HTTPClient& client,flags& flag,std::string& file,std::string& url){
    std::string body;
    if(!parse_cmdline(cmdline,1,true,url))return Status::argnumnotmatch;
    if(method=="method")return unsafe_method(url,cmdline,client,flag,url,file);
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


void info_callback(const void* c,int type,int ret){
    auto println=[](auto... arg){
        if(out_err){
            print(std::cout,arg...,"\n");
            print(std::cerr,arg...,"\n");
        }
        else{
            print(cerrf?std::cerr:std::cout,arg...,"\n");
        }
    };
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

int verify_callback(int ret,void* ctxp){
#if USE_SSL
    auto println=[](auto... arg){
        if(out_err){
            print(std::cout,arg...,"\n");
            print(std::cerr,arg...,"\n");
        }
        else{
            print(cerrf?std::cerr:std::cout,arg...,"\n");
        }
    };
    X509_STORE_CTX* ctx=(X509_STORE_CTX*)ctxp;
    if(!ret){
        auto err=X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx));
        if(err)println("error:",err);
    }
    println("depth:",X509_STORE_CTX_get_error_depth(ctx));
    auto cert=X509_STORE_CTX_get_current_cert(ctx);
    if(!cert){
        println("warning:can't get cert.");
        return ret;
    }
    auto issuername=X509_get_issuer_name(cert);
    auto subjectname=X509_get_subject_name(cert);
    if(!issuername){
        println("warning:can't get issuername.");
    }
    if(!subjectname){
        println("warning:can't get subjectname.");
    }
    char namebuf[2000]={0};
    auto base64_encoder=[](auto buf,auto len){
        Base64Context bctx;
        std::string toprint;
        Reader<std::string> pubkeys(std::string(buf,len));
        pubkeys.readwhile(toprint,base64_encode,&bctx);
        return toprint;
    };
    auto url_encoder=[](auto& buf){
        URLEncodingContext<std::string> ctx;
        ctx.path=true;
        ctx.query=true;
        ctx.no_escape=":*@()'<>$#\"![]";
        std::string toprint;
        Reader<std::string> pubkeys(buf);
        pubkeys.readwhile(toprint,url_encode,&ctx);
        return toprint;
    };
    auto print_obj_value=[&url_encoder,&println,&namebuf](auto asn1obj,auto asn1data,bool encode=false){
        if(!asn1data||!asn1obj)return;
        OBJ_obj2txt(namebuf,sizeof(namebuf),asn1obj,0);
        auto datastr=ASN1_STRING_get0_data(asn1data);
        if(!datastr)return;
        std::string value((const char*)datastr,ASN1_STRING_length(asn1data));
        if(encode){
            value=url_encoder(value);
        }
        println(namebuf,"=",value);
    };
    auto print_X509_NAME=[&print_obj_value,&println](auto msg,auto name){
        if(!name)return;
        println(msg);
        auto max=X509_NAME_entry_count(name);
        for(auto i=0;i<max;i++){
            auto entry=X509_NAME_get_entry(name,i);
            if(!entry)continue;
            auto asn1obj=X509_NAME_ENTRY_get_object(entry);
            auto asn1data=X509_NAME_ENTRY_get_data(entry);
            print_obj_value(asn1obj,asn1data);
        }
    };
    print_X509_NAME("issuer info",issuername);
    print_X509_NAME("subject info",subjectname);
    auto exts=X509_get0_extensions(cert);
    if(exts){
        println("extension info");
        auto p=sk_X509_EXTENSION_value(exts,0);
        for(auto i=0;p;i++){
            auto obj=X509_EXTENSION_get_object(p);
            auto data=X509_EXTENSION_get_data(p);
            print_obj_value(obj,data,true);
            p=sk_X509_EXTENSION_value(exts,i+1);
        }
    }
    auto pubkey=X509_get0_pubkey(cert);
    if(!pubkey){
        println("warning:can't get public key.");
        return ret;
    }
    auto keytype=EVP_PKEY_base_id(pubkey);
    auto print_base64=[&println,&namebuf,&base64_encoder](auto len,auto bit,auto... msg){
        auto toprint=base64_encoder(namebuf,len);
        if(toprint.size()){
            println("key-len:",len,"byte(",bit,"bit)");
            println(msg...,toprint);
        }
    };
    auto print_key=[&println,&print_base64,&namebuf](const auto* key,auto... msg){
        if(!key){
            println("warining:can't get BIGNUM");
            return;
        }
        auto bit=BN_num_bits(key);
        auto len=(bit+7)/8;
        if(len>sizeof(namebuf)){
            println("warining: large key length:",len);
            return;
        }
        BN_bn2bin(key,(unsigned char*)namebuf);
        print_base64(len,bit,msg...);
    };
    if(keytype==EVP_PKEY_RSA){
        println("crypto:RSA");
        auto rsa=EVP_PKEY_get0_RSA(pubkey);
        if(!rsa){
            println("warining:can't get RSA");
            return ret;
        }
        const BIGNUM* num[3];
        RSA_get0_key(rsa,&num[0],&num[1],&num[2]);
        print_key(num[0],"key-n:");
        print_key(num[1],"key-e:");
        //print_key(num[2],"key-d:");
    }
    else if(keytype==EVP_PKEY_DSA){
        println("crypto:DSA");
        auto dsa=EVP_PKEY_get0_DSA(pubkey);
        if(!dsa){
            println("warining:can't get DSA");
            return ret;
        }
        auto key=DSA_get0_pub_key(dsa);
        print_key(key,"key:");
    }
    else if(keytype==EVP_PKEY_DH){
        println("crypto:DH");
        auto dh=EVP_PKEY_get0_DH(pubkey);
        if(!dh){
            println("warining:can't get DH");
            return ret;
        }
        auto key=DH_get0_pub_key(dh);
        print_key(key,"key:");
    }
    else if(keytype==EVP_PKEY_EC){
        println("crypto:EC");
        auto ec=EVP_PKEY_get0_EC_KEY(pubkey);
        if(!ec){
            println("warining:can't get EC_KEY");
            return ret;
        }
        auto len=i2o_ECPublicKey(ec,nullptr);
        if(len>sizeof(namebuf)){
            println("warining: large key length:",len);
            return ret;
        }
        unsigned char* buf=(unsigned char*)namebuf;
        if(!i2o_ECPublicKey(ec,&buf))return ret;
        print_base64(len,"maybe "+std::to_string(len*8-7),"key:");
        //println("debug:",namebuf);
    }
    else{
        println("crypto:UNKNOWN");
    }
#endif
    return ret;
}

bool add_header(void* flag,std::string& ret,const HTTPClient* cl){
    flags* flagp=(flags*)flag;
    if(!flagp->add_reqmake){
        ret.append("accept: */*\r\n");
        flagp->add_reqmake=true;
        return true;
    }
    for(auto& it:flagp->customheader){
        ret.append(it.first+": "+it.second+"\r\n");
    }
    return true;
}

Status run_httpmethod(Reader<std::string>& cmdline,HTTPClient& client,flags& flag){
    auto print_hook=[&flag](auto... args){
        print_with_flag(flag,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    size_t pos=cmdline.readpos();
    if(!flag.noinfo){
        print_hookln("command:",&cmdline.ref().c_str()[pos]);
    }
    std::string method,savedfile,url;
    if(!parse_cmdline(cmdline,1,true,method))return Status::argnumnotmatch;
    auto tmp=parse_option(cmdline,flag);
    if(tmp!=Status::succeed)return tmp;
    client.set_auto_redirect(flag.auto_redirect);
    client.set_cacert(flag.cacert);
    client.set_default_path(flag.default_path);
    client.set_encoded(flag.urlencoded);
    client.set_infocb(flag.useinfocb?info_callback:nullptr);
    client.set_verifycb(flag.useinfocb?verify_callback:nullptr);
    cerrf=flag.allcerr;
    out_err=flag.out_and_err;
    auto res=httpmethod(method,cmdline,client,flag,savedfile,url);
    if(res==Status::succeed||res==Status::notsucceedstatus||res==Status::fileoutopenerr||res==Status::filewriteerror){
        if(flag.show_request){
            print_hook(client.requesst());
            print_hookln("");
        }
        if(flag.onlybody){
            print_hook(client.body());
            if(!flag.noinfo)print_hookln("");
        }
        else if(flag.header){
            print_hook(client.raw());
            if(!flag.noinfo)print_hookln("");
        }
        
        if(!flag.noinfo){
            print_hookln("url:",url);
            print_hookln("accessed address:",client.ipaddress());
            print_hookln("status code:",client.statuscode());
            print_hookln("reason phrase:",client.reasonphrase());
            auto delta=std::chrono::duration_cast<std::chrono::nanoseconds>(client.time()).count();
            if(delta<1000){
                print_hookln("time:",delta," nsec");
            }
            else if(delta<1000*1000){
                print_hookln("time:",delta/1000.0," usec");
            }
            else{
                print_hookln("time:",delta/(1000.0*1000.0)," msec");
            }
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
        }
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
bool show_http_help(Func& printl){
    auto println=[&printl](auto arg){
        printl("    ",arg);
    };
    println("get [<option>] <url> [<savefile>]");
    println("head [<option>] <url>");
    println("post [<option>] <url> <body> [<savefile>]");
    println("put [<option>] <url> <body> [<savefile>]");
    println("patch [<option>] <url> <body> [<savefile>]");
    println("options [<option>] <url> [<savefile>]");
    println("trace [<option>] <url> [<savefile>]");
    println("delete [<option>] <url> [<body>] [<savefile>]");
    println("method [<option>] <method> [no-body] <url> [<body>] [<savefile>] (not recommended)");
    return true;
}

template<class Func>
bool show_cmdline_help(Func& println,const std::string& usage){
    println("Usage:");
    println(usage);
    println("http method:");
    show_http_help(println);
    println("other method:");
    println("    help:show this help");
    println("flags:");
    show_flag_help(println,true);
    return true;
}

template<class Func>
bool show_interactive_help(Func& println,const std::string& usage){
    println("Usage:");
    println(usage);
    println("http method:");
    show_http_help(println);
    auto printlnt=[&println](auto arg){
        println("    ",arg);
    };
    println("other method:");
    printlnt("flag [<flag>] [<arg>]:show flag");
    printlnt("cd <dirname>:set current directory");
    printlnt("help:show this help");
    printlnt("clear:clear console");
    printlnt("exit:finish interactive mode");
    printlnt("quit:same as exit");
    println("flags:");
    show_flag_help(println,true);
    return true;
}

Status command_one(Reader<std::string>& cmdline,HTTPClient& client,flags& flag){
    auto print_hook=[&flag](auto... args){
        print_with_flag(flag,args...);
    };
    auto print_hookln=[&print_hook](auto... args){
        print_hook(args...,"\n");
    };
    if(cmdline.expect("flag",is_c_id_usable)){
        /*bool enable=true;
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
            if(opt[0]=='!'){
                opt.erase(0,1);
                enable=!enable;
            }
            ret=set_options(opt,cmdline,flag,enable);
        }*/
        auto ret=parse_option(cmdline,flag);
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
			print_hookln(dir, ":no such directory");
			return Status::argconditionnotmatch;
		}
        return Status::cdchanged;
    }
    else if(cmdline.expect("clear",is_c_id_usable)){
        ::system("cls");
    }
    else if(cmdline.expect("help",is_c_id_usable)){
        show_interactive_help(print_hookln,"<method> <args>");
    }
    else{
        return run_httpmethod(cmdline,client,flag);
    }
    return Status::succeed;
}

bool interactive_mode(HTTPClient& client,flags& flag){
    auto print_hook=[&flag](auto... args){
        print_with_flag(flag,args...);
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
        std::cin.clear();
        Reader<std::string> cmdline(std::move(input),ignore_space);
        auto res=command_one(cmdline,client,flag);
        if(res==Status::quit){
            print_hookln("Bye");
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

int netclient_str(const char* str){
    if(!str)return -1;
    Reader<std::string> cmdline(str,ignore_space);
    std::string procname;
    if(!parse_cmdline(cmdline,1,false,procname))return -1;
    flags gflag;
    auto s=parse_option(cmdline,gflag);
    if(s!=Status::succeed){
        return (int)s;
    }
    auto println=[&gflag](auto... arg){
        print_with_flag(gflag,arg...,"\n");
    };
    if(cmdline.expect("help",is_c_id_usable)){
        show_cmdline_help(println,procname+" [<option>] <method>");
        return 0;
    }
    else if(cmdline.expect("flag",is_c_id_usable)){
        print_cmd_status(println,gflag);
        return 0;
    }
    HTTPClient client;
    client.set_requestadder(add_header,&gflag);
    int ret=0;
    bool entered=false;
    if(gflag.interactive){
        interactive_mode(client,gflag);
        entered=true;
    }
    if(!cmdline.eof()){
        ret=(int)run_httpmethod(cmdline,client,gflag);
    }
    else if(!entered){
        print_with_flag(gflag,"input method!\n");
        return -1;
    }
    //print(gflag.allcerr?std::cerr:std::cout,"debug:",cmdline.ref());
    return ret;
}

int netclient_argv(int argc,char** argv){
    std::string argstr;
    for(int i=0;i<argc;i++){
        if(i){
            argstr+=" ";
        }
        if(argv[i][0]=='-'||argv[i][0]=='\''||argv[i][0]=='\"'){
            argstr+=argv[i];
        }
        else{
            Reader<std::string> check(argv[i]);
            check.readwhile(untilincondition_nobuf,is_alpha_or_num<char>);
            if(check.ceof()){
                argstr+=argv[i];
            }
            else{
                argstr+="'";
                argstr+=argv[i];
                argstr+="'";
            }
        }
    }
    return netclient_str(argstr.c_str());
}