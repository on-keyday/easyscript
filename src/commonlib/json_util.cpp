#include"json_util.h"
#include"reader_helper.h"
using namespace PROJECT_NAME;

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
            ret.append(std::move(tmp));
            if(!reader.expect(",")&&!reader.ahead("]"))throw "expect , or ] but not";
        }
        return ret;
    }
    else if(reader.ahead("\"")){
        std::string str;
        if(!reader.string(str,true))throw "unreadable key";
        str.pop_back();
        str.erase(0,1);
        return JSON(str);
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
            return JSON(num,JSONType::floats);
        }
        else{
            auto n=std::stoll(num);
            if(minus){
                n=-n;
            }
            return JSON(n);
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

std::string JSON::to_string(bool format,size_t indent,bool useskip){
    auto ret=to_string_detail(format,1,indent,0,useskip);
    if(ret.size())ret+="\n";
    return ret;
}

std::string JSON::to_string_detail(bool format,size_t ofs,size_t indent,size_t base_skip,bool useskip){
    if(type==JSONType::unset)return "";
    std::string ret;
    bool first=true;
    auto fmtprint=[format,&useskip,&base_skip,&ret,&indent](size_t ofs,bool line=true){
        if(!format)return;
        if(line)ret+="\n";
        for(auto i=0;i<ofs;i++)for(auto u=0;u<indent;u++)ret+=" ";
        if(useskip)for(auto i=0;i<base_skip;i++)ret+=" ";
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
            auto tmp=kv.second.to_string_detail(format,ofs+1,indent,base_skip+adds.size(),useskip);
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
            auto tmp=kv.to_string_detail(format,ofs+1,indent,base_skip,useskip);
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
    else {
        ret.append(value.const_str(),value.size());
    }
    return ret;
}