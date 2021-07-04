/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#include"tree.h"
#include"default_operator.h"
#include"interpret.h"
using namespace PROJECT_NAME;
using default_op::err;


bool op_expect(PROJECT_NAME::Reader<std::string>& reader,const char*& expected){
    return false;
}

ValType Struct::new_instance(Tree* constructor_arg,IdTable& table){
    auto& ref=this->instances[instcount];
    ref.id=instcount;
    instcount++;
    ref.base=this;
    for(auto i:this->inits.table){
        ref.member.assign(i.first,i.second);
    }
    if(this->find("__init__").second==EvalType::memberfunc){
        auto res=ref.call_membfunc("__init__",table,constructor_arg);
        if(res.second==EvalType::error)return res;
    }
    return {this->name+"("+std::to_string(ref.id)+")",EvalType::user};
}

ValType Instance::call_membfunc(const std::string& name,IdTable& table,Tree* arg){
    if(base->proxy){
        return base->call_proxy(name,this,table,arg);
    }
    auto func=base->funcs.find(name);
    if(!func)return err(name+" is not member function of "+base->name);
    IdTable selftable;
    selftable.global=table.global;
    ValType self={base->name+"("+std::to_string(id)+")",EvalType::user};
    selftable.vars.assign("self",self);
    return interpreter::call_function(arg,table,func);
}

Instance::~Instance(){
    IdTable tmp;
    call_membfunc("__delete__",tmp,nullptr);
}

ValType Struct::call_proxy(const std::string& name,Instance* id,IdTable& table,Tree* arg){
    ArgContext argctx(table,id);
    if(arg){
        for(auto p:arg->arg){
            auto argv=interpreter::interpret_tree_invoke(p,table);
            if(argv.second==EvalType::error)return argv;
            argctx.args.push_back(std::move(argv));
        }
    }
    proxy(this->ctx,name.c_str(),&argctx);
    return argctx.result;
}

Tree* expression(Reader<std::string>& reader){
    return bin(reader,7);
}

bool expect_on_depth(PROJECT_NAME::Reader<std::string>& reader,const char*& expected,int depth){
    auto check=[&reader,&expected](auto... args){return op_expect(reader,expected,args...);};
    switch (depth)
    {
    case 7:
        return check("=");
    case 6:
        return check("||");
    case 5:
        return check("&&");
    case 4:
        return check("==","!=");
    case 3:
        return check("<=",">=","<",">");
    case 2:
        return check("*","/","%");
    case 1:
        return check("+","-");
    default:
        return false;
    }
}

Tree* bin(Reader<std::string>& reader,int depth){
    if(!depth)return unary(reader);
    Tree* ret=bin(reader,depth-1);
    if(!ret)return nullptr;
    const char* expected=nullptr;
    auto check=[&]{return expect_on_depth(reader,expected,depth);};
    while(check()){
        auto tmp=bin(reader,depth-1);
        if(!tmp){
            delete ret;
            return nullptr;
        }
        auto will=make<Tree>(expected,EvalType::unknown,ret,tmp);
        if(!will){
            delete ret;
            delete tmp;
            return nullptr;
        }
        ret=will;
    }
    return ret;
}

Tree* unary(Reader<std::string>& reader){
    Tree* ret=nullptr;
    const char* expected=nullptr;
    if(op_expect(reader,expected,"++","--")){
        auto tmp=number_or_id(reader);
        if(!tmp)return nullptr;
        ret=make<Tree>(expected,EvalType::unknown,nullptr,tmp);
        if(!ret){
            delete tmp;
            return nullptr;
        }
    }
    else if(op_expect(reader,expected,"+","-","!")){
        auto tmp=unary(reader);
        if(!tmp)return nullptr;
        ret=make<Tree>(expected,EvalType::unknown,nullptr,tmp);
        if(!ret){
            delete tmp;
            return nullptr;
        }
    }
    else{
        return number_or_id(reader);
    }
    return ret;
}

Tree* number_or_id(Reader<std::string>& reader){
    Tree* ret=nullptr;
    if(reader.expect("(")){
        ret=expression(reader);
        if(!ret)return nullptr;
        if(!reader.expect(")")){
            delete ret;
            return nullptr;
        }
    }
    else if(reader.ahead("\"")||reader.ahead("'")){
        std::string str;
        if(!reader.string(str))return nullptr;
        ret=make<Tree>(str,EvalType::str);
        if(!ret)return nullptr;
    }
    else if(is_digit(reader.achar())){
        std::string num;
        NumberContext<char> ctx;
        reader.readwhile(num,number,&ctx);
        if(ctx.failed)return nullptr;
        if(ctx.floatf){
            ret=make<Tree>(num,EvalType::floats);
        }
        else{
            ret=make<Tree>(num,EvalType::integer);
        }
        if(!ret)return nullptr;
    }
    else if(reader.expect("new",is_c_id_usable)){
        auto tmp=expression(reader);
        if(!tmp||tmp->symbol!="()"||!tmp->right){
            delete tmp;
            return nullptr;
        }
        ret=make<Tree>("new",EvalType::create,nullptr,tmp);
        if(!ret){
            delete tmp;
            return nullptr;
        }
    }  
    else if(is_c_id_top_usable(reader.achar())){
        std::string id;
        reader.readwhile(id,untilincondition,is_c_id_usable<char>);
        if(id.size()==0)return nullptr;
        if(id=="true"||id=="false"){
            ret=make<Tree>(id,EvalType::boolean);
        }
        else{
            ret=make<Tree>(id);
        }
        if(!ret)return nullptr;
    }
    else{
        return nullptr;
    }
    if(!after(ret,reader))return nullptr;
    return ret;
}

