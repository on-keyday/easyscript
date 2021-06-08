/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include<reader.h>
#include<basic_helper.h>
#include<basic_tree.h>

namespace hard_expr{
    template<class Tree,class Str,class Kind,class Buf,class Customize>
    struct HardExprTree{
        Tree* result=nullptr;
        Customize cus;
        int depthmax=0;

        HardExprTree(Customize&& c,int depth):cus(std::move(c)),depthmax(depth){}


        Tree* make_tree(Str symbol,Tree*left=nullptr,Tree* right=nullptr,Kind kind=Kind::unknown){
            Tree* ret=nullptr;
            try{
                ret=new Tree();
            }
            catch(...){
                return nullptr;
            }
            ret->symbol=symbol;
            ret->left=left;
            ret->right=right;
            if(kind!=Kind::unknown){
                ret->kind=kind;
            }
            else{
                ret->kind=decl_kind(symbol,left,right);
            }
            return ret;
        }

        void delete_tree(Tree* tree){
            delete tree;
        }

        Kind decl_kind(const Str& expected,Tree* left,Tree* right){
            return cus.decl_kind(expected,left,right);
        }

        int expect(PROJECT_NAME::Reader<Buf>* self,int depth,Str& expected){
            return cus.expect(self,expected,depth);
        }

        Tree* primary_tree(PROJECT_NAME::Reader<Buf>* self){
            Tree* ret=nullptr;
            if(self->expect("(")){
                ret=parse_arg(nullptr,self,"()",")");
                if(!ret)return nullptr;
            }
            else if(self->expect("[")){
                ret=parse_arg(nullptr,self,"[]","]");
                if(!ret)return nullptr;
            }
            else if(self->expect("{")){
                ret=parse_arg(nullptr,self,"{}","}");
                if(!ret)return nullptr;
            }
            else if(self->expect("$",[](char c){return c!='['&&c!='('&&c!='{';})){
                ret=parse_closure(self);
                if(!ret)return nullptr;
            }
            else if(self->ahead("\"")||self->ahead("'")){
                Str str;
                if(!self->string(str))return nullptr;
                ret=make_tree(str,nullptr,nullptr,decl_kind("string",nullptr,nullptr));
                if(!ret){
                    cus.error("string:memory is full");
                    return nullptr;
                }
            }
            else if(PROJECT_NAME::is_digit(self->achar())){
                Str num;
                PROJECT_NAME::NumberContext<char> ctx;
                self->readwhile(num,PROJECT_NAME::number,&ctx);
                if(!ctx.succeed)return nullptr;
                if(ctx.floatf){
                    ret=make_tree(num,nullptr,nullptr,decl_kind("float",nullptr,nullptr));
                }
                else{
                    ret=make_tree(num,nullptr,nullptr,decl_kind("int",nullptr,nullptr));
                }
                if(!ret){
                    cus.error("digit:memory is full");
                    return nullptr;
                }
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
                ret=make_tree("new",nullptr,obj,decl_kind("new",nullptr,nullptr));
                if(!ret){
                    cus.error("new:memory is full");
                    delete obj;
                    return nullptr;
                }
            }  
            else if(PROJECT_NAME::is_c_id_top_usable(self->achar())||self->achar()=='$'||self->achar()==':'){
                Str id;
                if(self->expect("$$")){
                    id="$$";
                }
                else if(self->expect("$@")){
                    id="$@";
                }
                else{
                    if(!id_read(self,id))return nullptr;
                }
                if(id=="true"||id=="false"){
                    ret=make_tree(id,nullptr,nullptr,decl_kind("bool",nullptr,nullptr));
                }
                else{
                    ret=make_tree(id,nullptr,nullptr,decl_kind("var",nullptr,nullptr));
                }
                if(!ret)return nullptr;
            }
            return ret;
        }

