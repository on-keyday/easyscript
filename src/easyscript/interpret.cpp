#include"interpret.h"
#include"default_operator.h"
#include<cfenv>
#pragma STDC FENV_ACCESS ON
using namespace PROJECT_NAME;
using namespace interpreter;
using default_op::err;

int interpreter::interpret(Reader<std::string>& reader,ValType* err){
    IdTable global;
    global.global=&global;
    auto val=interpret_detail(reader,global);
    auto res=val.second==EvalType::error?-1:0;
    if(err){
        *err=std::move(val);
    }
    return res;
}

ValType interpreter::interpret_detail(PROJECT_NAME::Reader<std::string>& reader,IdTable& table){
    std::vector<Command> cmds; 
    size_t i=0;
    while(!reader.eof()){
        if(reader.ahead("}"))break;
        if(!parse(reader,cmds)){
            return err("parse failed.");
        }
        auto& cmd=cmds[i];
        auto res=interpret_a_cmd(cmd,table,cmds);
        if(res.first!=""||res.second!=EvalType::none){
            return res;
        }
        i++;
    }
    return {"",EvalType::none};
}

ValType interpreter::interpret_a_cmd(Command& cmd,IdTable& table,std::vector<Command>& cmds){
    ValType succeed={"",EvalType::none};
    /*if(cmd.type==CmdType::assign){
        auto val=interpret_tree(cmd.expr,table);
        if(val.second==EvalType::error)return val;
        if(!table.vars.assign(cmd.name,std::move(val))){
            return err("not assignable.");
        }
        return succeed;
    }
    else
    */ 
    if(cmd.type==CmdType::returns){
        if(!cmd.expr)return {"none",EvalType::none};
        auto val=interpret_tree(cmd.expr,table);
        if(val.second==EvalType::function)return err("higer-order function not surpported.");
        return val;
    }
    else if(cmd.type==CmdType::expr){
        auto val=interpret_tree(cmd.expr,table);
        if(val.second==EvalType::error)return val;
        return succeed;
    }
    else if(cmd.type==CmdType::func){
        if(!table.funcs.assign(cmd.name,cmd)){
            return err("function "+cmd.name+" already defined.");
        }
        cmd.copyed=true;
        return succeed;
    }
    else if(cmd.type==CmdType::ctrl){
        if(cmd.name=="if"){
            auto i=interpret_judge_do(cmd,table);
            if(i.first=="(nodone)"&&i.second==EvalType::none){

            }
            return i;
        }
        else{
            return err("unsupported control:"+cmd.name);
        }
    }
    else{
        return err("unexpected cmdtype.");
    }
}

ValType interpreter::interpret_judge_do(Command& cmd,IdTable& table){
    ValType nodone={"(nodone)",EvalType::none};
    auto i=interpreter::interpret_tree(cmd.expr,table);
    if(i.second==EvalType::error)return i;
    if(i.second!=EvalType::boolean){
        i=default_op::cast_val(EvalType::boolean,i);
        if(i.second==EvalType::error)return i;
    }    
    if(i.first=="true"){
        IdTable intable;
        intable.prev=&table;
        intable.global=table.global;
        return interpret_block(cmd.unrated,intable);
    }
    return nodone;
}

