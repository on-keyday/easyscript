#pragma once
#include"base.h"
#include<string>
#include<assert.h>
#include<memory>
namespace ast{
    enum class AstKind{
        unset,

        object    =0x001'00,
        special_op=0x002'00,
        bin_op    =0x004'00,
        assign_op =0x008'00,
        unary_op  =0x010'00,
        after_op  =0x020'00,
        stmt      =0x040'00,
        other     =0x080'00,


        string=object+1,
        floatn,
        intn,
        boolean,
        null,
        identifier,
        
        op_scope=special_op+1,
        op_cond,

        op_add=bin_op+1,
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


        op_assign=assign_op+1,
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
        op_init,

        op_bitnot=unary_op+1,
        op_logicalnot,
        op_cast,
        op_addr,
        op_deref,
        

        op_call=after_op+1,
        op_index,
        op_member,

        

        func_literal=stmt+1,
        init_var,
        init_border,
        if_stmt,
        block_stmt,
        for_stmt,
        var_stmt,
        func_stmt,
        decl_stmt,
        return_stmt,
        switch_stmt,
        case_stmt,
        default_stmt,
        sw_init_stmt,
        namespace_stmt,
        type_stmt,

        program=other+1,
    };

    std::underlying_type_t<AstKind> operator&(AstKind l,AstKind r){
        using basety=std::underlying_type_t<AstKind>;
        return (static_cast<basety>(l)&static_cast<basety>(r));
    }


    

#define foreach_node(name,list) for(auto name=list.node;name;name=name->next)

    namespace type{
        struct Type;
        struct Scope;

        using SType=std::shared_ptr<Type>;
        inline void deltybase(type::SType& t);
    }
    struct AstToken;
    using SAstToken=std::shared_ptr<AstToken>;

    template<class N>
    struct AstList{
        std::shared_ptr<N> node=nullptr;
        std::shared_ptr<N> last=nullptr;
        void push_back(std::shared_ptr<N>&& n){
            if(!n)return;
            if(!last){
                node=std::move(n);
                last=node;
            }
            else{
                last->next=std::move(n);
                last=last->next;
            }
        }

        void push_back(const std::shared_ptr<N>& n){
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
        ~AstList(){}

        AstList& operator=(AstList&& in){
            node=std::move(in.node);
            last=std::move(in.last);
            return *this;
        }
    };

   

    struct AstToken{
        AstKind kind=AstKind::unset;
        std::string str;

        type::SType type=nullptr;

        SAstToken left=nullptr;
        SAstToken right=nullptr;

        SAstToken cond=nullptr;

        SAstToken next=nullptr;

        type::Scope* scope=nullptr;
        /*
        std::string to_string(){
            switch (kind)
            {
            case AstKind::op_cond:
                assert(left&&right&&cond);
                return cond->to_string()+"?"+left->to_string()+":"+right->to_string();
            default:
                return (left?left->to_string():"")+str+(right?right->to_string():"");
            }
        }*/

        union{
            AstList<AstToken> param=AstList<AstToken>();
            AstList<AstToken> block;
        };

        AstToken(){}
        ~AstToken(){
            param.~AstList();
            type::deltybase(type);
        }
    };

    

    
    /*
    inline void delete_token(SAstToken tok){
        if(!tok)return;
        delete_token(tok->left);
        delete_token(tok->right);
        delete_token(tok->cond);
        //delete_token(tok->arg.node);
        delete_token(tok->block.node);
        delete_token(tok->next);
        delete_(tok);
    }*/

    namespace type{
        enum class TypeKind{
            primary,
            alias,
            pointer,
            reference,
            temporary,
            array,
            vector,
            constant,
            function,
            generic,
            structs,
            typeofexpr,
        };

        enum class ObjAttribute{
            noattr=0,
            atomic=0x1,
            
        };

        struct Object;
        using SObject=std::shared_ptr<Object>;

        struct Object{
            std::string name;
            ObjAttribute attr=ObjAttribute::noattr;
            SType type=nullptr;
            SAstToken init=nullptr;
            SObject next=nullptr;
            ~Object(){
                deltybase(type);
            }
        };


        struct Type{
            TypeKind kind;
            std::string name;
            SType base=nullptr;
            SAstToken token=nullptr;

            bool generic_relative=false;

            SType ptrto=nullptr;
            SType tmpof=nullptr;
            SType refof=nullptr;
            SType vecof=nullptr;
            SType constof=nullptr;

            AstList<Object> param;

            /*
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
            }*/
            ~Type(){

            }
        };

        inline void deltybase(type::SType& t){
            if(t){
                t->base.reset();
                t->ptrto.reset();
                t->refof.reset();
                t->tmpof.reset();
                t->vecof.reset();
                t->constof.reset();
            }
        }

        struct Scope{
            Scope* prev=nullptr;
            std::map<std::string,Scope> children;
            std::map<std::string,SObject> functions;
            std::map<std::string,SObject> variables;
            std::map<std::string,SType> types;
            bool tmp=false;
            ~Scope(){
                assert(!tmp&&"tmpscope not leaved.");
            }
        };

