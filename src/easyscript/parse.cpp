#include"parse.h"
using namespace PROJECT_NAME;

bool parse(Reader<std::string>& reader,std::vector<Command>& cmdvec){
    Command cmd;
    size_t res=false;
    bool semicolon=false;   
    bool nopush=false;
    if(reader.expect("func",is_c_id_usable)){
        res=parse_func(cmd,reader);
    }
    /*else if(reader.expect("var",is_c_id_usable)){
        ret=parse_var(cmd,reader,cmds);
        semicolon=true;
    }*/
    else if(reader.expect("delete",is_c_id_usable)){
        res=parse_delete(cmd,reader);
        semicolon=true;
    }
    else if(reader.expect("while",is_c_id_usable)){
        res=parse_while(cmd,reader);
    }
    else if(reader.expect("if",is_c_id_usable)){
        res=parse_if(cmd,reader,cmdvec);
        nopush=true;
    }
    else if(reader.expect("return",is_c_id_usable)){
       res=parse_return(cmd,reader);
       semicolon=true;
    }
    else{
        res=parse_var(cmd,reader);
        semicolon=true;
    }
    if(res){
        if(semicolon&&!reader.expect(";")){
            return false;
        }
        if(!nopush){
            cmdvec.push_back(std::move(cmd));
        }
    }
    return res;
}

bool parse_func(Command& cmd,Reader<std::string>& reader){
    cmd.type=CmdType::func;
    reader.readwhile(cmd.name,untilincondition,is_c_id_usable<char>);
    if(cmd.name=="")return false;
    if(!reader.expect("("))return false;
    while(true){
        if(reader.expect(")"))break;
        std::string arg;
        reader.readwhile(arg,untilincondition,is_c_id_usable<char>);
        if(arg.size()==0)return false;
        cmd.args.push_back(std::move(arg));
        if(!reader.expect(",")&&!reader.ahead(")"))return false;
    }
    if(!parse_unrated(reader,cmd.unrated))return false;
    //cmds.push_back(std::move(cmd));
    return true;
}

bool parse_var(Command& cmd,Reader<std::string>& reader){
    /*cmd.type=CmdType::define;
    /*reader.readwhile(cmd.name,untilincondition,is_c_id_usable<char>);
    if(cmd.name=="")return false;
    if(!reader.expect("="))return false;*/
    /*if(is_c_id_top_usable(reader.achar())){
        auto beginpos=reader.readpos();
        std::string str;
        reader.readwhile(str,untilincondition,is_c_id_usable<char>);
        if(!reader.ahead("==")&&reader.expect("=")){
            cmd.type=CmdType::assign;
            cmd.name=std::move(str);
        }
        else{
            reader.seek(beginpos);
            cmd.type=CmdType::expr;
        }
    }*/
    cmd.type=CmdType::expr;
    Tree* exp=nullptr;
    if(reader.expect("new",is_c_id_usable)){
        auto tmp=parse_expr(reader);
        if(tmp->symbol!="()"||!tmp->right||tmp->right->type!=EvalType::function){
            delete tmp;
            return false;
        }
        exp=make<Tree>("new",EvalType::create,nullptr,tmp);
        if(!exp){
            delete tmp;
            return false;
        }
    }
    else{
        exp=parse_expr(reader);
        if(!exp)return false;
    }
    cmd.expr=exp;
    //cmds.push_back(std::move(cmd));
    return true;
}

bool parse_delete(Command& cmd,Reader<std::string>& reader){
    cmd.type=CmdType::deletes;
    //std::string name;
    reader.readwhile(cmd.name,untilincondition,is_c_id_usable<char>);
    if(cmd.name=="")return false;
    //cmds.push_back(std::move(cmd));
    return true;
}

bool parse_while(Command& cmd,PROJECT_NAME::Reader<std::string>& reader){
    cmd.type=CmdType::ctrl;
    cmd.name="while";
    auto expr=parse_expr(reader);
    if(!expr)return false;
    if(!parse_unrated(reader,cmd.unrated)){
        delete expr;
        return false;
    }
    cmd.expr=expr;
    //cmds.push_back(std::move(cmd));
    return true;
}

size_t parse_if(Command& cmd,PROJECT_NAME::Reader<std::string>& reader,std::vector<Command>& cmds){
    cmd.type=CmdType::ctrl;
    cmd.name="if";
    auto expr=parse_expr(reader);
    if(!expr)return 0;
    if(!parse_unrated(reader,cmd.unrated)){
        delete expr;
        return 0;
    }
    cmd.expr=expr;
    cmds.push_back(std::move(cmd));
    size_t ret=1;
    while(true){
        if(reader.expect("elif",is_c_id_usable)){
            Command tmp;
            tmp.type=CmdType::ctrl;
            tmp.name="elif";
            expr=parse_expr(reader);
            if(!expr)return 0;
            if(!parse_unrated(reader,cmd.unrated)){
                delete expr;
                return 0;
            }
            tmp.expr=expr;
            cmds.push_back(std::move(tmp));
            ret++;
        }
        else if(reader.expect("else",is_c_id_usable)){
            Command tmp;
            tmp.type=CmdType::ctrl;
            tmp.name="else";
            if(!parse_unrated(reader,cmd.unrated)){
                return 0;
            }
            cmds.push_back(std::move(tmp));
            break;
        }
        else{
            break;
        }
    }
    return ret;
}

bool parse_return(Command& cmd,PROJECT_NAME::Reader<std::string>& reader){
    cmd.type=CmdType::returns;
    auto expr=expression(reader);
    if(expr){
        if(eval_tree(expr,nullptr)==EvalType::error){
            delete expr;
            return false;
        }
        cmd.expr=expr;
    }
    //cmds.push_back(std::move(cmd));
    return true;
}

bool parse_unrated(PROJECT_NAME::Reader<std::string>& reader,std::string& str){
    auto beginpos=reader.readpos();
    if(!reader.depthblock())return false;
    auto endpos=reader.readpos();
    reader.seek(beginpos);
    while(reader.readpos()!=endpos){
        str.push_back(reader.achar());
        reader.increment();
    }
    return true;
}

Tree* parse_expr(PROJECT_NAME::Reader<std::string>& reader){
    auto expr=expression(reader);
    if(!expr)return nullptr;
    if(eval_tree(expr,nullptr)==EvalType::error){
        delete expr;
        return nullptr;
    }
    return expr;
}
