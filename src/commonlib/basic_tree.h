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

    template<class Tree,class Str,class Buf,class Ctx>
    Tree* unary(Reader<Buf>* self,Ctx& ctx){
        //using Char=typename Reader<Buf>::char_type;
        //const Char* expected=nullptr;
        Str expected=Str();
        auto flag=ctx.expect(self,0,expected);
        Tree* tmp=nullptr;
        if(flag>0){
            tmp=primary<Tree>(self,ctx);
        }
        else if(ctx.expect(self,-1,expected)){
            tmp=unary<Tree,Str>(self,ctx);
        }
        else{
            return primary<Tree>(self,ctx);
        }
         if(!tmp)return nullptr;
        auto ret=ctx.make_tree(expected,nullptr,tmp);
        if(!ret){
            ctx.delete_tree(tmp);
            return nullptr;
        }
        return ret;
    }

    template<class Tree,class Str,class Buf,class Ctx>
    Tree* bin(Reader<Buf>* self,Ctx& ctx,int depth){
        //using Char=typename Reader<Buf>::char_type;
        if(depth<=0)return unary<Tree,Str>(self,ctx);
        Tree* ret=bin<Tree,Str>(self,ctx,depth-1);
        if(!ret)return nullptr;
        //const Char* expected=nullptr;
        Str expected=Str();
        while(true){
            auto flag=ctx.expect(self,depth,expected);
            if(flag==0)break;
            auto tmp=bin<Tree,Str>(self,ctx,flag>0?depth-1:depth);
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

    template<class Tree,class Str,class Buf,class Ctx>
    Tree* expression(Reader<Buf>* self,Ctx& ctx){
        return bin<Tree,Str>(self,ctx,ctx.depthmax);
    }

    template<class Buf,class Ctx,class Str=Buf>
    bool read_expr(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Tree=std::remove_pointer_t<remove_cv_ref<decltype(ctx.result)>>;
        if(!begin)return true;
        if(!self)return true;
        ctx.result=expression<Tree,Str>(self,ctx);
        return false;
    }


    template<class Buf,class Str,class... OP>
    bool op_expect2(Reader<Buf>* self,Str& expected,OP... op){
        using swallow = std::initializer_list<int>;
        bool res=false;
        (void)swallow{ (void( res=res||self->expectp(op,expected) ), 0)... };
        return res;
    }


    template<class Buf,class Str>
    bool op_expect(Reader<Buf>*,Str&){return false;}

    template<class Buf,class Str,class First,class... Other>
    bool op_expect(Reader<Buf>* self,Str& expected,First first,Other... other){
        return self->expectp(first,expected)||op_expect(self,expected,other...);
        //return op_expect2(self,expected,first,other...);
    }

    

}
