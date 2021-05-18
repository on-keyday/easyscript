/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include"control.h"
#include"type_parse.h"

using namespace PROJECT_NAME;
using namespace control;

bool control::parse_all(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec){
    while(!reader.eof()){
        if(!control_parse(reader,vec))return false;
    }
    return true;
}

bool control::check_initexpr(PROJECT_NAME::Reader<std::string>& reader){
    if(is_c_id_top_usable(reader.achar())){
        auto beginpos=reader.readpos();
        do{
            if(!is_c_id_top_usable(reader.achar()))break;
            reader.readwhile(untilincondition_nobuf,is_c_id_usable<char>);
            if(reader.expect(":=")){
                reader.seek(beginpos);
                return true;
            }
        }while(reader.expect(","));
        reader.seek(beginpos);
    }
    return false;
}

bool control::control_parse(Reader<std::string>& reader,std::vector<Control>& vec){
    const char* expected=nullptr;
    if(reader.expect("if",is_c_id_usable)){
        return parse_if(reader,vec);
    }
    else if(reader.expect("for",is_c_id_usable)){
        return parse_for(reader,vec);
    }
    else if(reader.expect("switch",is_c_id_usable)){
        return false;
    }
    else if(reader.expect("select",is_c_id_usable)){
        return false;
    }
    else if(reader.expectp("continue",expected,is_c_id_usable)||
            reader.expectp("break",expected,is_c_id_usable)){
        Control ret;
        if(!reader.expect(";"))return false;
        ret.name=expected;
        vec.push_back(std::move(ret));
    }
    else if(reader.expect("func",is_c_id_usable)){
        return parse_func(reader,vec);
    }
    else if(reader.expect("decl",is_c_id_usable)){
        return parse_func(reader,vec,true);
    }
    else if(reader.expect("var",is_c_id_usable)){
        return parse_var(reader,vec);
    }
    else if(reader.expect("return",is_c_id_usable)){
        Control ret;
        ret.name="return";
        if(!reader.ahead(";")){
            if(!(ret.expr=expr_parse(reader)));
        }
        if(!reader.expect(";"))return false;
        vec.push_back(std::move(ret));
    }
    else if(check_initexpr(reader)){
         return parse_var_detail(reader,vec,false,true)&&reader.expect(";");
    }
    else{
        return parse_expr(reader,vec,true);
    }
    return true;
}

bool control::parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf,size_t& pos){
    pos=reader.readpos();
    return reader.readwhile(buf,depthblock_withbuf,BasicBlock<std::string>(true,false,false),true);
}

bool control::parse_expr(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,bool semicolon){
    Control ret;
    auto tree=expr_parse(reader);
    if(!tree||(semicolon&&!reader.expect(";"))){
        delete tree;
        return false;
    }
    ret.expr=tree;
    ret.kind=CtrlKind::expr;
    vec.push_back(std::move(ret));
    return true;
}



bool control::parse_var(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec){
    const char* bracket=nullptr;
    if(reader.expect("(")){
        bracket=")";
    }
    return parse_var_detail(reader,vec,bracket,false)&&bracket?true:reader.expect(";");
}

bool control::parse_var_detail(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,const char* bracket,bool initeq){
    Control hold;
    bool check=false;
    do{
        std::string type;
        std::vector<std::string> varname;
        std::vector<Tree*> inits;
        do{
            std::string name;
            if(!is_c_id_top_usable(reader.achar()))return false;
            reader.readwhile(name,untilincondition,is_c_id_usable<char>);
            if(name=="")return false;
            varname.push_back(std::move(name));
        }while(reader.expect(","));
        check=initeq?reader.ahead(":="):reader.ahead("=");
        if(!check){
            if((type=parse_type(reader,true))=="")return false;
        }
        check=initeq?reader.expect(":="):reader.expect("=");
        if(check){
            bool failed=false;
            do{
                auto tmp=expr_parse(reader);
                if(!tmp){
                    failed=true;
                    break;
                }
                inits.push_back(tmp);
            }while(reader.expect(","));
            if(varname.size()<inits.size())failed=true;
            if(failed){
                for(auto&i:inits){
                    delete i;
                }
                return false;
            }
        }
        hold.kind=CtrlKind::var;
        size_t i=0;
        size_t remain=varname.size()-1;
        for(;i<inits.size();i++){
            hold.type=type;
            hold.expr=inits[i];
            hold.name=std::move(varname[i]);
            hold.inblockpos=remain;
            vec.push_back(std::move(hold));
            remain--;
        }
        for(;i<varname.size();i++){
            hold.type=type;
            hold.name=std::move(varname[i]);
            hold.inblockpos=remain;
            vec.push_back(std::move(hold));
            remain--;
        }
    }while(bracket&&!reader.ahead(bracket));
    if(bracket&&!reader.expect(bracket)){
        return false;
    }
    return true;
}

