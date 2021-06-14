#pragma once
#include"base.h"
#include<string>
#include<assert.h>
namespace ast{
    enum class AstKind{
        unset,
        string,
        floatn,
        intn,
        boolean,
        null,
        identifier,
        op_scope,
        op_cond,
        op_add,
        op_sub,
        op_mul,
        op_div,
        op_mod,
        op_shl,
        op_shr,
        op_bitor,
        op_bitand,
        op_bitxor,
        op_equ,
        op_neq,
        op_lgr,
        op_llr,
        op_legr,
        op_lelr,
        op_logicalor,
        op_logicaland,
        op_assign,
        op_add_eq,
        op_sub_eq,
        op_mul_eq,
        op_div_eq,
        op_mod_eq,
        op_shl_eq,
        op_shr_eq,
        op_bitor_eq,
        op_bitand_eq,
        op_bitxor_eq,
        op_bitnot,
        op_logicalnot,
        op_addr,
        op_deref,
        op_init,
        op_call,
        op_index,
        
        if_stmt,
        block_stmt,
        for_stmt,
        var_stmt,
        func_literal,
        program
    };

    template<class N>
    struct AstList{
        N* node=nullptr;
        N* last=nullptr;
        void push_back(N* n){
            if(!n)return;
            if(!last){
                node=n;
                last=n;
            }
            else{
                last->next=n;
                last=n;
            }
        }
    };

    namespace type{
        struct Type;
    }

    struct AstToken{
        AstKind kind=AstKind::unset;
        std::string str;

        type::Type* type=nullptr;

        AstToken* left=nullptr;
        AstToken* right=nullptr;

        AstToken* cond=nullptr;

        AstToken* next=nullptr;

        std::string to_string(){
            switch (kind)
            {
            case AstKind::op_cond:
                assert(left&&right&&cond);
                return cond->to_string()+"?"+left->to_string()+":"+right->to_string();
            default:
                return (left?left->to_string():"")+str+(right?right->to_string():"");
            }
        }

        AstList<AstToken> arg;
        AstList<AstToken> block;
    };

   
    inline void delete_token(AstToken* tok){
        if(!tok)return;
        delete_(tok->left);
        delete_(tok->right);
        delete_(tok->cond);
        delete_(tok->arg.node);
        delete_(tok->block.node);
        delete_(tok->next);
        delete_(tok);
    }

    namespace type{
        enum class TypeKind{
            primary,
            pointer,
            reference,
            temporary,
            array,
            vector,
            function,
            generic,
            structs,
        };

        enum class ObjAttribute{
            noattr=0,
            constant=0x1,
            atomic=0x2,
            
        };

        struct Type;

        struct Object{
            std::string name;
            ObjAttribute attr=ObjAttribute::noattr;
            Type* type=nullptr;
            AstToken* init=nullptr;
            Object* next=nullptr;
        };

        struct Type{
            TypeKind kind;
            std::string name;
            Type* base=nullptr;
            AstToken* token=nullptr;

            Type* ptrto=nullptr;
            Type* tmpof=nullptr;
            Type* refof=nullptr;
            Type* vecof=nullptr;

            AstList<Object> param;

            std::string to_string(){
                switch(kind){
                case TypeKind::primary:
                    return name;
                case TypeKind::pointer:
                    assert(base);
                    return "*"+base->to_string();
                case TypeKind::reference:
                    assert(base);
                    return "&"+base->to_string();
                case TypeKind::temporary:
                    assert(base);
                    return "&&"+base->to_string();
                case TypeKind::array:
                    assert(base&&token);
                    return "["+token->to_string()+"]"+base->to_string();
                default:
                    return "";
                }
            }
        };

        struct TypePool{
            std::vector<Type*> pool;
            //size_t pos=1;

            void delall(){
                for(auto r:pool){
                    ast::delete_(r);
                }
                pool.resize(0);
            }

            Type* new_Type(){
                auto ref=ast::new_<Type>();
                //pos++;
                pool.push_back(ref);
                return ref;
            }
            

            Type* pointer_to(Type* t){
                assert(t);
                if(!t->ptrto){
                    t->ptrto=new_Type();
                    t->ptrto->kind=TypeKind::pointer;
                    t->ptrto->base=t;
                }
                return t->ptrto;
            }

            Type* reference_to(Type* t){
                assert(t);
                if(!t->refof){
                    t->refof=new_Type();
                    t->refof->kind=TypeKind::reference;
                    t->refof->base=t;
                }
                return t->refof;
            }

            Type* temporary_of(Type* t){
                assert(t);
                if(!t->tmpof){
                    t->tmpof=new_Type();
                    t->tmpof->kind=TypeKind::temporary;
                    t->tmpof->base=t;
                }
                return t->tmpof;
            }

            Type* vector_of(Type* t){
                assert(t);
                if(!t->vecof){
                    t->vecof=new_Type();
                    t->vecof->kind=TypeKind::vector;
                    t->vecof->base=t;
                }
                return t->vecof;
            }

            Type* array_of(Type* t,AstToken* tok){
                assert(t&&tok);
                Type* ret=new_Type();
                ret->token=tok;
                ret->base=t;
                ret->kind=TypeKind::array;
                return ret;
            }

            Type* function(Type* ret,AstList<Object>& param){
                Type* t=new_Type();
                t->base=ret;
                t->param=std::move(param);
                t->kind=TypeKind::function;
                return t;
            }

            Type* generic(){
                Type* t=new_Type();
                t->kind=TypeKind::generic;
                return t;
            }

            Type* struct_(){
                Type*t =new_Type();
                t->kind=TypeKind::structs;
                return t;
            }
        };
    }


}