ValType interpreter::interpret_tree(Tree* tree,IdTable& table){
    ValType* transform=nullptr;
    if(!tree){
        return {"",EvalType::none};
    }
    if(tree->type==EvalType::create){
        return interpret_create(tree,table);
    }
    if(is_c_id_top_usable(tree->symbol[0])&&tree->type==EvalType::unknown){
        return {tree->symbol,EvalType::unknown};
    }
    if(tree->type>=EvalType::boolean&&tree->type<=EvalType::str){
        return {tree->symbol,tree->type};
    }
    if(tree->symbol!="()"&&tree->symbol!="."&&tree->type==EvalType::function){
        return {tree->symbol,EvalType::function};
    }
    auto leftval=interpret_tree(tree->left,table);
    if(tree->symbol!="="&&leftval.second==EvalType::error)return leftval;
    if(tree->symbol=="."){
        auto e=get_computable_value(leftval,table,transform);
        if(e.second==EvalType::error)return e;
        return interpret_dot(tree,table,*transform);
    }
    if(tree->symbol=="&&"||tree->symbol=="||"){
        auto e=get_computable_value(leftval,table,transform);
        if(e.second==EvalType::error)return e;
        leftval=*transform;
        if(leftval.second>=EvalType::boolean&&leftval.second<=EvalType::str){
            auto e=default_op::cast_val(EvalType::boolean,leftval);
            if(e.second==EvalType::error)return e;
            if(tree->symbol=="||"&&e.first=="true"){
                return e;
            }
            if(tree->symbol=="&&"&&e.first=="false"){
                return e;
            }
        }
    }
    auto rightval=interpret_tree(tree->right,table);
    if(rightval.second==EvalType::error)return rightval;
    if(tree->symbol=="++"||tree->symbol=="--"){
        return interpret_incrdecr(tree,table,leftval,rightval);
    }
    auto tmp=get_computable_value(rightval,table,transform);
    if(tmp.second==EvalType::error)return tmp;
    rightval=*transform;
    if(tree->symbol=="="){
        return interpret_assign(tree,table,leftval,rightval);
    }
    if(tree->symbol=="()"){
        if(rightval.second==EvalType::function){
            auto found=table.find_func(rightval.first);
            if(!found)return err(rightval.first+" is not function.");
            return call_function(tree,table,found);
        }
        else if(rightval.second==EvalType::memberfunc){
            Instance* inst=nullptr;
            Reader<std::string> check(rightval.first);
            auto e=resolve_instance(check,table,inst);
            if(e.second==EvalType::error)return e;
            std::string funcname;
            check.readwhile(funcname,untilincondition,is_c_id_usable<char>);
            if(funcname=="")return err("");
            return inst->call_membfunc(funcname,table,tree);
        }
    }
    auto e=get_computable_value(leftval,table,transform);
    if(e.second==EvalType::error)return e;
    leftval=*transform;
    return default_op::interpret_default(tree->symbol,leftval,rightval);
}

ValType interpreter::interpret_create(Tree* tree,IdTable& table){
    if(!tree->right||!tree->right->right){
        return err("invalid tree structure.");
    }
    auto name=tree->right->right->symbol;
    auto type=table.find_struct(name);
    if(!type)return err("struct "+name+"not defined.");
    bool first=true;
    ValType errhandle;
    size_t num=0;
    if(!type->new_instance(tree,num,errhandle))return errhandle;
    name.append("("+std::to_string(num)+")");
    return {name,EvalType::user};
}

ValType interpreter::interpret_assign(Tree* tree,IdTable& table,ValType& leftval,ValType& rightval){
    if(leftval.second==EvalType::membervar){
        ValType* id=nullptr;
        auto e=get_meber_val(leftval.first,table,id);
        if(e.second==EvalType::error)return e;
        *id=rightval;
        return rightval;
    }
    else{
        if(!tree->left)return err("invalid tree structure.");
        if(tree->left->type!=EvalType::unknown)return err("type not matched");
        Reader<std::string> s(tree->left->symbol);
        s.readwhile(untilincondition_nobuf,is_c_id_usable<char>);
        if(!s.ceof())return err("not match for id:"+s.ref());
        auto id=table.find_var(s.ref());
        if(id){
            *id=rightval;
        }
        else{
            if(!table.vars.assign(s.ref(),rightval))return err("not assignable:"+s.ref());
        }
        return rightval;
    }
}

