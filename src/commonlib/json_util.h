#pragma once
#include"reader.h"
#include"basic_helper.h"
#include<string>
#include<unordered_map>
#include<vector>
#include<cstring>
#include<stdint.h>


namespace PROJECT_NAME{
    enum class JSONType{
        unset,
        null,
        boolean,
        integer,
        unsignedi,
        floats,
        string,
        object,
        array
    };

    enum class JSONFormat{
        space=0x1,
        tab=0x2,
        mustline=0x4,
        elmpush=0x8,
        afterspace=0x10,
        mustspace=0x20,
        endline=0x40,
        must=mustline|mustspace,
        defaultf=space|afterspace|endline,
        tabnormal=tab|afterspace|endline,
        shift=space|elmpush|afterspace,
        spacemust=space|mustspace,
        noendline=space|afterspace,
        noendlinetab=tab|afterspace
    };

    
    struct EasyStr{
    private:
        char* str=nullptr;
        size_t len=0;
        void construct(const char* from,size_t fromlen){
            try{
                str=new char[fromlen+1];
                len=fromlen;
            }
            catch(...){
                str=nullptr;
                len=0;
                return;
            }
            memmove(str,from,fromlen);
            str[len]=0;
        }
    public:
        EasyStr(){
            str=nullptr;
            len=0;
        }

        EasyStr(const char* from,size_t fromlen){
            construct(from,fromlen);
        }

        EasyStr(const char* from){
            construct(from,strlen(from));
        }

        EasyStr(const EasyStr&)=delete;
        EasyStr(EasyStr&& from)noexcept{
            str=from.str;
            len=from.len;
            from.str=nullptr;
            from.len=0;
        }
        EasyStr& operator=(EasyStr&& from)noexcept{
            if(str!=nullptr&&str==from.str)return *this;
            delete[] str;
            str=from.str;
            len=from.len;
            from.str=nullptr;
            from.len=0;
            return *this;
        }

        char& operator[](size_t pos){
            if(pos>len)return str[len];
            return str[pos];
        }

        const char* const_str()const{return str;}
        size_t size()const{return len;}

        ~EasyStr(){
            delete[] str;
            str=nullptr;
            len=0;
        }
    };

    struct JSON{
        using JSONObjectType=std::unordered_map<std::string,JSON>;
        using JSONArrayType=std::vector<JSON>;
    private:
        
        union{
            bool boolean;
            int64_t numi;
            uint64_t numu;
            double numf;
            EasyStr value=EasyStr();
            JSONObjectType* obj;
            JSONArrayType* array;
        };
        JSONType type=JSONType::unset;

        template<class T>
        T* init(){
            try{
                return new T();
            }
            catch(...){
                return nullptr;
            }
        }

        bool init_as_obj(JSONObjectType&& json){
            obj=init<JSONObjectType>();
            if(!obj){
                type=JSONType::unset;
                return false;
            }
            type=JSONType::object;
            *obj=std::move(json);
            return true;
        }

        bool init_as_array(JSONArrayType&& json){
            array=init<JSONArrayType>();
            if(!array){
                type=JSONType::unset;
                return false;
            }
            type=JSONType::array;
            *array=std::move(json);
            return true;
        }

        template<class Num>
        bool init_as_num(Num num){
            type=JSONType::integer;
            numi=num;
            return true;
        }

        void destruct(){
            if(type==JSONType::object){
                delete obj;
                obj=nullptr;
            }
            else if(type==JSONType::array){
                delete array;
                array=nullptr;
            }
            else if(type==JSONType::string){
                value.~EasyStr();
            }
            else{
                numu=0;
            }
        }

        JSON& move_assign(JSON&& from){
            this->type=from.type;
            if(type==JSONType::object){
                obj=from.obj;
                from.obj=nullptr;
            }
            else if(type==JSONType::array){
                array=from.array;
                from.array=nullptr;
            }
            else if(type==JSONType::boolean){
                boolean=from.boolean;
                from.boolean=false;
            }
            else if(type==JSONType::integer){
                numi=from.numi;
                from.numi=0;
            }
            else if(type==JSONType::unsignedi){
                numu=from.numu;
                from.numu=0;
            }
            else if(type==JSONType::floats){
                numf=from.numf;
                from.numi=0;
            }
            else if(type==JSONType::null||type==JSONType::unset){

            }
            else if(type==JSONType::string){
                value=std::move(from.value);
            }
            from.type=JSONType::unset;
            return *this;
        }

