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

bool collect_ids(std::vector<control::Control>& ctrlvec,IdTable& table){
    for(auto& it:ctrlvec){
        if(it.kind==CtrlKind::var){
            return false;
        }
    }
    return false;
}