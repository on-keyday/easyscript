#pragma once
#include"parse.h"
#include<utility>
//auto err(const std::string& msg)->ValType;

namespace interpreter{

    int interpret(PROJECT_NAME::Reader<std::string>& reader,ValType* err);

    ValType interpret_detail(PROJECT_NAME::Reader<std::string>& reader,IdTable& table);

    ValType interpret_a_cmd(Command& cmd,IdTable& table,std::vector<Command>& cmds);

    ValType interpret_judge_do(Command& cmd,IdTable& table);

    ValType interpret_tree(Tree* tree,IdTable& table);

    ValType interpret_create(Tree* tree,IdTable& table);

    ValType interpret_assign(Tree* tree,IdTable& table,ValType& leftval,ValType& rightval);

    ValType interpret_incrdecr(Tree* tree,IdTable& table,ValType& leftval,ValType& rightval);

    ValType call_function(Tree* tree,IdTable& table,Command* sentence);
    
    ValType interpret_block(std::string& sentence,IdTable& table);

    ValType interpret_dot(Tree* tree,IdTable& table,ValType& leftval);

    ValType resolve_instance(PROJECT_NAME::Reader<std::string>& check,IdTable& table,Instance*& got);

    ValType get_meber_val(std::string& name,IdTable& table,ValType*& val);

    ValType get_var_val(std::string& name,IdTable& table,ValType*& val);

    ValType get_computable_value(ValType& type,IdTable& table,ValType*& val);
}