bool control::parse_if(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec){
    Control ret;
    ret.name="if";
    auto tree=expr_parse(reader);
    if(!tree)return false;
    ret.expr=tree;
    if(!parse_inblock(reader,ret.inblock,ret.inblockpos))return false;
    vec.push_back(std::move(ret));
    while(reader.expect("else",is_c_id_usable)){
        if(reader.expect("if",is_c_id_usable)){
            ret.name="elif";
            tree=expr_parse(reader);
            if(!tree)return false;
            ret.expr=tree;
            if(!parse_inblock(reader,ret.inblock,ret.inblockpos))return false;
            vec.push_back(std::move(ret));
        }
        else{
            ret.name="else";
            if(!parse_inblock(reader,ret.inblock,ret.inblockpos))return false;
            vec.push_back(std::move(ret));
            break;
        }
    }
    return true;
}

bool control::parse_func(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,bool decl){
    Control ret;
    if(!is_c_id_top_usable(reader.achar()))return false;
    reader.readwhile(ret.name,untilincondition,is_c_id_usable<char>);
    if(ret.name=="")return false;
    std::vector<std::string> arg,argtype;
    std::string rettype,opt;
    if(!reader.expect("(")||!parse_funcarg(reader,arg,argtype,rettype,opt,decl))return false;
    ret.arg=std::move(arg);
    ret.argtype=std::move(argtype);
    ret.type=std::move(rettype);
    if(opt!=""){
        ret.argtype.push_back(std::move(opt));
    }
    if(decl){
        if(!reader.expect(";"))return false;
        ret.kind=CtrlKind::decl;
    }
    else{
        if(!parse_inblock(reader,ret.inblock,ret.inblockpos))return false;
        ret.kind=CtrlKind::func;
        
    }
    vec.push_back(std::move(ret));
    return true;
}

bool control::parse_for(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec){
    Control hold;
    if(reader.ahead("{")){
        hold.name="for-block";
        if(!parse_inblock(reader,hold.inblock,hold.inblockpos))return false;
        vec.push_back(std::move(hold));
        return true;
    }
    hold.name="for-begin";
    vec.push_back(std::move(hold));
    size_t beginpos=vec.size()-1;
    if(!reader.ahead(";")){
        if(check_initexpr(reader)){
            if(!parse_var_detail(reader,vec,false,true))return false;
            vec[beginpos].name="for-range";
        }
        else{
            if(!parse_expr(reader,vec,false))return false;
            vec.back().name="first";
            vec[beginpos].name="for-expr";
        }
        if(reader.ahead("{")){
            hold.name="for-end";
            if(!parse_inblock(reader,hold.inblock,hold.inblockpos))return false;
            vec.push_back(std::move(hold));
            return true;
        }
    }
    if(!reader.expect(";"))return false;
    vec[beginpos].name="for-three";
    if(!reader.ahead(";")){
        if(!parse_expr(reader,vec,false))return false;
        vec.back().name="second";
    }
    if(!reader.expect(";"))return false;
    if(!reader.ahead("{")){
        if(!parse_expr(reader,vec,false))return false;
        vec.back().name="third";
    }
    hold.name="for-end";
    if(!parse_inblock(reader,hold.inblock,hold.inblockpos))return false;
    vec.push_back(std::move(hold));
    return true;
}