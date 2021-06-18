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
        op_member,
        func_literal,

        if_stmt,
        block_stmt,
        for_stmt,
        var_stmt,
        func_stmt,
        decl_stmt,
        return_stmt,
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

        union{
            AstList<AstToken> param=AstList<AstToken>();
            AstList<AstToken> block;
        };

        AstToken(){}
    };

   
    inline void delete_token(AstToken* tok){
        if(!tok)return;
        delete_token(tok->left);
        delete_token(tok->right);
        delete_token(tok->cond);
        //delete_token(tok->arg.node);
        delete_token(tok->block.node);
        delete_token(tok->next);
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
            constant,
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

            bool generic_relative=false;

            Type* ptrto=nullptr;
            Type* tmpof=nullptr;
            Type* refof=nullptr;
            Type* vecof=nullptr;
            Type* constof=nullptr;

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

        struct Scope{
            Scope* prev=nullptr;
            std::map<std::string,Scope> children;
            std::map<std::string,Object*> functions;
            std::map<std::string,Object*> variables;
            std::map<std::string,Type*> types;
            bool tmp=false;
            ~Scope(){
                assert(!tmp&&"tmpscope not leaved.");
            }
        };

        struct TypePool{
            std::vector<Type*> pool;
            std::vector<Object*> obj;
            Scope globalscope;
            Scope* currentscope=&globalscope;

            void delall(){
                for(auto r:pool){
                    ast::delete_(r);
                }
                for(auto r:obj){
                    ast::delete_(r);
                }
                pool.resize(0);
                obj.resize(0);
            }

            Object* new_Object(){
                auto ref=ast::new_<Object>();
                obj.push_back(ref);
                return ref;
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
                    t->ptrto->generic_relative=t->generic_relative;
                }
                return t->ptrto;
            }

            Type* reference_to(Type* t){
                assert(t);
                if(!t->refof){
                    t->refof=new_Type();
                    t->refof->kind=TypeKind::reference;
                    t->refof->base=t;
                    t->refof->generic_relative=t->generic_relative;
                }
                return t->refof;
            }

            Type* temporary_of(Type* t){
                assert(t);
                if(!t->tmpof){
                    t->tmpof=new_Type();
                    t->tmpof->kind=TypeKind::temporary;
                    t->tmpof->base=t;
                    t->tmpof->generic_relative=t->generic_relative;
                }
                return t->tmpof;
            }

            Type* vector_of(Type* t){
                assert(t);
                if(!t->vecof){
                    t->vecof=new_Type();
                    t->vecof->kind=TypeKind::vector;
                    t->vecof->base=t;
                    t->vecof->generic_relative=t->generic_relative;
                }
                return t->vecof;
            }

            Type* array_of(Type* t,AstToken* tok){
                assert(t&&tok);
                Type* ret=new_Type();
                ret->token=tok;
                ret->base=t;
                ret->kind=TypeKind::array;
                ret->generic_relative=t->generic_relative;
                return ret;
            }

            Type* const_of(Type* t){
                assert(t);
                if(t->kind==TypeKind::constant){
                    throw "type:double const is invalid";
                }
                if(!t->constof){
                    t->constof=new_Type();
                    t->constof->kind=TypeKind::constant;
                    t->constof->base=t;
                    t->constof->generic_relative=t->generic_relative;
                }
                return t->constof;
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
                t->generic_relative=true;
                return t;
            }

            Type* struct_(){
                Type*t =new_Type();
                t->kind=TypeKind::structs;
                return t;
            }

            Type* keyword(const char* t){
                throw "keyword:unimplemented";
            }

            Type* identifier(const char* t){
                throw "identifier:unimplemented";
            }

            bool enterscope(const std::string& name){
                assert(currentscope);
                if(!currentscope->children.count(name)){
                    return false;
                }
                currentscope=&currentscope->children[name];
                return true;           
            }

            bool leavescope(const std::string& name){
                assert(currentscope);
                if(!currentscope->prev){
                    return false;
                }
                currentscope->tmp=false;
                currentscope=currentscope->prev;
                return true;
            }

            bool makescope(const std::string& name){
                assert(currentscope);
                if(currentscope->children.count(name)){
                    return false;
                }
                auto& ref=currentscope->children[name];
                ref.prev=currentscope;
                return true;
            }

            bool tmpscope(Scope& scope,AstList<Object>* var,AstList<Object>* func){
                if(var){
                    for(auto p=var->node;p;p=p->next){
                        if(scope.variables.count(p->name)){
                            throw "scope:variablee name conflict";
                        }
                        scope.variables[p->name]=p;
                    }
                }
                if(func){
                    for(auto p=func->node;p;p=p->next){
                        if(scope.functions.count(p->name)){
                            throw "scope:function name conflict";
                        }
                        scope.functions[p->name]=p;
                    }
                }
                scope.tmp=true;
                scope.prev=currentscope;
                currentscope=&scope;
                return true;
            }
        };
    }


}