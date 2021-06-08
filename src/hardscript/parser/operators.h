#pragma once
#include"hard_tree.h"
namespace node{

    enum class NodeKind {
        //unknown
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
        op_lgr, //a > b
        op_llr, //a < b
        op_lger, //a <= b
        op_ller, //a >= b
        op_rsh, //a >> b
        op_lsh, //a << b
        op_call, //a()
        op_access, //a[b]
        op_block, //{}
        op_incr, //a++ ++a
        op_decr, //a-- --a
        op_dref, //*a
        op_addr, //&a
        op_assign, //a = b
        op_add_assign, //a += b
        op_sub_assign, //a -= b
        op_mul_assign, //a *= b
        op_div_assign, //a /= b
        op_mod_assign, //a %= b
        op_band_assign, //a &= b
        op_bor_assign, //a |= b
        op_bxor_assign, //a ^= b
        op_init, //a := b
    };


    inline const char* operator_to_str(NodeKind kind){
#define CASE_LAB_OPS_TO_STR_NODE(x,s) case NodeKind::x:return s;
        switch (kind)
        {
        CASE_LAB_OPS_TO_STR_NODE(op_add,"+")
        CASE_LAB_OPS_TO_STR_NODE(op_sub,"-")
        CASE_LAB_OPS_TO_STR_NODE(op_mul,"*")
        CASE_LAB_OPS_TO_STR_NODE(op_div,"/")
        CASE_LAB_OPS_TO_STR_NODE(op_mod,"%")
        CASE_LAB_OPS_TO_STR_NODE(op_lor,"||")
        CASE_LAB_OPS_TO_STR_NODE(op_land,"&&")
        CASE_LAB_OPS_TO_STR_NODE(op_bor,"|")
        CASE_LAB_OPS_TO_STR_NODE(op_band,"&")
        CASE_LAB_OPS_TO_STR_NODE(op_bxor,"^")
        CASE_LAB_OPS_TO_STR_NODE(op_cast,"cast")
        CASE_LAB_OPS_TO_STR_NODE(op_cond,"?:")
        CASE_LAB_OPS_TO_STR_NODE(op_equ,"==")
        CASE_LAB_OPS_TO_STR_NODE(op_neq,"!=")
        CASE_LAB_OPS_TO_STR_NODE(op_lgr,">")
        CASE_LAB_OPS_TO_STR_NODE(op_llr,"<")
        CASE_LAB_OPS_TO_STR_NODE(op_lger,">=")
        CASE_LAB_OPS_TO_STR_NODE(op_ller,"<=")
        CASE_LAB_OPS_TO_STR_NODE(op_lsh,"<<")
        CASE_LAB_OPS_TO_STR_NODE(op_rsh,">>")
        CASE_LAB_OPS_TO_STR_NODE(op_call,"()")
        CASE_LAB_OPS_TO_STR_NODE(op_access,"[]")
        CASE_LAB_OPS_TO_STR_NODE(op_block,"{}")
        CASE_LAB_OPS_TO_STR_NODE(op_incr,"++")
        CASE_LAB_OPS_TO_STR_NODE(op_decr,"--")
        CASE_LAB_OPS_TO_STR_NODE(op_dref,"*")
        CASE_LAB_OPS_TO_STR_NODE(op_addr,"&")
        CASE_LAB_OPS_TO_STR_NODE(op_assign,"=")
        CASE_LAB_OPS_TO_STR_NODE(op_add_assign,"+=")
        CASE_LAB_OPS_TO_STR_NODE(op_sub_assign,"-=")
        CASE_LAB_OPS_TO_STR_NODE(op_mul_assign,"*=")
        CASE_LAB_OPS_TO_STR_NODE(op_div_assign,"/=")
        CASE_LAB_OPS_TO_STR_NODE(op_mod_assign,"%=")
        CASE_LAB_OPS_TO_STR_NODE(op_band_assign,"&=")
        CASE_LAB_OPS_TO_STR_NODE(op_bor_assign,"|=")
        CASE_LAB_OPS_TO_STR_NODE(op_bxor_assign,"^=")
        CASE_LAB_OPS_TO_STR_NODE(op_init,":=")
        default:return "";
        }
    }

#undef CASE_LAB_OPS_TO_STR_NODE


    template<class String>
    NodeKind str_to_nodekind(const String& str,bool has_left){
#define CASE_LAB_OPS_TO_STR_NODE(n,s) if(str==s){return NodeKind::n}
        CASE_LAB_OPS_TO_STR_NODE(op_add,"+")
        CASE_LAB_OPS_TO_STR_NODE(op_sub,"-")
        if(has_left)CASE_LAB_OPS_TO_STR_NODE(op_mul,"*")
        CASE_LAB_OPS_TO_STR_NODE(op_div,"/")
        CASE_LAB_OPS_TO_STR_NODE(op_mod,"%")
        CASE_LAB_OPS_TO_STR_NODE(op_lor,"||")
        CASE_LAB_OPS_TO_STR_NODE(op_land,"&&")
        CASE_LAB_OPS_TO_STR_NODE(op_bor,"|")
        if(has_left)CASE_LAB_OPS_TO_STR_NODE(op_band,"&")
        CASE_LAB_OPS_TO_STR_NODE(op_bxor,"^")
        CASE_LAB_OPS_TO_STR_NODE(op_cast,"cast")
        CASE_LAB_OPS_TO_STR_NODE(op_cond,"?:")
        CASE_LAB_OPS_TO_STR_NODE(op_equ,"==")
        CASE_LAB_OPS_TO_STR_NODE(op_neq,"!=")
        CASE_LAB_OPS_TO_STR_NODE(op_lgr,">")
        CASE_LAB_OPS_TO_STR_NODE(op_llr,"<")
        CASE_LAB_OPS_TO_STR_NODE(op_lger,">=")
        CASE_LAB_OPS_TO_STR_NODE(op_ller,"<=")
        CASE_LAB_OPS_TO_STR_NODE(op_lsh,"<<")
        CASE_LAB_OPS_TO_STR_NODE(op_rsh,">>")
        CASE_LAB_OPS_TO_STR_NODE(op_call,"()")
        CASE_LAB_OPS_TO_STR_NODE(op_access,"[]")
        CASE_LAB_OPS_TO_STR_NODE(op_block,"{}")
        CASE_LAB_OPS_TO_STR_NODE(op_incr,"++")
        CASE_LAB_OPS_TO_STR_NODE(op_decr,"--")
        CASE_LAB_OPS_TO_STR_NODE(op_dref,"*")
        CASE_LAB_OPS_TO_STR_NODE(op_addr,"&")
        CASE_LAB_OPS_TO_STR_NODE(op_assign,"=")
        CASE_LAB_OPS_TO_STR_NODE(op_add_assign,"+=")
        CASE_LAB_OPS_TO_STR_NODE(op_sub_assign,"-=")
        CASE_LAB_OPS_TO_STR_NODE(op_mul_assign,"*=")
        CASE_LAB_OPS_TO_STR_NODE(op_div_assign,"/=")
        CASE_LAB_OPS_TO_STR_NODE(op_mod_assign,"%=")
        CASE_LAB_OPS_TO_STR_NODE(op_band_assign,"&=")
        CASE_LAB_OPS_TO_STR_NODE(op_bor_assign,"|=")
        CASE_LAB_OPS_TO_STR_NODE(op_bxor_assign,"^=")
        CASE_LAB_OPS_TO_STR_NODE(op_init,":=")
        return NodeKind::unknown;
    }

#undef CASE_LAB_OPS_TO_STR_NODE

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

