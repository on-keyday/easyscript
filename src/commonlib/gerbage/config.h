#pragma once
#include"project_name.h"
#include"reader.h"
#include"utf_helper.h"
#include"utfreader.h"
#include"json_util.h"
#include"extension_operator.h"
#include"basic_helper.h"
#include"fileio.h"
#include<string>
#include<map>
#include<vector>
#include<assert.h>
namespace PROJECT_NAME{
    struct CRange{
        char32_t prev;
        char32_t post;
        CRange(char32_t prev,char32_t post):prev(prev),post(post){}
    };
    enum class ConfigType{
        none,
        range,
        string,
        rawstr,
        list,
        integer,
        integer_u,
        float_,
        binary,
        config,
        identifier,
    };

    struct Config{
        ConfigType type;
        union{
            CRange range;
            std::vector<Config>* list;
            std::map<std::string,Config>* config;
            double numf;
            int64_t numi;
            uint64_t numu;
            EasyStr str;
            EasyStr bin;
            EasyStr name;
        };
        Config():type(ConfigType::none){}
        Config(char32_t prev,char32_t post):range(prev,post),type(ConfigType::range){}
        Config(const char* str,size_t size,ConfigType type):str(str,size),type(type){}
        Config(int64_t in):numi(in),type(ConfigType::integer){}
        Config(uint64_t in):numu(in),type(ConfigType::integer_u){}
        Config(double in):numf(in),type(ConfigType::float_){}

        Config(std::vector<Config>&& in):type(ConfigType::list){
            list=new std::vector<Config>();
            *list=std::move(in);
        }

        Config(std::map<std::string,Config>&& in):type(ConfigType::config){
            config=new std::map<std::string,Config>();
            *config=std::move(in);
        }

        void destruct(){
            switch(type){
            case ConfigType::string:
            case ConfigType::rawstr:
            case ConfigType::binary:
            case ConfigType::identifier:
                str.~EasyStr();
                break;
            case ConfigType::list:
                delete list;
                list=nullptr;
                break;
            case ConfigType::config:
                delete config;
                config=nullptr;
                break;
            default:
                numi=0;
            }
        }

        void move(Config&& in){
            switch(in.type){
            case ConfigType::string:
            case ConfigType::rawstr:
            case ConfigType::binary:
            case ConfigType::identifier:
                str=std::move(in.str);
                break;
            case ConfigType::list:
                list=in.list;
                in.list=nullptr;
                break;
            default:
                numi=in.numi;
            };
        }

        Config(Config&& in){
            move(std::forward<Config>(in));
        }

        Config& operator=(Config&& in){
            destruct();
            move(std::forward<Config>(in));
            return *this;
        }

        ~Config(){
            destruct();
        }
    };

    template<class Buf>
    struct ConfigParser{
    private:
        Reader<Buf> r;
        const char* errmsg=nullptr;
        void error(const char* err){
            errmsg=err;
        }
        bool range(Config& config){
            auto read_char=[&,this](auto& pre){
                auto tmp=r.set_ignore(nullptr);
                if(!r.expect("'")){
                    error("range:expected \"'\" but not");
                    return false;
                }
                int err=0;
                pre=utf8toutf32_impl(&r,&err);
                if(err){
                    error("range:error at first param");
                    return false;
                }
                if(!r.expect("'")){
                    error("range:expected \"'\" but not");
                    return false;
                }
                r.set_ignore(tmp);
                return true;
            };
            char32_t prev=0,post=0;
            if(!read_char(prev)){
                return false;
            }
            if(!r.expect("-")){
                error("range:expected '-' but not");
                return false;
            }
            if(!read_char(post)){
                return false;
            }
            if(prev>post){
                error("range:post value must bigger than prev value");
                return false;
            }
            config=Config(prev,post);
            return true;
        }

