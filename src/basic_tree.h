/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"reader.h"

namespace PROJECT_NAME{
    
    template<class Tree,class Buf,class Ctx>
    Tree* primary(Reader<Buf>* self,Ctx& ctx){
        Tree* ret=ctx.primary_tree(self);
        if(!ret)return nullptr;
        return ctx.after_tree(ret,self);
    }

    template<class Tree,class Buf,class Ctx>
    Tree* unary(Reader<Buf>* self,Ctx& ctx){
        using Char=typename Reader<Buf>::char_type;
        const Char* expected=nullptr;
        if(ctx.expect(self,0,expected)){
            auto tmp=primary<Tree>(self,ctx);
            if(!tmp)return nullptr;
            auto ret=ctx.make_tree(expected,nullptr,tmp);
            if(!ret){
                ctx.delete_tree(tmp);
                return nullptr;
            }
            return ret;
        }
        else if(ctx.expect(self,-1,expected)){
            auto tmp=unary<Tree>(self,ctx);
            if(!tmp)return nullptr;
            auto ret=ctx.make_tree(expected,nullptr,tmp);
            if(!ret){
                ctx.delete_tree(tmp);
                return nullptr;
            }
            return ret;
        }
        else{
            return primary<Tree>(self,ctx);
        }
    }

    template<class Tree,class Buf,class Ctx>
    Tree* bin(Reader<Buf>* self,Ctx& ctx,int depth){
        using Char=typename Reader<Buf>::char_type;
        if(depth<=0)return unary<Tree>(self,ctx);
        Tree* ret=bin<Tree>(self,ctx,depth-1);
        if(!ret)return nullptr;
        const Char* expected=nullptr;
        while(ctx.expect(self,depth,expected)){
            auto tmp=bin<Tree>(self,ctx,depth-1);
            if(!tmp){
                ctx.delete_tree(ret);
                return nullptr;
            }
            auto rep=ctx.make_tree(expected,ret,tmp);
            if(!rep){
                ctx.delete_tree(tmp);
                ctx.delete_tree(ret);
                return nullptr;
            }
            ret=rep;
        }
        return ret;
    }

    template<class Tree,class Buf,class Ctx>
    Tree* expression(Reader<Buf>* self,Ctx& ctx){
        return bin<Tree>(self,ctx,ctx.depthmax);
    }

    template<class Buf,class Ctx>
    bool parse_expr(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Tree=std::remove_pointer_t<remove_cv_ref<decltype(ctx.result)>>;
        if(!begin)return true;
        if(!self)return true;
        ctx.result=expression<Tree>(self,ctx);
        return false;
    }
/*
    template<class Tree,class Kind,class Buf>
    struct ExampleTreeCtx{
        Tree* result=nullptr;

        template<class Str>
        Tree* make_tree(Str symbol,Tree*left,Tree* right,Kind kind=Kind::unknown){
            Tree* ret=nullptr;
            try{
                ret=new Tree();
            }
            catch(...){
                return nullptr;
            }
            ret.symbol=symbol;
            ret.left=left;
            ret.right=right;
            ret.kind=kind;
            return ret;
        }

        void delete_tree(Tree* tree){
            delete tree;
        }

        bool op_expect(Reader<Buf>*,const char*&){return false;}

        template<class First,class... Other>
        bool op_expect(Reader<Buf>* self,const char*& expected,First first,Other... other){
            return self->expect(first,expected)||op_expect(self,expected,other...);
        }

        int depthmax=7;
    
        bool expect(Reader<Buf>* self,int depth,const char* expected){
            auto check=[&](auto... args){return op_expect(self,expected,args...);};
            switch (depth){
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
                return check("*","/","%");
            case 1:
                return check("+","-");
            case 0:
                return check("++","--","+","-","!");
            default:
                return false;
            }
        }

        Tree* primary_tree(Reader<Buf>* self){
            Tree* ret=nullptr;
            if(self->expect("(")){
                ret=expression<Tree>(self,*this);
                if(!ret)return nullptr;
                if(!self->expect(")")){
                    delete ret;
                    return nullptr;
                }
            }
            else if(self->ahead("\"")||self->ahead("'")){
                Buf str;
                if(!reader.string(str))return nullptr;
                ret=make_tree(str,nullptr,nullptr,Kind::string);
                if(!ret)return nullptr;
            }
            else if(is_digit(self->achar())){
                Buf num;
                NumberContext<char> ctx;
                self->readwhile(num,number,&ctx);
                if(!ctx.succeed)return nullptr;
                if(ctx.floatf){
                    ret=make_tree(num,nullptr,nullptr,Kind::floats);
                }
                else{
                    ret=make_tree(num,nullptr,nullptr,Kind::integer);
                }
                if(!ret)return nullptr;
            }
            else if(reader.expect("new",is_c_id_usable)){
                auto tmp=expression(self,*this);
                if(!tmp)return nullptr;
                ret=make<Tree>("new",EvalType::create,nullptr,tmp);
                if(!ret){
                    delete tmp;
                    return nullptr;
                }
            }  
            else if(is_c_id_top_usable(reader.achar())){
                Buf id;
                reader.readwhile(id,untilincondition,is_c_id_usable<char>);
                if(id.size()==0)return nullptr;
                if(id=="true"||id=="false"){
                    ret=make_tree(id,Kind::boolean);
                }
                else{
                    ret=make_tree(id);
                }
                if(!ret)return nullptr;
            }
            return ret;
        }
    };*/
}