        JSON& copy_assign(const JSON& from){
            this->type=from.type;
            if(type==JSONType::object){
                *this=JSON("",type);
                auto& m=*from.obj;
                for(auto& i:m){
                    obj->operator[](i.first)=i.second;
                }
            }
            else if(type==JSONType::array){
                *this=JSON("",type);
                auto& m=*from.array;
                for(auto& i:m){
                    array->push_back(i);
                }
            }
            else if(type==JSONType::boolean){
                boolean=from.boolean;
            }
            else if(type==JSONType::null||type==JSONType::unset){

            }
            else if(type==JSONType::integer){
                numi=from.numi;
            }
            else if(type==JSONType::unsignedi){
                numu=from.numu;
            }
            else if(type==JSONType::floats){
                numf=from.numf;
            }
            else if(type==JSONType::string){
                value=EasyStr(from.value.const_str(),from.value.size());
            }
            return *this;
        }


        template<class Buf>
        JSON parse_json_detail(Reader<Buf>& reader){
            const char* expected=nullptr;
            if(reader.expect("{")){
                JSON ret("",JSONType::object);
                while(!reader.expect("}")){
                    std::string key;
                    if(!reader.ahead("\""))throw "expect \" but not";
                    if(!reader.string(key,true))throw "unreadable key";
                    if(!reader.expect(":"))throw "expect : but not";
                    key.pop_back();
                    key.erase(0,1);
                    auto tmp=parse_json_detail(reader);
                    ret[key]=tmp;
                    if(!reader.expect(",")&&!reader.ahead("}"))throw "expect , or } but not";
                }
                return ret;
            }
            else if(reader.expect("[")){
                JSON ret("",JSONType::array);
                while(!reader.expect("]")){
                    auto tmp=parse_json_detail(reader);
                    ret.push_back(std::move(tmp));
                    if(!reader.expect(",")&&!reader.ahead("]"))throw "expect , or ] but not";
                }
                return ret;
            }
            else if(reader.ahead("\"")){
                std::string str;
                if(!reader.string(str,true))throw "unreadable key";
                str.pop_back();
                str.erase(0,1);
                return JSON(str,JSONType::string);
            }
            else if(reader.achar()=='-'||is_digit(reader.achar())){
                bool minus=false;
                if(reader.expect("-")){
                    minus=true;
                }
                std::string num;
                NumberContext<char> ctx;
                reader.readwhile(num,number,&ctx);
                if(ctx.failed)throw "undecodable number";
                if(ctx.floatf){
                    auto n=std::stod(num);
                    if(minus){
                        n=-n;
                    }
                    return JSON(n);
                }
                else{
                    try{
                        auto n=std::stoull(num);
                        if(n&0x80'00'00'00'00'00'00'00){
                            if(minus){
                                auto f=std::stod(num);
                                f=-f;
                                return JSON(f);
                            }
                            else{
                                return JSON(n);
                            }
                        }
                        auto i=(int64_t)n;
                        if(minus){
                            i=-i;
                        }
                        return JSON(i);
                    }
                    catch(...){
                        auto f=std::stod(num);
                        if(minus){
                            f=-f;
                        }
                        return JSON(f);
                    }
                }
            }
            else if(reader.expectp("true",expected,is_c_id_usable)||
                    reader.expectp("false",expected,is_c_id_usable)){
                return JSON(expected,JSONType::boolean);
            }
            else if(reader.expect("null",is_c_id_usable)){
                return JSON(nullptr);
            }
            throw "not json";
        }

