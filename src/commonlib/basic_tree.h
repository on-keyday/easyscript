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
    bool read_expr(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Tree=std::remove_pointer_t<remove_cv_ref<decltype(ctx.result)>>;
        if(!begin)return true;
        if(!self)return true;
        ctx.result=expression<Tree>(self,ctx);
        return false;
    }

    template<class Buf>
    bool op_expect(Reader<Buf>*,const char*&){return false;}

    template<class Buf,class First,class... Other>
    bool op_expect(Reader<Buf>* self,const char*& expected,First first,Other... other){
        return self->expectp(first,expected)||op_expect(self,expected,other...);
    }

}
