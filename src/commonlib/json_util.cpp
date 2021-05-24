#include"json_util.h"
#include"reader_helper.h"
using namespace PROJECT_NAME;

JSON::JSON(const std::string& in,JSONType type){
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

void JSON::destruct(){
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
        numi=0;
    }

}

JSON& JSON::move_assign(JSON&& from){
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

JSON& JSON::copy_assign(const JSON& from){
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

bool JSON::parse_assign(const std::string& in){
    Reader<const char*> reader(in.c_str(),ignore_space_line);
    try{    
        *this=parse_json_detail(reader);
    }   
    catch(...){
        return false;
    }
    return true;
}

JSON JSON::parse_json_detail(Reader<const char*>& reader){
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
        if(!ctx.succeed)throw "undecodable number";
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
                auto i=(long long)n;
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

std::string JSON::to_string(size_t indent,JSONFormat format) const{
    auto ret=to_string_detail(format,1,indent,0);
    auto flag=[&format](auto n){
        return (bool)((unsigned int)format&(unsigned int)n);
    };
    if(ret.size()&&flag(JSONFormat::endline))ret+="\n";
    return ret;
}

std::string JSON::to_string_detail(JSONFormat format,size_t ofs,size_t indent,size_t base_skip)const{
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

std::string JSON::escape(const std::string& base){
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
                return 0;
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

JSON PROJECT_NAME::operator""_json(const char* in,size_t size){
    JSON js;
    js.parse_assign(std::string(in,size));
    return js;
}

JSON* JSON::path(const std::string& p){
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
                unsigned long long pos=0;
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