        bool id_read(PROJECT_NAME::Reader<Buf>* self,Str& id){
            if(self->expect("::")){
                id+="::";
            }
            size_t prev=0;
            while(true){
                self->readwhile(id,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
                if(id.size()==prev){
                    cus.error("id:expected identifier but not");
                    return false;
                }
                if(self->expect("::")){
                    id+="::";
                    prev=id.size();
                    continue;
                }
                break;
            }
            return true;
        }

        Tree* after_tree(Tree* ret,PROJECT_NAME::Reader<Buf>* self){
            const char* expected=nullptr;
            while(true){
                if(self->expect("(")){
                   if(!(ret=parse_arg(ret,self,"()",")")))return nullptr;
                    continue;
                }
                else if(self->expect("[")){
                    auto hold=make_tree("[]",nullptr,ret);
                    if(!hold){
                        cus.error("[]:memory is full");
                        delete ret;
                        return nullptr;
                    }
                    ret=hold;
                    auto tmp=cus.recall(self,*this);
                    if(!tmp){
                        cus.error(":[]");
                        delete tmp;
                        delete ret;
                        return nullptr;
                    }
                    if(!self->expect("]")){
                        cus.error("[]:expected ] but not");
                        delete tmp;
                        delete ret;
                        return nullptr;
                    }
                    ret->left=tmp;
                    //if(!(ret=parse_arg(ret,self,"[]","]")))return nullptr;
                    continue;
                }
                else if(self->expect(".")){
                    if(!PROJECT_NAME::is_c_id_top_usable(self->achar())){
                        cus.error(".:expected identifier but not");
                        delete ret;
                        return nullptr;
                    }
                    Str id;
                    self->readwhile(id,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
                    if(id.size()==0){
                        cus.error(".:expected identifier but not");
                        delete ret;
                        return nullptr;
                    }
                    auto tmp1=make_tree(id,nullptr,nullptr);
                    if(!tmp1){
                        cus.error(".:memory is full");
                        delete ret;
                        return nullptr;
                    }
                    auto tmp2=make_tree(".",ret,tmp1);
                    if(!tmp2){
                        cus.error(".:memory is full");
                        delete ret;
                        delete tmp1;
                        return nullptr;
                    }
                    ret=tmp2;
                    continue;
                }
                else if(self->expectp("++",expected)||self->expectp("--",expected)){
                    auto tmp=make_tree(expected,nullptr,ret,decl_kind(expected,nullptr,ret));
                    if(!tmp){
                        cus.error("++:memory is full");
                        delete ret;
                        return nullptr;
                    }
                    ret=tmp;
                    continue;
                }
                break;
            }
            return ret;
        }

        Tree* parse_arg(Tree* ret,PROJECT_NAME::Reader<Buf>* self,const char* symbol,const char* end){
            auto hold=make_tree(symbol,nullptr,ret);
            if(!hold){
                cus.error("arg:memory is full");
                delete ret;
                return nullptr;
            }
            ret=hold;
            while(true){
                if(self->expect(end))break;
                auto tmp=cus.recall(self,*this);
                if(!tmp){
                    cus.error(":arg");
                    delete tmp;
                    delete ret;
                    return nullptr;
                }
                if(!self->expect(",")&&!self->ahead(end)){
                    cus.error("arg:expected , but not");
                    delete tmp;
                    delete ret;
                    return nullptr;
                }
                ret->arg.push_back(tmp);
            }
            return ret;
        }

        Tree* parse_closure(PROJECT_NAME::Reader<Buf>* self){
            PROJECT_NAME::BasicBlock<Buf> block(false,false,true);
            bool needless=false;
            Str closure="$";
            if(self->expect("[")){
                closure+="[";
                self->readwhile(closure,PROJECT_NAME::until,']');
                if(!self->expect("]")){
                    cus.error("closure:expected ] but not");
                    return nullptr;
                }
                closure+="]";
                needless=true;
            }
            if(self->ahead("(")){
                closure+="(";
                if(!self->readwhile(closure,PROJECT_NAME::depthblock_withbuf,block)){
                    cus.error("closure:expected ) but not");
                    return nullptr;
                }
                closure+=")";
            }
            if(self->expect("->")){
                closure+="->";
                self->readwhile(closure,PROJECT_NAME::until,'{');
            }
            block.bracket=false;
            block.c_block=true;
            closure+="{";
            if(!self->readwhile(closure,PROJECT_NAME::depthblock_withbuf,block)){
                cus.error("closure:expected } but not");
                return nullptr;
            }
            closure+="}";
            auto r=make_tree(closure,nullptr,nullptr,decl_kind("closure",nullptr,nullptr));
            if(!r){
                cus.error("closure:memory is full");
            }
            return r;
        }
    };
}