        struct TypePool{
            //std::vector<SType> pool;
            //std::vector<SObject> obj;
            std::map<std::string,SType> keywords; 
            Scope globalscope;
            Scope* currentscope=&globalscope;

            void delall(){
                /*for(auto r:pool){
                    ast::delete_(r);
                }
                for(auto r:obj){
                    ast::delete_(r);
                }
                pool.resize(0);
                obj.resize(0);*/
            }

            SObject new_Object(){
                auto ref=ast::new_<Object>();
                //obj.push_back(ref);
                return SObject(ref,ast::delete_<Object>);
            }

            SType new_Type(){
                auto ref=ast::new_<Type>();
                //pos++;
                //pool.push_back(ref);
                return SType(ref,ast::delete_<Type>);
            }
            

            SType pointer_to(SType t){
                assert(t);
                if(!t->ptrto){
                    t->ptrto=new_Type();
                    t->ptrto->kind=TypeKind::pointer;
                    t->ptrto->base=t;
                    t->ptrto->generic_relative=t->generic_relative;
                }
                return t->ptrto;
            }

            SType reference_to(SType t){
                assert(t);
                if(!t->refof){
                    t->refof=new_Type();
                    t->refof->kind=TypeKind::reference;
                    t->refof->base=t;
                    t->refof->generic_relative=t->generic_relative;
                }
                return t->refof;
            }

            SType temporary_of(SType t){
                assert(t);
                if(!t->tmpof){
                    t->tmpof=new_Type();
                    t->tmpof->kind=TypeKind::temporary;
                    t->tmpof->base=t;
                    t->tmpof->generic_relative=t->generic_relative;
                }
                return t->tmpof;
            }

            SType vector_of(SType t){
                assert(t);
                if(!t->vecof){
                    t->vecof=new_Type();
                    t->vecof->kind=TypeKind::vector;
                    t->vecof->base=t;
                    t->vecof->generic_relative=t->generic_relative;
                }
                return t->vecof;
            }

            SType array_of(SType t,SAstToken tok){
                assert(t&&tok);
                SType ret=new_Type();
                ret->token=tok;
                ret->base=t;
                ret->kind=TypeKind::array;
                ret->generic_relative=t->generic_relative;
                return ret;
            }

            SType const_of(SType t){
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

            SType function(SType ret,AstList<Object>& param){
                SType t=new_Type();
                t->base=ret;
                t->param=std::move(param);
                t->kind=TypeKind::function;
                return t;
            }

            SType generic(){
                SType t=new_Type();
                t->kind=TypeKind::generic;
                t->generic_relative=true;
                return t;
            }

            SType struct_(){
                SType t=new_Type();
                t->kind=TypeKind::structs;
                return t;
            }

            SType keyword(const char* t){
                PROJECT_NAME::Reader<const char*> re(t);
                if(re.expect("int",PROJECT_NAME::is_c_id_usable)){
                    t="int32";
                }
                else if(re.expect("uint",PROJECT_NAME::is_c_id_usable)){
                    t="uint32";
                }
                if(!keywords.count(t)){
                    auto& ref=keywords[t];
                    ref=std::move(new_Type());
                    ref->name=t;
                    return ref;
                }
                return keywords[t];
            }

            SType typeofexpr(){
                SType ret=new_Type();
                ret->kind=TypeKind::typeofexpr;
                return ret;
            }

            SType identifier(Scope* scope,const char* t){
                assert(scope&&t);
                if(!scope->types.count(t)){
                    throw "identifier:undecleared type name";
                }
                return scope->types[t];
            }

            SType alias(const char* name){
                SType ret=new_Type();
                ret->kind=TypeKind::alias;
                ret->name=name;
                return ret;
            }

            Scope* get_scope(Scope* rel,const char* t){
                if(!t)return currentscope;
                PROJECT_NAME::Reader<const char*> re(t);
                if(re.expect("::")){
                    return &globalscope;
                }
                bool root=false;
                if(!rel){
                    rel=currentscope;
                    root=true;
                }
                do{
                    if(rel->children.count(t)){
                        return &rel->children[t];
                    }
                    rel=rel->prev;
                }while(root&&rel);
                return nullptr;
            }

            bool register_type(const std::string& name,SType t){
                assert(currentscope);
                if(currentscope->types.count(name)){
                    throw "type:type name conflict";
                }
                currentscope->types[name]=t;
                return true;
            }

            SObject register_var(const std::string& name){
                assert(currentscope);
                if(currentscope->variables.count(name)){
                    throw "var:variable name conflict";
                }
                auto o=new_Object();
                currentscope->variables[name]=o;
                o->name=name;
                return o;
            }

            bool enterscope(const std::string& name){
                assert(currentscope);
                if(!currentscope->children.count(name)){
                    return false;
                }
                currentscope=&currentscope->children[name];
                return true;           
            }

            bool leavescope(){
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

            size_t count=0;

            Scope* unnamed_scope(const char* prefix){
                assert(currentscope);
                auto d=std::string(prefix)+"!"+std::to_string(count)+"!";
                count++;
                if(!makescope(d)||!enterscope(d)){
                    throw "unnamed scope:compiler is broken";
                }
                return currentscope;
            }
            ~TypePool(){}
        };
    }


}