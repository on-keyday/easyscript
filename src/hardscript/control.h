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

    struct Control{
        //CtrlKind kind=CtrlKind::unknown;
        Tree* expr=nullptr;
        std::string name;
        std::string inblock;
        std::string type;
        Control(const std::string name="",Tree* expr=nullptr):
        name(name)/*,kind(kind)*/,expr(expr){}
        ~Control(){
            delete expr;
        }
    };

    Control* control_parse(PROJECT_NAME::Reader<std::string>& reader);

    bool parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf);

    std::string parse_type(PROJECT_NAME::Reader<std::string>& reader,bool strict=false);

    bool parse_function(PROJECT_NAME::Reader<std::string>& reader,std::vector<std::string>& arg,std::vector<std::string>& type,bool strict=false);
}