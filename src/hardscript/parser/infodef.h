#pragma once

#include<basic_tree.h>
#include"operators.h"

namespace info{

    template<class N>
    struct NodeList{
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

    template<class Str>
    struct Error{
        Str msg;
        void error(const Str& in){
            msg+=in;
            msg+="\n";
        }
    };

    //expr has infomation of expression
    template<class Str>
    struct Expr{
        static Expr zero={"0",node::NodeKind::obj_int,true};
        static Expr one={"1",node::NodeKind::obj_int,true};
        static Expr true_={"true",node::NodeKind::obj_bool,true};
        static Expr false_={"false",node::NodeKind::obj_bool,true};

        Str symbol=Str();
        node::NodeKind kind;

        bool no_delete=false;

        Expr* left=nullptr;
        Expr* right=nullptr;
        Expr* cond=nullptr;
        
        
        Expr* next=nullptr;

        NodeList<Expr> arg;
    };

    template<class T>
    void kill(T* s){
        if(!s||s->no_delete)return;
        delete s;
    }

    //value has infomation of value
    struct Value{
    };

    

    

    //type has infomation of type
    struct Type{

    };


    //object has infomation of type and value
    struct Object{

    };

    enum class ScopeKind{
        block,
        space
    };

    //scope has infomation of namespace or block
    struct Scope{
        ScopeKind kind;
        Scope* parent=nullptr;

    };

    template<class Str>
    struct Expector;

    template<class Str,class Buf>
    Expr<Str>* cond(Reader<Buf>& r,Expextor<Str>& ctx){
        Expr<Str>* ret=PROJECT_NAME::bin<Str>(r,ctx);
        if(r.expect("?")){
            decltype(ret) than=nullptr,els=nullptr,tmp=nullptr;
            than=cond<Str>(r,ctx);
            if(!than)return nullptr;
            if(!r.expect(":")){
                ctx.err.error("cond:expected : but not");
            }
            if(!(els=cond<Str>(r,ctx))){

            }
              !(tmp=ctx.make_tree("?:",nullptr,nullptr))){
                delete els;
                delete than;
                delete ret;
                return nullptr;
            }
            tmp->cond=ret;
            tmp->than=than;
            tmp->els=els;
            ret=tmp;
        }
    } 
    

    template<class Str>
    struct Expector{
        using Expr=Expr<Str>;
        Error err;

        Expr* make(){
            try{
                return new Expr();
            }
            catch(...){
                err.error("memory error");
                return nullptr;
            }
        }

        Expr* make_tree(const Str& symbol,Expr* left,Expr* right){
            Expr* ret=make();
            if(!ret)return nullptr;
            ret->kind=node::str_to_nodekind(symbol,(bool)left);
            ret->left=left;
            ret->right=right;
            return ret;
        }

        

        void delete_tree(Expr* p){
            kill(p);
        }
        

        template<class Buf>
        Expr* unary(PROJECT_NAME::Reader<Buf>* r){
            Str expected=Str();
            if(r->expect("&",expected)||r->expect("*",expected)){
                auto ps=prim(r);
                if(!ps)return nullptr;
                return make_tree(expected,nullptr,ps);
            }
            if(r->expect("++",expected)||r->expect("--",expected)){
                auto ps=prim(r);
                if(!ps)return nullptr;
                return expected=="++"?make_tree("+=",ps,&Expr::one):make_tree("-=",ps.&Expr::one);
            }
            else if(r->expect("+")){
                return prim(r);
            }
            else if(r->expect("-")){
                auto ps=prim(r);
                if(!ps)return nullptr;
                return make_tree("-",&Expr::zero,ps);
            }
            else if(r->expect("!",expected)||r->expect("~",expected)){
                auto ps=unary(r);
                if(!ps)return nullptr;
                return make_tree(expected,nullptr,ps);
            }
            return prim(r);
        }

        template<class Buf>
        Expr* prim(PROJECT_NAME::Reader<Buf>* r){
            Expr* ret=nullptr;
            if(self->expect("(")){
                ret=parse_arg(nullptr,self,"()",")");
                if(!ret)return nullptr;
            }
            else if(self->expect("[")){
                ret=parse_arg(nullptr,r,"[]","]");
                if(!ret)return nullptr;
            }
            else if(self->expect("new",PROJECT_NAME::is_c_id_usable)){
                Str id;
                if(!id_read(self,id))return nullptr;
                auto tmp=make_tree(id);
                if(!tmp){
                    cus.error("new:memory is full");
                    return nullptr;
                }
                if(!self->expect("(")){
                    cus.error("new:expected ( but not");
                    delete tmp;
                    return nullptr;
                }
                auto obj=parse_arg(tmp,self,"()",")");
                if(!obj)return nullptr;
                ret=make_tree("new",nullptr,obj,node::NodeKind::obj_new);
                if(!ret){
                    cus.error("new:memory is full");
                    delete obj;
                    return nullptr;
                }
            }
            else if(expect_common_object(ret,r)){
                if(!ret)return nullptr;
            }
            else if(self->expect("func",PROJECT_NAME::is_c_id_usable)){
                
            }
            else if(PROJECT_NAME::is_c_id_top_usable(self->achar())){
                Str id;
                if(!id_read(self,id))return nullptr;
                ret=make_tree(id,node::NodeKind::obj_id);
                if(!ret)return nullptr;
            }
            return ret;
        }

        Expr* make_tree(const Str& symbol,node::NodeKind kind){
            Expr* ret=make();
            if(!ret)return nullptr;
            ret->kind=kind;
            return ret;
        }


        template<class Buf>
        bool expect_common_object(Expr*& ret,PROJECT_NAME::Reader<Buf>* self){
            Str expected;
            if(self->ahead("\"")||self->ahead("'")){
                Str str;
                if(!self->string(str))return false;
                ret=make_tree(str,node::NodeKind::obj_string);
            }
            else if(PROJECT_NAME::is_digit(self->achar())){
                Str num;
                PROJECT_NAME::NumberContext<char> ctx;
                self->readwhile(num,PROJECT_NAME::number,&ctx);
                if(!ctx.succeed)return nullptr;
                if(ctx.floatf){
                    ret=make_tree(num,node::NodeKind::obj_float);
                }
                else{
                    ret=make_tree(num,node::NodeKind::obj_int);
                }
            }
            else if(r->expect("true",expected,PROJECT_NAME::is_c_id_usable)||
                    r->expect("false",expected,PROJECT_NAME::is_c_id_usable)){
                return expected=="true"?&Expr::true_:&Expr::false_;
            }
            else{
                return false;
            }
            return true;
        }

        template<class Buf>
        Expr* after(PROJECT_NAME::Reader<Buf>* r){

        }


        template<class Buf>
        Expr* arg(PROJECT_NAME::Reader<Buf>* r,const char* sym,const char* end){
            Expr* ret=make_tree(sym,nullptr,nullptr);
            if(r->expect(end)){
                return ret;
            }  
            while(true){
                
            }
        }
    };
}