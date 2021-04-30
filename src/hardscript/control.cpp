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
        if(!parse_inblock(reader,ret->inblock)){
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

    }
    else if(reader.expect("var",is_c_id_usable)){

    }
    else{
        auto tree=expr_parse(reader);
        if(!tree||!reader.expect(";")){
            delete tree;
            return nullptr;
        }
        ret=make<Control>("",tree);
        if(!ret){
            delete tree;
            return nullptr;
        }
    }
    return ret;
}

bool control::parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf){
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
        if(!parse_function(reader,_,types,strict)){
            return "";
        }
        bool first=true;
        name+="(";
        for(auto& s:types){
            if(first){
                first=false;
            }
            else{
                name+=",";
            }
            if(s==""){
                name+="@";
            }
            else{
                name+=s;
            }
        }
        name+=")";
        if(reader.expect("->")){
            auto tmp=parse_type(reader,strict);
            if(tmp=="")return "";
            name+="->"+tmp;
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

bool control::parse_function(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,bool strict){
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
    return true;
}