        std::string to_string_detail(JSONFormat format,size_t ofs,size_t indent,size_t base_skip) const{
            auto flag=[&format](auto n){
                return (bool)((unsigned int)format&(unsigned int)n);
            };
            if(type==JSONType::unset)
                return "";
            std::string ret;
            bool first=true;
            auto fmtprint=[&flag,&base_skip,&ret,&indent](size_t ofs){
                if((indent==0)&&!flag(JSONFormat::mustline))return;
                ret+="\n";
                if(flag(JSONFormat::tab)){
                    for(auto i=0;i<ofs;i++)for(auto u=0;u<indent;u++)ret+="\t";
                }
                else if(flag(JSONFormat::space)){
                    for(auto i=0;i<ofs;i++)for(auto u=0;u<indent;u++)ret+=" ";
                }
                if(flag(JSONFormat::elmpush))for(auto i=0;i<base_skip;i++)ret+=" ";
            };
            if(type==JSONType::object){
                ret+="{";
                for(auto& kv:*obj){
                    if(first){
                        first=false;
                    }
                    else{
                        ret+=",";
                    }
                    fmtprint(ofs);
                    auto adds="\""+kv.first+"\""+":";
                    ret+=adds;
                    if((indent&&flag(JSONFormat::afterspace))||flag(JSONFormat::mustspace)){
                        ret+=" ";
                    }
                    auto tmp=kv.second.to_string_detail(format,ofs+1,indent,base_skip+adds.size()+1);
                    if(tmp=="")return "";
                    ret+=tmp;
                }
                if(obj->size())fmtprint(ofs-1);
                ret+="}";
            }
            else if(type==JSONType::array){
                ret+="[";
                for(auto& kv:*array){
                    if(first){
                        first=false;
                    }
                    else{
                        ret+=",";
                    }
                    fmtprint(ofs);
                    auto tmp=kv.to_string_detail(format,ofs+1,indent,base_skip);
                    if(tmp=="")return "";
                    ret+=tmp;
                }
                if(array->size())fmtprint(ofs-1);
                ret+="]";
            }
            else if(type==JSONType::null){
                ret="null";
            }
            else if(type==JSONType::boolean){
                ret=boolean?"true":"false";
            }
            else if(type==JSONType::string){
                ret="\"";
                ret.append(value.const_str(),value.size());
                ret+="\"";
            }
            else if(type==JSONType::integer){
                ret=std::to_string(numi);
            }
            else if(type==JSONType::unsignedi){
                ret=std::to_string(numu);
            }
            else if(type==JSONType::floats){
                ret=std::to_string(numf);
            }
            return ret;
        }

        std::string escape(const std::string& base){
            std::string s;
            for(auto c:base){
                if(c=='\\'){
                    s+="\\\\";
                }
                else if(c=='"'){
                    s+="\\\"";
                }
                else if(c=='\n'){
                    s+="\\n";
                }
                else if(c=='\r'){
                    s+="\\r";
                }
                else if(c=='\t'){
                    s+="\\t";
                }
                else if(c<0x20||c>0x7E){
                    auto msb=(c&0xf0)>>4;
                    auto lsb=c&0x0f;
                    auto translate=[](unsigned char c)->unsigned char{
                        if(c>15)return 0;
                        if(c<10){
                            return c+'0';
                        }
                        else{
                            return c-10+'a';
                        }
                    };
                    s+="\\u00";
                    s+=translate(msb);
                    s+=translate(lsb);
                }
                else{
                    s+=c;
                }
            }
            return s;
        }

        template<class Struct>
        auto to_json_detail(JSON& to,const Struct& in)
        ->decltype(to_json(to,in)){
            to_json(to,in);
        }

        template<class Struct>
        auto to_json_detail(JSON& to,const Struct& in) 
        ->decltype(in.to_json(to)) {
            in.to_json(to);
        }


        template<class Vec>
        auto to_json_detail(JSON& to,const Vec& vec)
        ->decltype(to_json_detail(to,vec[0])){
            JSON js;
            for(auto& o:vec){
                js.push_back(JSON(o));
            }
            to=std::move(js);
        }

