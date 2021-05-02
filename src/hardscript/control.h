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
        reader.readwhile(PROJECT_NAME::read_expr,ctx);
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
        var,
        expr
    };

    struct Control{
    private:
        bool moving(Control& from){
            if(expr!=from.expr){
                delete expr;
            }
            expr=from.expr;
            from.expr=nullptr;
            kind=from.kind;
            name=std::move(from.name);
            inblock=std::move(from.inblock);
            type=std::move(from.type);
            arg=std::move(from.arg);
            argtype=std::move(from.argtype);
            inblockpos=from.inblockpos;
            from.inblockpos=0;
            return true;
        }
    public:
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
        Control(const Control&)=delete;
        Control(Control&& from) noexcept{
            moving(from);
        }

        Control& operator=(Control&& from){
            moving(from);
        }
        ~Control(){
            delete expr;
        }
    };

    bool parse_all(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool check_initexpr(PROJECT_NAME::Reader<std::string>& reader);

    bool control_parse(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf,size_t& pos);

    bool parse_expr(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,bool semicolon);

    std::string parse_type(PROJECT_NAME::Reader<std::string>& reader,bool strict=false);

    bool parse_funcarg(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,std::string& ret,bool strict=false);

    std::string typevec_to_type(std::vector<std::string>& type,std::string& ret);

    bool parse_var(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_var_detail(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,bool bracket,bool initeq);

    bool parse_if(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_func(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_for(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);
}