        void escape(std::string& str){
            std::string out;
            for(auto& i:str){
                switch(i){
                case '\n':
                    out+="\\n";
                    break;
                case '\r':
                    out+="\\r";
                    break;
                case '\a':
                    out+="\\a";
                    break;
                case '\b':
                    out+="\\b";
                    break;
                case '\0':
                    out+="\\0";
                    break;
                case '\\':
                    out+="\\\\";
                    break;
                default:
                    out+=i;
                }
            }
            str=std::move(out);
        }

        bool deescape(std::string& str){
            std::string out;
            bool escaped=false;
            Reader<Refer<std::string>> test(str);
            while(!test.ceof()){
                if(test.achar()=='\\'){
                    test.increment();
                    switch(test.achar()){
                    case 'n':
                        out+="\n";
                        break;
                    case 'r':
                        out+="\r";
                        break;
                    case 't':
                        out+='\t';
                        break;
                    case '0':
                        out+="\0";
                        break;
                    case 'a':
                        out+="\a";
                        break;
                    case 'b':
                        out+="\b";
                        break;
                    case '\\':
                        out+="\\";
                        break;
                    default:
                        out+=test.achar();
                    }
                }
                else{
                    out+=test.achar();
                }
                test.increment();
            }
            str=std::move(out);
            return true;
        }

        bool string(Config& config){
            std::string str;
            if(!read_string(str)){
                return false;
            }
            if(!deescape(str)){
                return false;
            }
            config=Config(str.c_str(),str.size(),ConfigType::string);
            return true;
        }   

        bool rawstring(Config& config){
            std::string str;
            if(!read_string(str)){
                return false;
            }
            config=Config(str.c_str(),str.size(),ConfigType::rawstr);
            return true;
        }

        bool numbers(Config& config){
            auto pos=r.readpos();
            std::string str;
            NumberContext<char> ctx;
            bool minus=false;
            if(r.expect("-")){
                minus=true;
            }
            r.readwhile(str,number,&ctx);
            if(ctx.failed||str.size()==0){
                error("invalid number");
                return false;
            }
            if(ctx.radix==16||ctx.radix==2){
                str.erase(0,2);
            }
            else if(ctx.radix==8){
                str.erase(0,1);
            }
            if(any(ctx.flag&NumberFlag::floatf)){
                double f;
                if(!parse_float(str,f,ctx.radix==16)){
                    error("unparsable number");
                    return false;
                }
                if(minus){
                    f=-f;
                }
                config=Config(f);
            }
            else{
                if(minus){
                    int64_t s;
                    if(!parse_int(str,s,ctx.radix)){
                        error("unparsable number");
                        return false;
                    }
                    s=-s;
                    config=Config(s);
                }
                else{
                    uint64_t s;
                    if(!parse_int(str,s,ctx.radix)){
                        error("unparsable number");
                        return false;
                    }
                    config=Config(s);
                }
            }
            return true;
        }

        bool read_string(std::string& str,bool noline=true){
            if(!r.string(str,noline)){
                error("string:invalid string");
                return false;
            }
            str.erase(0,1);
            str.pop_back();
            if(str.size()!=0&&ToUTF32<std::string>(str).size()==0){
                error("string:invalid Unicode character");
                return false;
            }
            return true;
        }

        bool read_id(std::string& key){
            if(r.achar()=='i'&&r.offset(1)=='"'){
                r.increment();
                return read_string(key);
            }
            else if(r.achar()=='"'){
                return read_string(key);
            }
            auto tmp=r.set_ignore(nullptr);
            int ctx=0;
            int* ctxp=&ctx;
            while(!r.ceof()){
                auto n=r.achar();
                if(n==' '||n=='\t'||n=='\n'||n=='\r'||n==':'||n=='='||
                    n=='{'||n=='}'||n=='('||n==')'||n=='['||n==']'||n==','){
                    break;
                }
                U8MiniBuffer minbuf;
                utf8_read(&r,minbuf,ctxp,false);
                r.increment();
                if(ctx){
                    error("config:invalid UTF-8 string");
                    return false;
                }
                key.append((const char*)minbuf.minbuf,minbuf.size());   
            }
            r.set_ignore(tmp);
            return true;
        }