        template<class Map>
        auto to_json_detail(JSON& to,const Map& m)
        ->decltype(to_json_detail(to,m[std::string()])){
            JSON js;
            for(auto& o:m){
                js[o.first]=JSON(o.second);
            }
            to=std::move(js);
        }
        

        

        void obj_check(bool exist,const std::string& in)const{
            if(type!=JSONType::object)throw "object kind miss match.";
            if(exist&&!obj->count(in))throw "invalid range";
        }

        void array_check(bool check,size_t pos)const{
            if(type!=JSONType::array)
                throw "object kind miss match.";
            if(check&&array->size()<=pos)
                throw "invalid index";
        }

        void array_if_unset(){
            if(type==JSONType::unset){
                *this=JSONArrayType{};
            }
        }

        void obj_if_unset(){
            if(type==JSONType::unset){
                *this=JSONObjectType{};
            }
        }

    public:

        JSON& operator=(const JSON& from){
            if(this==&from)return *this;
            destruct();
            return copy_assign(from);
        }

        JSON& operator=(JSON&& from) noexcept {
            if(this==&from)return *this;
            destruct();
            return move_assign(std::move(from));
        }

        JSON(){
            type=JSONType::unset;
        }

        JSON(const JSON& from){
            copy_assign(from);
        }

        JSON(JSON&& from) noexcept{
             move_assign(std::move(from));
        }

        JSON(const std::string& in,JSONType type){
            if(type==JSONType::object){
                init_as_obj({});
            }
            else if(type==JSONType::array){
                init_as_array({});
            }
            else if(type==JSONType::boolean){
                if(in=="true"){
                    boolean=true;
                }
                else if(in=="false"){
                    boolean=false;
                }
                else{
                    type=JSONType::unset;
                    return;
                }
            }
            else if(type==JSONType::null){

            }
            else if(type==JSONType::integer){
                try{
                    numi=std::stoll(in);
                }
                catch(...){
                    type=JSONType::unset;
                    return;
                }
            }
            else if(type==JSONType::unsignedi){
                try{
                    numu=std::stoull(in);
                }
                catch(...){
                    type=JSONType::unset;
                    return;
                }
            }
            else if(type==JSONType::floats){
                try{
                    numi=std::stod(in);
                }
                catch(...){
                    type=JSONType::unset;
                    return;
                }
            }
            else if(type==JSONType::string){
                value=EasyStr(in.c_str(),in.size());
            }
            this->type=type;
        }

        JSON(std::nullptr_t){
            type=JSONType::null;
        }

        JSON(bool b){
            type=JSONType::boolean;
            boolean=b;
        }

        JSON(char num){init_as_num(num);}
        JSON(unsigned char num){init_as_num(num);}
        JSON(short num){init_as_num(num);}
        JSON(unsigned short num){init_as_num(num);}
        JSON(int num){init_as_num(num);}
        JSON(unsigned int num){init_as_num(num);}
        JSON(long num){init_as_num(num);}
        JSON(unsigned long num){init_as_num(num);}
        JSON(long long num){init_as_num(num);}
        JSON(unsigned long long num){
            type=JSONType::unsignedi;
            numu=num;
        }

        template<class Struct>
        JSON(const Struct& in){
            try{
                to_json_detail(*this,in);
            }
            catch(...){
                destruct();
                type=JSONType::unset;
            }
        }


        template<class Struct>
        JSON(Struct* in){
            if(!in){
                type=JSONType::null;
            }
            else{
                try{
                    to_json_detail(*this,*in);
                }
                catch(...){
                    destruct();
                    type=JSONType::unset;
                } 
            }
        }

        
        JSON(double num){
            type=JSONType::floats;
            numf=num;
        }

        JSON(const std::string& in){
            type=JSONType::string;
            auto str=escape(in);
            value=EasyStr(str.c_str(),str.size());
        }

        JSON(const char* in){
            if(!in){
                type=JSONType::null;
            }
            else{
                type=JSONType::string;
                auto str=escape(in);
                value=EasyStr(str.c_str());
            }
        }

        JSON(JSONObjectType&& json){
            init_as_obj(std::move(json));   
        }

