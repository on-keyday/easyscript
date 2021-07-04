/*license*/
#pragma once
#include"tree.h"




/*
struct Proccess{
    std::vector<Command> cmds;
    IdTable table;
};*/


bool parse(PROJECT_NAME::Reader<std::string>& reader,std::vector<Command>& cmdvec);

bool parse_func(Command& cmd,PROJECT_NAME::Reader<std::string>& reader);

bool parse_expr(Command& cmd,PROJECT_NAME::Reader<std::string>& reader);

bool parse_delete(Command& cmd,PROJECT_NAME::Reader<std::string>& reader);

bool parse_while(Command& cmd,PROJECT_NAME::Reader<std::string>& reader);

size_t parse_if(Command& cmd,PROJECT_NAME::Reader<std::string>& reader,std::vector<Command>& cmdvec);

bool parse_return(Command& cmd,PROJECT_NAME::Reader<std::string>& reader);

bool parse_unrated(PROJECT_NAME::Reader<std::string>& reader,std::string& str);

//Tree* parse_expr(PROJECT_NAME::Reader<std::string>& reader);

