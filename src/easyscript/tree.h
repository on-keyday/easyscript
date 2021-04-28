/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"../reader.h"
#include"../reader_helper.h"
#include<string>
#include<vector>
#include<map>

enum class EvalType{
    error,
    //undefined,
    unknown,
    none,
    boolean,
    integer,
    floats,
    str,
    function,
    user,
    memberfunc,
    membervar,
    create
};

struct Tree{
    std::string symbol;
    EvalType type=EvalType::unknown;
    Tree* left=nullptr;
    Tree* right=nullptr;
    std::vector<Tree*> arg;
    Tree(const std::string& symbol,EvalType type=EvalType::unknown,Tree* left=nullptr,Tree* right=nullptr)
    :symbol(symbol),type(type),left(left),right(right){}
    ~Tree(){
        delete left;
        delete right;
        for(auto tmp:arg){
            delete tmp;
        }
    }
};
enum class CmdType{
    //define,
    deletes,
    func,
    ctrl,
    returns,
    expr,
    assign
};


struct Command{
    CmdType type;
    std::string name;
    Tree* expr=nullptr;
    std::vector<std::string> args;
    std::string unrated;
    bool copyed=false;
    Command()=default;
    Command(const Command&)=delete;
    Command(Command&& from){
        this->expr=from.expr;
        from.expr=nullptr;
        this->type=from.type;
        this->args=std::move(from.args);
        this->name=std::move(from.name);
        this->unrated=std::move(from.unrated);
    }
    Command& operator=(Command& from){
        if(this->expr!=from.expr){
            delete this->expr;
            this->expr=from.expr;
            from.expr=nullptr;
        }
        this->type=from.type;
        this->args=std::move(from.args);
        this->name=std::move(from.name);
        this->unrated=std::move(from.unrated);
        return *this;
    }

    Command& operator=(Command&& from) noexcept{
        if(this->expr!=from.expr){
            delete this->expr;
            this->expr=from.expr;
            from.expr=nullptr;
        }
        this->type=from.type;
        this->args=std::move(from.args);
        this->name=std::move(from.name);
        this->unrated=std::move(from.unrated);
        return *this;
    }

    ~Command(){
        delete expr;
    }
};

//using VarMap=std::map<std::string,std::pair<std::string,EvalType>>;
using ValType=std::pair<std::string,EvalType>;
struct VarTable{
    std::map<std::string,ValType> table; 
    /*bool new_value(const std::string& str,std::pair<std::string,EvalType>&& value){
        return table.try_emplace(str,std::move(value)).second;
    }*/
    bool nochange=false;
    bool assign(const std::string& str,ValType& value){
        if(nochange&&!table.count(str))return false;
        table[str]=value;
        return true;
    }
    ValType* find(const std::string& key){
        if(!table.count(key))return nullptr;
        return &table[key];
    }
    //bool remove_from_table(const std::string& name);
};

struct FuncTable{
    std::map<std::string,Command> table; 
    Command* find(const std::string& key){
        if(!table.count(key))return nullptr;
        return &table[key];
    }

    bool assign(const std::string& key,Command& value){
        if(table.count(key))return false;
        table[key]=std::move(value);
        return true;
    }
};

struct Struct;
struct IdTable;

struct Instance{
    size_t id=0;
    Struct* base;
    VarTable member;
    bool assign(const std::string& name,ValType& value){
        return member.assign(name,value);
    }
    ValType call_membfunc(const std::string& name,IdTable& table,Tree* arg);
};

struct ArgContext{

};

struct Struct{
    using Proxy=int(*)(void* ctx,const char* membname,ArgContext* arg);
    std::string name;
    FuncTable funcs;
    VarTable inits;
    std::map<size_t,Instance> instances;
    size_t instcount=0;
    Proxy proxy;
    void* ctx;
    ValType new_instance(Tree* constructor_arg,IdTable& table);
    ValType call_proxy(size_t id,IdTable& func,Tree* arg);
    ValType find(const std::string& name){
        if(proxy)return {name,EvalType::memberfunc};
        if(inits.find(name)){
            return {name,EvalType::membervar};
        }
        else if(funcs.find(name)){
            return {name,EvalType::memberfunc};
        }
        return {"",EvalType::none};
    }
};

struct StructTable{
    std::map<std::string,Struct> table;
    Struct* find(const std::string& key){
        if(!table.count(key))return nullptr;
        return &table[key];
    }
};

struct IdTable{
    IdTable* prev=nullptr;
    IdTable* global=nullptr;
    VarTable vars;
    StructTable structs;
    FuncTable funcs;
    ValType* find_var(const std::string& key){
        auto ret=vars.find(key);
        auto p=prev;
        while(!ret&&p){
            ret=p->vars.find(key);
            p=p->prev;
        }
        if(!ret&&global){
            ret=global->vars.find(key);
        }
        return ret;
    }
    Command* find_func(const std::string& key){
        auto ret=funcs.find(key);
        auto p=prev;
        while(!ret&&p){
            ret=p->funcs.find(key);
            p=p->prev;
        }
        if(!ret&&global){
            ret=global->funcs.find(key);
        }
        return ret;
    }
    Struct* find_struct(const std::string& key){
        auto ret=structs.find(key);
        auto p=prev;
        while(!ret&&p){
            ret=p->structs.find(key);
            p=p->prev;
        }
        if(!ret&&global){
            ret=global->structs.find(key);
        }
        return ret;
    }
};


template<class T>
inline T* make(){
    try{
        return new T;
    }
    catch(...){
        return nullptr;
    }
}

template<class T,class... Args>
inline T* make(Args... args){
    try{
        return new T(args...);
    }
    catch(...){
        return nullptr;
    }
}

bool op_expect(PROJECT_NAME::Reader<std::string>& reader,const char*& expected);

template<class First,class... Args>
bool op_expect(PROJECT_NAME::Reader<std::string>& reader,const char*& expected,First first,Args... args){
    if(reader.expect(first,expected)){
        return true;
    }
    return op_expect(reader,expected,args...);
}

Tree* expression(PROJECT_NAME::Reader<std::string>& reader);

bool expect_on_depth(PROJECT_NAME::Reader<std::string>& reader,const char*& expected,int depth);

Tree* bin(PROJECT_NAME::Reader<std::string>& reader,int depth);

Tree* unary(PROJECT_NAME::Reader<std::string>& reader);

Tree* number_or_id(PROJECT_NAME::Reader<std::string>& reader);

bool after(Tree*& ret,PROJECT_NAME::Reader<std::string>& reader);

EvalType eval_tree(Tree* tree,IdTable* table=nullptr);

EvalType default_operator(Tree* tree,EvalType lefttype,EvalType righttype);