#include"compile.h"

using namespace compiler;
using namespace control;
using namespace PROJECT_NAME;

/*
bool compiler::to_bytecode(std::vector<control::Control>& ctrlvec,size_t& pos,IdTable& table,std::string& out){
    if(ctrlvec[pos].kind==CtrlKind::ctrl){
        if(ctrlvec[pos].name=="if"){
            return genifcode(ctrlvec,pos,table,out);
        }
    }
    else if(ctrlvec[pos].kind==CtrlKind::var){

    }
    else if(ctrlvec[pos].kind==CtrlKind::func){

    }
    return false;
}

bool compiler::genifcode(std::vector<control::Control>& ctrlvec,size_t& pos,IdTable& table,std::string& out){
    return false;
}

bool compiler::genbytree(Tree* tree,IdTable& table,std::string& out){
    if(!tree)return false;
    if(tree->kind==TreeKind::unknown&&is_c_id_top_usable(tree->symbol[0])){

    }
    return false;
}*/

bool compiler::collect_ids(std::vector<control::Control>& ctrlvec,IdTable& table){
    for(auto& it:ctrlvec){
        if(it.kind==CtrlKind::var){
            return false;
        }
    }
    return false;
}

enum {
    STR=0x1,
    SIGNED=0x2,
    UNSIGNED=0x4,
    FLOAT=0x8,
    BASE_STR=0x10,
    BASE_INT=0x20,
    BASE_FLOAT=0x40,
    BASE_BOOL=0x80,
    ALL_OK=STR|SIGNED|UNSIGNED|FLOAT
};

#define SET_FLAG(ret,flag) do{ret&=(0x0f&flag);ret|=(0xf0&flag);}while(0)

unsigned int compiler::calcable_as_const(control::Tree* t,std::string& str,long long& sint,unsigned long long& uint,double& floats){
    if(!t)return 0;
    unsigned int ret=ALL_OK,leftc=ret,rightc=ret;
    std::string lefts,rights;
    long long leftn=0,rightn=0;
    unsigned long long leftu=0,rightu=0;
    double leftf=0,rightf=0;
    if(t->left){
        leftc=calcable_as_const(t->left,lefts,leftn,leftu,leftf);
        if(leftc==0)return 0;
    }
    if(t->right){
        rightc=calcable_as_const(t->right,rights,rightn,rightu,rightf);
        if(rightc==0)return 0;
    }
    SET_FLAG(ret,rightc);
    SET_FLAG(ret,leftc);
    if(ret==0)return 0;
    if(t->kind==TreeKind::boolean){
        if(!calcable_bool(t,str,sint,uint,floats))return 0;
        ret|=BASE_BOOL;
    }
    else if(t->kind==TreeKind::integer){
        ret&=calcable_num(t,str,sint,uint,floats);
        ret|=BASE_INT;
    }
    else if(t->kind==TreeKind::floats){
        ret&=calcable_num(t,str,sint,uint,floats);
        ret|=BASE_FLOAT;
    }
    else if(t->kind==TreeKind::string){
        
    }
    return ret;
}

bool compiler::calcable_bool(control::Tree* t,std::string& str,long long& sint,unsigned long long& uint,double& floats){
    packstr(str,t->symbol);
    if(t->symbol=="true"){
        sint=1;
        uint=1;
        floats=1;
    }
    else if(t->symbol=="false"){
        sint=0;
        uint=0;
        floats=0;
    }
    else{
        return false;
    }
    return true;
}

unsigned int compiler::calcable_num(control::Tree* t,std::string& str,long long& sint,unsigned long long& uint,double& floats){
    unsigned int ret=ALL_OK;
    packstr(str,t->symbol);
    try{
        uint=std::stoull(t->symbol);
    }
    catch(...){
        ret&=~UNSIGNED;
        ret&=~SIGNED;
    }
    if(uint&0x80'00'00'00'00'00'00'00){
        ret&=~SIGNED;
    }
    else{
        sint=(long long)uint;
    }
    try{
        floats=std::stod(t->symbol);
    }
    catch(...){
        ret=~FLOAT;
    }
    return ret;
}