        JSON(JSONArrayType&& json){
            init_as_array(std::move(json));
        }

        

        JSON& operator[](size_t pos){
            array_if_unset();
            array_check(true,pos);
            return array->operator[](pos);
        }
        
        JSON& operator[](const std::string& key){
            obj_if_unset();
            obj_check(false,key);
            return obj->operator[](key);
        }

        template<class Struct>
        JSON& not_null_assign(const std::string& name,Struct* in){
            if(!in)return *this;
            (*this)[name]=in;
            return *this;
        }


        JSON& at(const std::string& key)const{
            obj_check(true,key);
            return obj->at(key);
        }

        JSON& at(size_t pos)const{
            array_check(true,pos);
            return array->at(pos);
        }

        JSON* idx(size_t pos)const{
            //array_if_unset();
            try{
                array_check(true,pos);
            }
            catch(...){
                return nullptr;
            }
            return &array->at(pos);
        }

        JSON* idx(const std::string& key)const{
            //obj_if_unset();
            try{
                obj_check(true,key);
            }
            catch(...){
                return nullptr;
            }
            return &obj->at(key);
        }

        bool push_back(JSON&& json){
            array_if_unset();
            array_check(false,0);
            array->push_back(std::move(json));
            return true;
        }

        size_t size(){
            if(type!=JSONType::array)return 0;
            return array->size();
        }

        JSONType gettype()const{return type;}

        ~JSON(){
            destruct();
        }
        
        bool parse_assign(const std::string& in){
            Reader<const char*> reader(in.c_str(),ignore_space_line);
            try{    
                *this=parse_json_detail(reader);
            }   
            catch(...){
                return false;
            }
            return true;
        }

        template<class Buf>
        bool parse_assign(Reader<Buf>& r){
            auto ig=r.set_ignore(ignore_space_line);
            try{    
                *this=parse_json_detail(r);
            }   
            catch(...){
                r.set_ignore(ig);
                return false;
            }
            r.set_ignore(ig);
            return true;
        }

        std::string to_string(size_t indent=0,JSONFormat format=JSONFormat::defaultf) const{
            auto ret=to_string_detail(format,1,indent,0);
            auto flag=[&format](auto n){
                return (bool)((unsigned int)format&(unsigned int)n);
            };
            if(ret.size()&&flag(JSONFormat::endline))ret+="\n";
            return ret;
        }

        bool operator==(const JSON& right){
            if(this->type==right.type){
                if(this->type==JSONType::unset||this->type==JSONType::null){
                    return true;
                }
                else if(this->type==JSONType::string){
                    return this->value.size()==right.value.size()&&
                    strcmp(this->value.const_str(),right.value.const_str())==0;
                }
                else if(this->type==JSONType::integer){
                    return this->numi==right.numi;
                }
                else if(this->type==JSONType::floats){
                    return this->numf==right.numf;
                }
                else if(this->type==JSONType::unsignedi){
                    return this->numu==right.numu;
                }
                else if(this->type==JSONType::boolean){
                    return this->boolean==right.boolean;
                }
            }
            return this->to_string()==right.to_string();
        }

