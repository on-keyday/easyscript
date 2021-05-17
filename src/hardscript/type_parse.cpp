#include"type_parse.h"
#include"control.h"

using namespace PROJECT_NAME;
using namespace control;

std::string control::parse_type(PROJECT_NAME::Reader<std::string>& reader,bool strict){
    std::string name;
    auto result=[&](auto& add)->std::string{
        return (name=parse_type(reader,true)).size()==0?"":add+name;
    };
    if(reader.expect("const",is_c_id_usable)){
        return result("const ");
    }
    else if(reader.expect("*")){
        return result("*");
    }
    else if(reader.expect("&")){
        return result("&");
    }
    else if(reader.expect("[")){
        if(!reader.readwhile(name,until,']')||!reader.expect("]")){
            return "";
        }
        auto tmp="["+name+"]";
        return result(tmp);
    }
    else if(reader.expect("(")){
        std::vector<std::string> _,types;
        std::string ret;
        if(!parse_funcarg(reader,_,types,ret,strict)){
            return "";
        }
        return typevec_to_type(types,ret);
    }
    else if(is_c_id_top_usable(reader.achar())){
        reader.readwhile(name,untilincondition,is_c_id_usable<char>);
        const char* expected=nullptr;
        while(reader.expectp(".",expected,[](char c){return c=='.';})||reader.expectp("::",expected)){
            if(!is_c_id_top_usable(reader.achar()))return "";
            std::string tmp;
            reader.readwhile(tmp,untilincondition,is_c_id_usable<char>);
            if(tmp=="")return "";
            name+=expected+tmp;
        }
        if(reader.expect("...")){
            name+="...";
        }
        return name;
    }
    return "";
}

bool control::parse_funcarg(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,std::string& ret,bool strict){
    size_t count=0;
    while(!reader.expect(")")){
        std::string argname;
        std::string typenames;
        if(strict){
            if(!parse_arg_need_type(reader,argname,typenames)){
                return false;
            }
        }
        else if(!parse_arg_need_name(reader,argname,typenames)){
            return false;
        }
        if(typenames[0]=='@'){
            if(std::to_string(count)!=&typenames.c_str()[1]){
                return false;
            }
        }
        if(argname!=""&&reader.expect("=")){
            argname+="=";
            auto beginpos=reader.readpos();
            auto check=expr_parse(reader);
            if(!check)return false;
            delete check;
            auto endpos=reader.readpos();
            reader.seek(beginpos);
            for(size_t i=0;i!=endpos;i++){
                argname.push_back(reader.achar());
                reader.increment();
            }
        }
        if(argname==""){
            argname="$"+std::to_string(count);
        }
        if(typenames==""){
            typenames="@"+std::to_string(count);
        }
        arg.push_back(argname);
        type.push_back(typenames);
        count++;
        if(!reader.expect(",")&&!reader.ahead(")")){
            return false;
        }
    }
    if(reader.expect("->")){
        ret=parse_type(reader,true);
        if(ret=="")return false;
    }
    return true;
}

bool control::parse_arg_need_type(Reader<std::string>& reader,std::string& name,std::string& type){
    if(is_c_id_top_usable(reader.achar())){
        auto beginpos=reader.readpos();
        reader.readwhile(name,untilincondition,is_c_id_usable<char>);
        if(name=="const"||reader.ahead(".")||reader.ahead(",")||reader.ahead(")")){
            reader.seek(beginpos);
            name="";
        }
    }
    if(reader.expect("@")){
        type="@";
        reader.readwhile(type,untilincondition,is_digit<char>);
    }
    else{
        if(reader.expect("...")){
            type="...";
        }
        else{
            type=parse_type(reader,false);
            return type!="";
        }
    }
    return true;
}

bool control::parse_arg_need_name(Reader<std::string>& reader,std::string& name,std::string& type){
    reader.readwhile(name,untilincondition,is_c_id_usable<char>);
    if(name==""||name=="const"||reader.ahead(".",[](char c){return c=='.';})){
        return false;
    }
    if(!reader.ahead(",")&&!reader.ahead(")")){
        if(reader.expect("...")){
            type="...";
        }
        else{
            type=parse_type(reader,false);
            return type!="";
        }
    }
    return true;
}

std::string control::typevec_to_type(std::vector<std::string>& types,std::string& ret){
    bool first=true;
    std::string name="(";
    size_t num=0;
    for(auto& s:types){
        if(first){
            first=false;
        }
        else{
            name+=",";
        }
        if(s==""){
            name+="@"+std::to_string(num);
            num++;
        }
        else{
            name+=s;
        }
    }
    name+=")";
    if(ret!=""){
        name+="->"+ret;
    }
    return name;
}