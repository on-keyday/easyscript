/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"../example_tree.h"
#include<string>
#include<vector>

namespace control{
    enum class TreeKind {
        unknown,
        create,
        boolean,
        integer,
        floats,
        string
    };

    struct Tree{
        Tree* left=nullptr;
        Tree* right=nullptr;
        std::string symbol;
        TreeKind kind=TreeKind::unknown;
        std::vector<Tree*> arg;
        ~Tree(){
            delete left;
            delete right;
            for(auto p:arg){
                delete p;
            }
        }
    };

    template<class Buf>
    Tree* expr_parse(PROJECT_NAME::Reader<Buf>& reader){
        PROJECT_NAME::ExampleTree<Tree,TreeKind,Buf> ctx;
        reader.readwhile(PROJECT_NAME::parse_expr,ctx);
        if(!ctx.result||ctx.syntaxerr){
            delete ctx.result;
            return nullptr;
        }
        return ctx.result;
    }

    template<class T>
    inline T* make(){
        try{
            return new T();
        }
        catch(...){
            return nullptr;
        }
    }

    template<class T,class... Arg>
    inline T* make(Arg... arg){
        try{
            return new T(arg...);
        }
        catch(...){
            return nullptr;
        }
    }


    enum class CtrlKind{
        ctrl,
        func,
        expr
    };

    struct Control{
        //CtrlKind kind=CtrlKind::unknown;
        CtrlKind kind=CtrlKind::ctrl;
        Tree* expr=nullptr;
        std::string name;
        std::string inblock;
        std::string type;
        std::vector<std::string> arg;
        std::vector<std::string> argtype;
        size_t inblockpos=0;
        Control(const std::string name="",Tree* expr=nullptr,CtrlKind kind=CtrlKind::ctrl):
        name(name),kind(kind),expr(expr){}
        ~Control(){
            delete expr;
        }
    };

    Control* control_parse(PROJECT_NAME::Reader<std::string>& reader);

    bool parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf,size_t& pos);

    std::string parse_type(PROJECT_NAME::Reader<std::string>& reader,bool strict=false);

    bool parse_function(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,std::string& ret,bool strict=false);
}
