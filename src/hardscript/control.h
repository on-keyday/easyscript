/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"../commonlib/example_tree.h"
#include<string>
#include<vector>
#include"../commonlib/json_util.h"

namespace control{
    enum class TreeKind {
        unknown,
        create,
        boolean,
        integer,
        floats,
        string, 
        closure
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
        void to_json(PROJECT_NAME::JSON& j)const;
    };
    
    
    struct Flags{
        bool mustexpect=false;
        bool syntaxerr=false;
    };

    template<class Str,class Buf>
    int checker(PROJECT_NAME::Reader<Buf>* self,Str& expected,int depth,Flags& flag){
        auto check=[&](auto... args){return (int)PROJECT_NAME::op_expect(self,expected,args...);};
        auto chone=[&](auto& arg){
            return (int)self->expectp(arg,expected,[&](auto c){return c==arg[0];});
        };
        auto cast=[&]{
            auto beginpos=self->readpos();
            if(self->expect("cast",PROJECT_NAME::is_c_id_usable)){
                if(!self->expect("<")){
                    self->seek(beginpos);
                    return 0;
                }
                Str buf;
                buf.append("cast<");
                if(!self->readwhile(buf,PROJECT_NAME::until,'>')){
                    self->seek(beginpos);
                    return 0;
                }
                self->expect(">");
                expected=std::move(buf);
                return 1;
            }
            return 0;
        };
        switch (depth){
        case 8:
            if(flag.mustexpect){
                if(check(":")){
                    flag.mustexpect=false;
                    return -1;
                }
                flag.syntaxerr=true;
                return 0;
            }
            else if(check("?")){
                flag.mustexpect=true;
                return 1;
            }
            return 0;
        case 7:
            return check("=");
        case 6:
            return check("||");
        case 5:
            return check("&&");
        case 4:
            return check("==","!=");
        case 3:
            return check("<=",">=","<",">");
        case 2:
            return check("+","-")||chone("|")||chone("&")||chone("^");
        case 1:
            return check("*","/","%","<<",">>");
        case 0:
            return check("++","--","+","-","!","&","*")||cast();
        default:
            return false;
        }
    }

    template<class Buf,class Str=Buf>
    Tree* expr_parse(PROJECT_NAME::Reader<Buf>& reader){
        PROJECT_NAME::ExampleTree<Tree,Str,TreeKind,Buf,decltype(checker<Str,Buf>),Flags> 
        ctx(checker,8);
        reader.readwhile(PROJECT_NAME::read_expr<Buf,decltype(ctx),Str>,ctx);
        if(!ctx.result||ctx.flags.syntaxerr){
            delete ctx.result;
            return nullptr;
        }
        return ctx.result;
    }

    inline Tree* expr(PROJECT_NAME::Reader<std::string>& reader){
        return expr_parse(reader);
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
        decl,
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
            return *this;
        }
        ~Control(){
            delete expr;
        }

        void to_json(PROJECT_NAME::JSON& j)const;
    };

    bool parse_all(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool check_initexpr(PROJECT_NAME::Reader<std::string>& reader);

    bool control_parse(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_inblock(PROJECT_NAME::Reader<std::string>& reader,std::string& buf,size_t& pos);

    bool parse_expr(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,bool semicolon);

    bool parse_var(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_var_detail(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,const char* bracket,bool initeq);

    bool parse_if(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);

    bool parse_func(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec,bool decl=false);

    bool parse_for(PROJECT_NAME::Reader<std::string>& reader,std::vector<Control>& vec);
}
