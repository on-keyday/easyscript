/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include<string>
#include<vector>
#include"../commonlib/reader_helper.h"

namespace control{
    std::string parse_type(PROJECT_NAME::Reader<std::string>& reader,bool strict=false);

    bool parse_funcarg(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,std::string& ret,std::string& opt,bool strict=false);

    bool parse_arg_need_type(PROJECT_NAME::Reader<std::string>& reader,std::string& name,std::string& type);

    bool parse_arg_need_name(PROJECT_NAME::Reader<std::string>& reader,std::string& name,std::string& type);

    std::string typevec_to_type(std::vector<std::string>& type,std::string& ret,std::string& opt);
}