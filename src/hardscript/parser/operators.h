#pragma once
#include"hard_tree.h"
namespace node{

    enum class NodeKind {
        unknown,

        //object
        create,
        boolean,
        integer,
        floats,
        string, 
        closure,
        id,
        
        //node
        ctrl_for,
        ctrl_select,
        ctrl_switch,
        ctrl_if,
        ctrl_case,
        ctrl_break,
        ctrl_continue,
        ctrl_return,

        //define
        def_var,
        def_func,
        dec_func,
        def_struct,
        def_interface,
        def_enum,
        def_union,

        //operator
        op_add, //a + b
        op_sub, //a - b
        op_mul, //a * b
        op_div, //a / b
        op_mod, //a % b
        op_bor, //a | b
        op_band, //a & b
        op_bxor, //a ^ b
        op_lor, //a || b
        op_land, //a && b
        op_cast, //cast<T> a
        op_cond, //a ? b : c
        op_equ, //a == b
        op_neq, //a != b
        op_rsh, //a >> b
        op_lsh, //a << b
        op_call, //a()
        op_incr, //a++ ++a
        op_decr, //a-- --a
        op_dref, //*a
        op_addr, //&a
        op_assign, //a=b
        op_init, //a:=b
    };

    template<class Tree,class Buf,class Recall,class Error,class Str=Buf>
    struct Check{
        bool mustexpect=false;
        bool syntaxerr=false;
        Recall& recall;
        Error errbuf;
        Check(Recall& func):recall(func){}

        void error(const char* str){
            errbuf.append(str);
        }

        NodeKind decl_kind(const Str& str,Tree* left,Tree* right){
            auto e=[&str](auto s){return str==s;};
            if(!left&&!right){
                if(e("int")){
                    return NodeKind::integer;
                }
                else if(e("float")){
                    return NodeKind::floats;
                }
                else if(e("bool")){
                    return NodeKind::boolean;
                }
                else if(e("closure")){
                    return NodeKind::closure;
                }
                else if(e("new")){
                    return NodeKind::create;
                }
                else if(e("var")){
                    return NodeKind::id;
                }
                else{
                    return NodeKind::unknown;
                }
            }
            return NodeKind::unknown;
            if(!left&&right){
                if(e("()")){
                    return NodeKind::op_call;
                }
                else if(e("++")){
                    return NodeKind::op_incr;
                }
                else if(e("--")){
                    return NodeKind::op_decr;
                }
            }
            return NodeKind::unknown;
        }
        
        int expect(PROJECT_NAME::Reader<Buf>* self,Str& expected,int depth){
            auto check=[&](auto... args){return (int)PROJECT_NAME::op_expect_default(self,expected,args...);};
            auto chone=[&](auto... args){return PROJECT_NAME::op_expect_one(self,expected,[](char c){
                return c=='=';
            },args...);};
            auto cheqal=[&](auto... args){return PROJECT_NAME::op_expect(self,expected,[](char c){
                return c=='=';
            },args...);};
            auto cast=[&]{
                auto beginpos=self->readpos();
                if(self->expect("cast<")){
                    Str buf;
                    buf.append("cast<");
                    if(!self->readwhile(buf,PROJECT_NAME::until,'>')){
                        self->seek(beginpos);
                        return 0;
                    }
                    self->expect(">");
                    buf.append(">");
                    expected=std::move(buf);
                    return -1;
                }
                return 0;
            };
            switch (depth){
            case 8:
                if(mustexpect){
                    if(check(":")){
                        mustexpect=false;
                        return -1;
                    }
                    syntaxerr=true;
                    return 0;
                }
                else if(check("?")){
                    mustexpect=true;
                    return 1;
                }
                return 0;
            case 7:
                return check("=","+=","-=","*=","/=","%=","|=","&=","^=");
            case 6:
                return check("||");
            case 5:
                return check("&&");
            case 4:
                return check("==","!=");
            case 3:
                return check("<=",">=","<",">");
            case 2:
                return cheqal("+","-",">>","<<")||chone("|","&","^");
            case 1:
                return cheqal("*","/","%");
            case 0:
                if(check("++","--","-","!","~")){
                    return 1;
                }
                else if(check("+")){
                    return 0;
                }
                else if(check("&","*")){
                    return -1;
                }
                return cast();
            default:
                return 0;
            }
        }

        
    };

    template<class Tree,class Buf,class Str=Buf>
    Tree* example_expr_parse(PROJECT_NAME::Reader<Buf>& reader,int depth){
        auto rc=[](PROJECT_NAME::Reader<Buf>* r,auto& ctx){
            return PROJECT_NAME::expression<Tree,Str>(r,ctx);
        };        
        using CTX=Check<Tree,Buf,decltype(rc),Str,Str>;
        typename hard_expr::HardExprTree<Tree,Str,NodeKind,Buf,CTX> ctx(CTX(rc),depth);
        reader.readwhile(PROJECT_NAME::read_bintree<Buf,decltype(ctx),Str>,ctx);
        if(!ctx.result||ctx.cus.syntaxerr){
            delete ctx.result;
            return nullptr;
        }
        return ctx.result;
    }


}