        JSON* path(const std::string& p){
            Reader<const char*> reader(p.c_str());
            bool filepath=false;
            if(reader.ahead("/")){
                filepath=true;
            }
            JSON* ret=this,*hold;
            while(!reader.ceof()){
                bool arrayf=false;
                if(filepath){
                    if(!reader.expect("/"))return nullptr;
                    if(reader.ceof())break;
                }
                else if(reader.expect("[")){
                    arrayf=true;
                }
                else if(!reader.expect(".")){
                    return nullptr;
                }
                std::string key;
                auto path_get=[&](auto& key){
                    hold=ret->idx(key);
                    if(!hold)return false;
                    ret=hold;
                    return true;
                };
                auto read_until_cut=[&](){
                    if(!reader.readwhile(key,untilincondition,[&filepath,&arrayf] (char c){
                        if(filepath){
                            if(c=='/')return false;
                        }
                        else {
                            if(c=='.'||c=='[')return false;
                        }
                        return true;
                    },true))return false;
                    return path_get(key);
                };

                if(reader.ahead("\"")||reader.ahead("'")){
                    if(!reader.string(key,true))return nullptr;
                    key.pop_back();
                    key.erase(0,1);
                    if(!path_get(key))return nullptr;
                }
                else if(is_digit(reader.achar())){
                    auto beginpos=reader.readpos();
                    if(!reader.readwhile(key,untilincondition,is_digit<char>))return nullptr;
                    if(key=="")return nullptr;
                    bool ok=false;
                    if(filepath){
                        if(!reader.ahead("/")&&!reader.ceof()){
                            reader.seek(beginpos);
                            key="";
                            if(!read_until_cut())
                                return nullptr;
                            ok=true;
                        }
                    }
                    if(!ok){
                        uint64_t pos=0;
                        try{
                            pos=std::stoull(key);
                        }
                        catch(...){
                            if(!path_get(key))return nullptr;
                            ok=true;
                        }
                        if(!ok){
                            hold=ret->idx(pos);
                            if(!hold){
                                if(!filepath||!path_get(key)){
                                    return nullptr;
                                }
                            }
                            else{
                                ret=hold;
                            }
                        }
                    }
                }
                else if(!arrayf){
                    if(!read_until_cut())return nullptr;
                }
                else{
                    return nullptr;
                }
                if(arrayf){
                    if(!reader.expect("]"))return nullptr;
                }
            }
            return ret;
        }

    private:
        template<class T>
        T signed_num_get()const {
            if(type!=JSONType::integer&&type!=JSONType::unsignedi){
                throw "object type missmatch";
            }
            auto check=[this](int64_t s,int64_t l){
                if(numi<s||numi>l){
                    throw "out of range";
                }
                return (T)numi;
            };
            switch(sizeof(T)){
            case 1:return check((signed char)0x80,0x7f);
            case 2:return check((signed short)0x8000,0x7fff);
            case 4:return check((signed int)0x80000000,0x7fffffff);
            case 8:{
                if(type==JSONType::unsignedi&& numi<0){
                    throw "out of range";
                }
                return (T)numi;
            }
            default:
                throw "too large number size";
            }
            throw "unreachable";
        }
        template<class T>
        T unsigned_num_get()const{
            if(type!=JSONType::integer&&type!=JSONType::unsignedi){
                throw "object type missmatch";
            }
            if(type==JSONType::integer&&numi<0){
                throw "out of range";
            }
            auto check=[this](uint64_t c){
                if(numu>c){
                    throw "out of range";
                }
                return (T)numu;
            };
            switch (sizeof(T))
            {
            case 1:return check(0xff);
            case 2:return check(0xffff);
            case 4:return check(0xffffffff);
            case 8:return numu;
            default:
                throw "too large number size";
            }
            throw "unreachable";
        }

        template<class T>
        T float_num_get() const{
            if(type==JSONType::floats){
                return (T)numf;
            }
            else if(type==JSONType::integer){
                return (T)numi;
            }
            else if(type==JSONType::unsignedi){
                return (T)numu;
            }
            throw "object type missmatch";
        }
    public:

        template<class Struct>
        Struct get()const{
            Struct tmp=Struct();
            get_to(tmp);
            return tmp;
        }


        template<class Struct,class Allocator=Struct*(*)(),class Deleter=void(*)(Struct*)>
        Struct* get_ptr(Allocator&& alloc=[]{return new Struct();},
                        Deleter&& del=[](Struct* p){delete p;})const{
            if(type==JSONType::null){
                return nullptr;
            }
            Struct* s=nullptr;
            s=alloc();
            if(!s){
                throw "memory error";
            }
            try{
                get_to(*s);
            }
            catch(...){
                del(s);
                throw;
            }
            return s;
        }

        template<class Struct>
        bool try_get(Struct& s)const{
            try{
                get_to(s);
            }
            catch(...){
                return false;
            }
            return true;
        }

