/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include"control.h"

using namespace PROJECT_NAME;
using namespace control;

Control* control::control_parse(Reader<std::string>& reader){
    Control* ret=nullptr;
    const char* expected=nullptr;
    if(reader.expect("if",expected,is_c_id_usable)||
       reader.expect("while",expected,is_c_id_usable)){
        auto tree=expr_parse(reader);
        if(!tree)return nullptr;
        ret=make<Control>(expected,tree);
        if(!ret){
            delete tree;
            return nullptr;
        }
        if(!parse_inblock(reader,ret->inblock,ret->inblockpos)){
            delete ret;
            return nullptr;
        }
    }
    else if(reader.expect("for",is_c_id_usable)){

    }
    else if(reader.expect("switch",is_c_id_usable)){

    }
    else if(reader.expect("select",is_c_id_usable)){

    }
    else if(reader.expect("continue",expected,is_c_id_usable)||
            reader.expect("break",expected,is_c_id_usable)){
        if(!reader.expect(";"))return nullptr;
        ret=make<Control>(expected);
        if(!ret)return nullptr;
    }
    else if(reader.expect("func",is_c_id_usable)){
        if(!is_c_id_top_usable(reader.achar()))return nullptr;
        std::string name;
        reader.readwhile(name,untilincondition,is_c_id_usable<char>);
        if(name=="")return nullptr;
        std::vector<std::string> arg,argtype;
        std::string rettype;
        if(!parse_function(reader,arg,argtype,rettype)){
            return nullptr;
        }
        ret=make<Control>(name,nullptr,CtrlKind::func);
        if(!ret)return nullptr;
        ret->arg=std::move(arg);
        ret->argtype=std::move(argtype);
        ret->type=std::move(rettype);
        if(!parse_inblock(reader,ret->inblock,ret->inblockpos)){
            delete ret;
            return nullptr;
        }
    }
    else if(reader.expect("var",is_c_id_usable)){
        
    }
    else{
        auto tree=expr_parse(reader);
        if(!tree||!reader.expect(";")){
            delete tree;
            return nullptr;
        }
        ret=make<Control>("",tree,CtrlKind::expr);
        if(!ret){
            delete tree;
            return nullptr;
        }
    }
    return ret;
}

bool control::parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf,size_t& pos){
    pos=reader.readpos();
    return reader.readwhile(buf,depthblock_withbuf,BasicBlock<std::string>(true,false,false));
}

std::string control::parse_type(PROJECT_NAME::Reader<std::string>& reader,bool strict){
    std::string name;
    auto result=[&](auto& add)->std::string{
        return (name=parse_type(reader)).size()==0?"":add+name;
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
        return "["+name+"]";
    }
    else if(reader.expect("(")){
        std::vector<std::string> _,types;
        std::string ret;
        if(!parse_function(reader,_,types,ret,strict)){
            return "";
        }
        bool first=true;
        name+="(";
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
    else if(is_c_id_top_usable(reader.achar())){
        reader.readwhile(name,untilincondition,is_c_id_usable<char>);
        while(reader.expect(".")){
            if(!is_c_id_top_usable(reader.achar()))return "";
            std::string tmp;
            reader.readwhile(tmp,untilincondition,is_c_id_usable<char>);
            if(tmp=="")return "";
            name+="."+tmp;
        }
        return name;
    }
    return "";
}

bool control::parse_function(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,std::string& ret,bool strict){
    while(!reader.expect(")")){
        std::string argname;
        std::string typenames;
        if(strict){
            if(is_c_id_top_usable(reader.achar())){
                auto beginpos=reader.readpos();
                reader.readwhile(argname,untilincondition,is_c_id_usable<char>);
                if(argname=="const"||reader.ahead(".")||reader.ahead(",")||reader.ahead(")")){
                    reader.seek(beginpos);
                    argname="";
                }
            }
            typenames=parse_type(reader,strict);
            if(typenames=="")return false;
        }
        else{
            reader.readwhile(argname,untilincondition,is_c_id_usable<char>);
            if(argname==""||argname=="const"||reader.ahead(".")){
                return false;
            }
            if(!reader.ahead(",")&&!reader.ahead(")")){
                typenames=parse_type(reader,strict);
                if(typenames=="")return false;
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
        arg.push_back(argname);
        type.push_back(typenames);
        if(!reader.expect(",")&&!reader.ahead(")")){
            return false;
        }
    }
    if(reader.expect("->")){
        ret=parse_type(reader,strict);
        if(ret=="")return false;
    }
    return true;
}