bool after(Tree*& ret,PROJECT_NAME::Reader<std::string>& reader){
    while(true){
        const char* expected=nullptr;
        if(reader.expect("(")){
            std::vector<Tree*> args;
            while(true){
                if(reader.expect(")"))break;
                auto tmp=expression(reader);
                if(!tmp){
                    delete ret;
                    return false;
                }
                args.push_back(tmp);
                if(!reader.expect(",")&&!reader.ahead(")")){
                    delete ret;
                    return false;
                }
            }
            //ret->type=EvalType::function;
            auto will=make<Tree>("()",EvalType::unknown,nullptr,ret);
            if(!will){
                for(auto t:args){
                    delete t;
                }
                delete ret;
                return false;
            }
            will->arg=std::move(args);
            ret=will;
            continue;
        }
        else if(reader.expect(".")){
            if(!is_c_id_top_usable(reader.achar())){
                delete ret;
                return false;
            }
            std::string id;
            reader.readwhile(id,untilincondition,is_c_id_usable<char>);
            if(id.size()==0)return false;
            auto tmp1=make<Tree>(id);
            if(!tmp1){
                delete ret;
                return false;
            }
            auto tmp2=make<Tree>(".",EvalType::unknown,ret,tmp1);
            if(!tmp2){
                delete ret;
                delete tmp1;
                return false;
            }
            ret=tmp2;
            continue;
        }
        else if(reader.expectp("++",expected)||reader.expectp("--",expected)){
            auto tmp=make<Tree>(expected,EvalType::unknown,nullptr,ret);
            if(!tmp){
                delete ret;
                return false;
            }
            ret=tmp;
        }
        return true;
    }
}

EvalType eval_tree(Tree* tree,IdTable* table){
    if(!tree)return EvalType::none;
    if(tree->type>=EvalType::boolean&&tree->type<=EvalType::str){
        return tree->type;
    }
    auto lefttype=eval_tree(tree->left,table);
    auto righttype=eval_tree(tree->right,table);
    if(tree->symbol=="()"){
        if(!table)return EvalType::unknown;
    }
    if(tree->type==EvalType::create){
        if(!table)return EvalType::unknown;
    }
    if(is_c_id_top_usable(tree->symbol[0])){
        if(!table)return EvalType::unknown;
    }
    else if(tree->symbol=="."){
        if(!table)return EvalType::unknown;
    }
    else{
        if(lefttype<=EvalType::unknown||righttype<=EvalType::unknown){
            return lefttype<righttype?lefttype:righttype;
        }
        if(lefttype==EvalType::user||righttype==EvalType::user){
            if(!table)return EvalType::unknown;
        }
        return default_operator(tree,righttype,lefttype);
    }
}

EvalType default_operator(Tree* tree,EvalType lefttype,EvalType righttype){
    EvalType ret=EvalType::unknown;
    if(lefttype==EvalType::none&&righttype!=EvalType::none){
        if(tree->symbol=="!"){
            ret=EvalType::boolean;
        }
        else{
            if(righttype!=EvalType::integer)return EvalType::error;
            ret= EvalType::integer;
        }
    }
    if(tree->symbol=="+"){
        ret=righttype>lefttype?righttype:lefttype;
    }
    else if(tree->symbol=="-"){
        if(righttype==EvalType::str||lefttype==EvalType::str)return EvalType::error;
        ret=righttype>lefttype?righttype:lefttype;
    }
    else if(tree->symbol=="*"){
        if(righttype==EvalType::str&&lefttype==EvalType::integer){
            ret=righttype;
        }
        else{
            if(righttype==EvalType::str||lefttype==EvalType::str)return EvalType::error;
            ret=righttype>lefttype?righttype:lefttype;
        }
    }
    else if(tree->symbol=="/"){
        if(righttype==EvalType::str||lefttype==EvalType::str)return EvalType::error;
        ret=righttype>lefttype?righttype:lefttype;
    }
    else if(tree->symbol=="%"){
        if(righttype!=EvalType::integer||lefttype!=EvalType::integer)return EvalType::error;
        ret=EvalType::integer;
    }
    else{
        ret=EvalType::boolean;
    }
    //tree->type=ret;
    return ret;
}