        template<class Struct,class Allocator=Struct*(*)(),class Deleter=void(*)(Struct*)>
        bool try_get_ptr(Struct*& s,Allocator&& alloc=[]{return new Struct();},
                        Deleter&& del=[](Struct* p){delete p;})const{
            try{
                s=get_ptr<Struct>(alloc,del);
            }
            catch(...){
                return false;
            }
            return true;
        }

        template<class Struct>
        void get_to(Struct& s)const{
            from_json_detail(s,*this);
        }

        bool is_enable(){
            return type!=JSONType::unset;
        }

    private:
#define COMMONLIB_JSON_FROM_JSON_DETAIL_SIG(x) \
        void from_json_detail(x& s,const JSON& j)const{s=j.signed_num_get<x>();}
#define COMMONLIB_JSON_FROM_JSON_DETAIL_UNSIG(x) \
        void from_json_detail(unsigned x& s,const JSON& j)const{s=j.unsigned_num_get<unsigned x>();}


        COMMONLIB_JSON_FROM_JSON_DETAIL_SIG(char)
        COMMONLIB_JSON_FROM_JSON_DETAIL_SIG(short)
        COMMONLIB_JSON_FROM_JSON_DETAIL_SIG(int)
        COMMONLIB_JSON_FROM_JSON_DETAIL_SIG(long)
        COMMONLIB_JSON_FROM_JSON_DETAIL_SIG(long long)

        COMMONLIB_JSON_FROM_JSON_DETAIL_UNSIG(char)
        COMMONLIB_JSON_FROM_JSON_DETAIL_UNSIG(short)
        COMMONLIB_JSON_FROM_JSON_DETAIL_UNSIG(int)
        COMMONLIB_JSON_FROM_JSON_DETAIL_UNSIG(long)
        COMMONLIB_JSON_FROM_JSON_DETAIL_UNSIG(long long)


        void from_json_detail(float& s,const JSON& in)const{
            s=in.float_num_get<float>();
        }

        void from_json_detail(double& s,const JSON& in)const{
            s=in.float_num_get<double>();
        }

         
        void from_json_detail(bool& s,const JSON& in)const{
            if(in.type!=JSONType::boolean){
                throw "object type missmatch";
            }
            s=in.boolean;
        }
        
         
        void from_json_detail(std::string& s,const JSON& in)const{
            if(in.type!=JSONType::string){
                throw "object type missmatch";
            }
            s=std::string(value.const_str(),value.size());
        }

         
        void from_json_detail(const char*& s,const JSON& in)const{
            if(in.type!=JSONType::string){
                throw "object type missmacth";
            }
            s=value.const_str();
        }
        
        template<class Struct>
        auto from_json_detail(Struct& to,const JSON& in)const
        ->decltype(from_json(to,in)){
            from_json(to,in);
        }

        template<class Struct>
        auto from_json_detail(Struct& to,const JSON& in)const
        ->decltype(to.from_json(in)){
            to.from_json(in);
        }

        template<class Vec>
        auto from_json_detail(Vec& to,const JSON& in)const
        ->decltype(from_json_detail(to[0],in)){
            if(in.type!=JSONType::array){
                throw "object kind missmatch";
            }
            for(auto& o:*in.array){
                to.push_back(o.get<remove_cv_ref<decltype(to[0])>>());
            }
        }

        template<class Map>
        auto from_json_detail(Map& to,const JSON& in)const
        ->decltype(from_json_detail(to[std::string()],in)){
            if(in.type!=JSONType::object){
                throw "object kind missmatch";
            }
            for(auto& o:*in.obj){
                to[o.first]=o.second.get<remove_cv_ref<decltype(to[std::string()])>>();
            }
        }
    };

    inline JSON operator""_json(const char* in,size_t size){   
        JSON js;
        js.parse_assign(std::string(in,size));
        return js;
    }
        
    using JSONObject=JSON::JSONObjectType;
    using JSONArray=JSON::JSONArrayType;

    template<class Vec>
    JSON atoj(Vec& in){
        JSON ret="[]"_json;
        for(auto& i:in){
            ret.push_back(i);
        }
        return ret;
    }
}