        bool list(Config& config){
            std::vector<Config> vec;
            while(!r.expect("]")){
                Config in;
                if(!parse_config(in)){
                    return false;
                }
                vec.push_back(std::move(in));
                r.expect(",");
            }
            config=std::move(vec);
            return true;
        }

        bool config(Config& config,bool root=false){
            std::map<std::string,Config> cfg;
            while(true){
                if(root){
                    if(r.eof()){
                        break;
                    }
                }
                else{
                    if(r.expect("}")){
                        break;
                    }
                }
                std::string key;
                Config value;
                if(!read_id(key)){
                    return false;
                }
                if(key.size()==0){
                    error("config:invalid key value");
                    return false;
                }
                if(cfg.count(key)){
                    error("config:key must be unique");
                    return false;
                }
                if(!r.expect("=")&&!r.expect(":")){
                    error("config:expected = or : but not");
                    return false;
                }
                if(!parse_config(value)){
                    return false;
                }
                cfg[key]=std::move(value);
                r.expect(",");
            }
            config=std::move(cfg);
            return true;
        }

        bool binary(Config& config){
            auto tmp=r.set_ignore(nullptr);
            if(!r.expect("(")){
                error("binary:expected ( but not");
                return false;
            }
            int i=0;
            if(!r.read_byte(&i,4,translate_byte_net_and_host,true)<4){
                error("binary:need least 4 byte but not");
                return false;
            }
            std::string bytes;
            bytes.reserve((size_t)i);
            if(r.read_byte(bytes.data(),(size_t)i,translate_byte_as_is,true)<(size_t)i){
                error("binary:require byte not exist");
                return false;
            }
            r.set_ignore(tmp);
            if(!r.expect(")")){
                error("binary:expected ) but not");
                return false;
            }
            return true;
        }

        bool parse_config(Config& config){
            if(r.expect("{")){
                if(!this->config(config)){
                    return false;    
                }
            }
            else if(r.expect("[")){
                if(!this->list(config)){
                    return false;
                }
            }
            else if(r.ahead("(")){
                if(!this->binary(config)){
                    return false;
                }
            }
            else if(is_digit(r.achar())){
                if(!this->numbers(config)){
                    return false;
                }
            }
            else if(r.achar()=='"'){
                if(!this->string(config)){
                    return false;
                }
            }
            else if(r.achar()=='\''){
                if(!this->range(config)){
                    return false;
                }
            }
            else if(r.achar()=='`'){
                if(!this->rawstring(config)){
                    return false;
                }
            }
            else {
                std::string str;
                if(!read_id(str)){
                    return false;
                }
                while(true){
                    if(r.expect("[")){
                        str+="[";
                        if(!read_id(str)){
                            return false;
                        }
                        str+="]";
                        continue;
                    }
                    break;
                }
                config=Config(str.c_str(),str.size(),ConfigType::identifier);
            }
            return true;
        }
    public:
        bool config_root(Config& ret){
            r.set_ignore(ignore_c_comments);
            if(r.expect("{")){
                return config(ret,false);
            }
            else if(r.expect("[")){
                return list(ret);
            }
            else{
                return config(ret,true);
            }
        }

        const char* get_errmsg()const{
            return errmsg?errmsg:"no error";
        }

        ConfigParser(Buf&& buf):r(std::forward<Buf>(buf)){}
        ConfigParser(const Buf& buf):r(buf){}
    };

    template<class C>
    Config loadconfig(const C* cfg){
        ConfigParser<FileMap> ps=FileMap(cfg);
        Config ret;
        if(!ps.config_root(ret)){
            return Config();
        }
        return ret;
    }

    template<class Buf>
    Config makeconfig(const Buf& buf){
        ConfigParser<Refer<const Buf>> ps(buf);
        Config ret;
        if(!ps.config_root(ret)){
            return Config();
        }
        return ret;
    }
}