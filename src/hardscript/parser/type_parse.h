/*license*/
#pragma once
#include<string>
#include<vector>
#include"../../commonlib/basic_helper.h"
#include"control.h"

namespace control{
    std::string parse_type(ReaderT& reader,bool strict=false);

    bool parse_funcarg(ReaderT& reader,std::vector<std::string>& arg,std::vector<std::string>& type,std::string& ret,std::string& opt,bool strict=false);

    bool parse_arg_need_type(ReaderT& reader,std::string& name,std::string& type);

    bool parse_arg_need_name(ReaderT& reader,std::string& name,std::string& type);

    std::string typevec_to_type(std::vector<std::string>& type,std::string& ret,std::string& opt);
}
