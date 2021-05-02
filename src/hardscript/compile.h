/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"reader.h"
#include"control.h"


namespace compiler{
    enum class bycode{
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
    };

    bool to_bytecode(std::vector<control::Control>& ctrlvec,size_t& pos,std::string& out);

    bool genifcode(std::vector<control::Control>& ctrlvec,size_t& pos,std::string& out);

    bool genbytree(control::Tree* tree,std::string& out);
}