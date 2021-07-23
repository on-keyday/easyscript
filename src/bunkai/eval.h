#pragma once
#include<assert.h>
#include"base.h"
#include"ast_obj.h"
#include<reader.h>
#include<basic_helper.h>
#include<optional>
namespace eval{
    

    struct Evaluator{
        using TyPool=ast::type::TypePool;
        TyPool& pool;
        using SToken=ast::SAstToken;
        using AstKind=ast::AstKind;


        template<class E>
        E opeval(AstKind kind,E& l,E& r){
#define opcase(name,op) case AstKind::name:return *l op *r;
            switch (kind)
            {
            opcase(op_add,+)
            opcase(op_sub,-)
            opcase(op_mul,*)
            opcase(op_div,/)
            opcase(op_equ,==)
            opcase(op_neq,!=)
            opcase(op_lelr,<=)
            opcase(op_legr,>=)
            opcase(op_llr,<)
            opcase(op_lgr,>)
            opcase(op_logicalor,||)
            opcase(op_logicaland,&&)
            default:break;
            }
            if CONSTEXPRIF (!std::is_floating_point_v<E>){
                switch (kind)
                {
                opcase(op_shl,<<)
                opcase(op_shr,>>)
                opcase(op_bitor,|)
                opcase(op_bitand,&)
                opcase(op_bitxor,^)
                default:break;
                }
            }
            return std::nullopt;
#undef opcase
        }

        template<class T>
        static bool getint(T& t,SToken in){
            PROJECT_NAME::Reader<PROJECT_NAME::Refer<std::string>> tes(in->str);
            int radix=10;
            if(tes.expect("0x")){
                radix=16;
            }
            else if(tes.expect("0b")){
                radix=2;
            }
            else if(tes.expect("0")){
                if(tes.ceof()){
                    radix=0;
                }
                else{
                    radix=8;
                }
            }
            if(radix){
                if(!PROJECT_NAME::parse_int(in->str,t,radix)){
                    return false;
                }
            }
            else{
                t=0;
            }
            return true;
        }
        
        std::optional<long long> evalint(SToken in){
            if(!in)return std::nullopt;
            if(in->kind==AstKind::intn){
                long long i;
                if(!getint(i,in))return std::nullopt;
                return i;
            }
            else if(!(in->kind&AstKind::bin_op)){
                return std::nullopt;
            }
            auto lres=evalint(in->left);
            auto rres=evalint(in->right);
            if(lres==std::nullopt||rres==std::nullopt){
                return std::nullopt;
            }
            return opeval(in->kind,lres,rres);
        }

        bool error(const char* s){
            return false;
        }

        bool eval(SToken tok,const char** err){
            auto error=[&](auto in){
                if(err){
                    *err=in;
                }
                return false;
            };
            if(!tok)return error("invalid argument");
            if(tok->kind!=AstKind::program)return error("invalid token kind");
        }

        bool evalblock(ast::AstList<ast::AstToken>& block){
            foreach_node(p,block){
                if(p->kind&AstKind::stmt){

                }
                switch (p->kind)
                {
                case  AstKind::op_init:{
                    initdef(p);
                }
                default:
                    assert(false);
                }
            }
        }

        bool evalstmt(SToken tok){

        }

        bool evalexpr(SToken tok){

        }

        bool initdef(SToken in){
            if(in->kind==AstKind::var_stmt){
                
            }
            else if(in->kind==AstKind::op_init){
                if(!in->left)return error("init:invalid tree structure");
                if(in->left->kind!=AstKind::identifier){
                    return error("init:invalid type kind");
                }
                auto obj=pool.register_var(in->left->str);
                if(in->right){
                    evalexpr(in->right);
                }
                assert(in->right&&in->right->type);
                obj->type=in->right->type;
                obj->init=in->right;
                return true;
            }
            assert(false);
            return error("");
        }

        Evaluator(TyPool& in):pool(in){}
    };

}