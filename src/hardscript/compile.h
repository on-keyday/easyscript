/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"../commonlib/reader.h"
#include"control.h"
#include<map>

namespace compiler{
    /*enum class bycode{
        mov,  
        nop,
        add,
        sub,
        push,
        pop,
        ret,
        call,
        ld,
        jmp
    };

    struct Code
    {
        bycode code;
        std::string op1;
        std::string op2;
    };*/

    struct IdTable{
        IdTable* prev;
        IdTable* global;
        std::map<std::string,std::string> funcs;
        std::map<std::string,std::string> vars;
    };
    /*
    bool to_bytecode(std::vector<control::Control>& ctrlvec,size_t& pos,IdTable& table,std::string& out);

    bool genifcode(std::vector<control::Control>& ctrlvec,size_t& pos,IdTable& table,std::string& out);

    bool genbytree(control::Tree* tree,IdTable& table,std::string& out);
    */

   bool collect_ids(std::vector<control::Control>& ctrlvec,IdTable& table);

   unsigned int calcable_as_const(control::Tree* t,std::string& str,long long& sint,unsigned long long& uint,double& floats);
   
   inline void packstr(std::string& str,std::string& from){str="'"+from+"'";}

   bool calcable_bool(control::Tree* t,std::string& str,long long& sint,unsigned long long& uint,double& floats);

   unsigned int calcable_num(control::Tree* t,std::string& str,long long& sint,unsigned long long& uint,double& floats);

   
}