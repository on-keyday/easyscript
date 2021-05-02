#include"compile.h"

using namespace compiler;
using namespace control;

bool compiler::to_bytecode(std::vector<control::Control>& ctrlvec,size_t& pos,std::string& out){
    if(ctrlvec[pos].kind==CtrlKind::ctrl){
        if(ctrlvec[pos].name=="if"){
            return genifcode(ctrlvec,pos,out);
        }
    }
}

bool compiler::genifcode(std::vector<control::Control>& ctrlvec,size_t& pos,std::string& out){
    
}

bool compiler::genbytree(Tree* tree,std::string& out){
    if(!tree)return false;
    if(tree->symbol=="="){
        
    }
}