ValType interpreter::interpret_incrdecr(Tree* tree,IdTable& table,ValType& leftval,ValType& rightval){
    auto i=tree;
    for(;i->right;i=i->right);
    auto found=table.find_var(i->symbol);
    if(!found)return err(i->symbol+" is not defined.");
    auto tmp=default_op::interpret_default(tree->symbol,leftval,rightval);
    if(tmp.second==EvalType::error)return tmp;
    *found=tmp;
    return tmp;
}

ValType interpreter::call_function(Tree* args,IdTable& table,Command* sentence){
    if(!sentence)return err("no block exist.");
    if(sentence->args.size()!=args->arg.size())return err("arg number not matched.");
    IdTable infunctable;
    size_t i=0;
    for(auto& ref:sentence->args){
        auto arg=interpret_tree(args->arg[i],table);
        if(arg.second==EvalType::error)return arg;
        infunctable.vars.assign(ref,arg);
    }
    infunctable.global=table.global;
    return interpret_block(sentence->unrated,infunctable);
}

ValType interpreter::interpret_block(std::string& sentence,IdTable& table){
    Reader<std::string> willparse(sentence,ignore_c_comments);
    if(!willparse.expect("{"))return err("unexpected token.expected {, but "+std::string(1,willparse.achar())+".");
    auto result=interpret_detail(willparse,table);
    if(result.second==EvalType::error)return result;
    //if(!willparse.expect("}"))return err("unexpected token.expected }, but "+std::string(1,willparse.achar())+".");
    return result;
}

ValType interpreter::interpret_dot(Tree* tree,IdTable& table,ValType& leftval){
    Instance* inst=nullptr;
    Reader<std::string> check(leftval.first);
    auto res=resolve_instance(check,table,inst);
    if(res.second==EvalType::error)return res;
    if(!tree->right)return err("invalid tree structure.");
    auto found=inst->base->find(tree->right->symbol);
    if(found.second==EvalType::none)return err("member not found.");
    found.first=leftval.first+found.first;
    return found;
}

ValType interpreter::resolve_instance(Reader<std::string>& check,IdTable& table,Instance*& got){
    std::string result;
    if(!is_c_id_top_usable(check.achar()))return err("not match for struct name:"+check.ref());
    check.readwhile(result,untilincondition,is_c_id_usable<char>);
    if(result=="")return err("not match for struct name:"+check.ref());
    auto found=table.find_struct(result);
    if(!found)return err(result+" is not struct.");
    if(!check.expect("("))return err("not instance:"+check.ref());
    std::string num;
    NumberContext<char> ctx;
    check.readwhile(num,number,&ctx);
    if(!check.expect(")"))return err("not instance:"+check.ref());
    uint64_t i;
    if(!default_op::stoi(num,i))return err("not instance:"+check.ref());
    if(!found->instances.count(i))return err("too large instance number:"+check.ref());
    got=&found->instances[i];
    return {"",EvalType::none}; 
}

ValType interpreter::get_meber_val(std::string& name,IdTable& table,ValType*& val){
    Reader<std::string> check(name);
    Instance* got=nullptr;
    auto i=resolve_instance(check,table,got);
    if(i.second==EvalType::error)return i;
    std::string membname;
    check.readwhile(membname,untilincondition,is_c_id_usable<char>);
    if(membname=="")return err("not match for member:"+name);
    auto id=got->member.find(membname);
    if(!id)return err("member "+membname+" not found");
    val=id;
    return {"",EvalType::none};
}

ValType interpreter::get_var_val(std::string& name,IdTable& table,ValType*& val){
    auto value=table.find_var(name);
    if(!value)return err(name+" is not defined");
    val=value;
    return {"",EvalType::none};
}

ValType interpreter::get_computable_value(ValType& type,IdTable& table,ValType*& val){
    val=nullptr;
    if(type.second==EvalType::unknown){
        return get_var_val(type.first,table,val);
    }
    else if(type.second==EvalType::membervar){
        return get_meber_val(type.first,table,val);
    }
    else{
        val=&type;
        return {"",EvalType::none};
    }
}