/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"reader.h"
#include"reader_helper.h"
#include"basic_tree.h"

namespace PROJECT_NAME{
    template<class Tree,class Str,class Kind,class Buf,class Checker,class Flags=bool>
    struct ExampleTree{
        Tree* result=nullptr;
        Checker& check;
        Flags flags;
        int depthmax=0;

        ExampleTree(Checker& func,int depth):check(func),depthmax(depth){}


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
            ret->kind=kind;
            return ret;
        }

        void delete_tree(Tree* tree){
            delete tree;
        }




        int expect(Reader<Buf>* self,int depth,Str& expected){
            return check(self,expected,depth,flags);
        }

        Tree* primary_tree(Reader<Buf>* self){
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
                Buf str;
                if(!self->string(str))return nullptr;
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
            else if(self->expect("new",is_c_id_usable)){
                if(!is_c_id_top_usable(self->achar()))
                    return nullptr;
                Buf id;
                self->readwhile(id,untilincondition,is_c_id_usable<char>);
                if(id.size()==0)return nullptr;
                auto tmp=make_tree(id);
                if(!tmp)return nullptr;
                if(!self->expect("(")){
                    delete tmp;
                    return nullptr;
                }
                auto obj=parse_arg(tmp,self,"()",")");
                if(!obj)return nullptr;
                ret=make_tree("new",nullptr,obj,Kind::create);
                if(!ret){
                    delete obj;
                    return nullptr;
                }
            }  
            else if(is_c_id_top_usable(self->achar())||self->achar()=='$'){
                Buf id;
                if(self->expect("$$")){
                    id="$$";
                }
                else if(self->expect("$@")){
                    id="$@";
                }
                else{
                    self->readwhile(id,untilincondition,is_c_id_usable<char>);
                }
                if(id.size()==0)return nullptr;
                if(id=="true"||id=="false"){
                    ret=make_tree(id,nullptr,nullptr,Kind::boolean);
                }
                else{
                    ret=make_tree(id);
                }
                if(!ret)return nullptr;
            }
            return ret;
        }

        Tree* after_tree(Tree* ret,Reader<Buf>* self){
            const char* expected=nullptr;
            while(true){
                if(self->expect("(")){
                   if(!(ret=parse_arg(ret,self,"()",")")))return nullptr;
                    continue;
                }
                else if(self->expect("[")){
                    auto hold=make_tree("[]",nullptr,ret);
                    if(!hold){
                        delete ret;
                        return nullptr;
                    }
                    ret=hold;
                    auto tmp=expression<Tree,Str>(self,*this);
                    if(!tmp||!self->expect("]")){
                        delete tmp;
                        delete ret;
                        return nullptr;
                    }
                    ret->left=tmp;
                    //if(!(ret=parse_arg(ret,self,"[]","]")))return nullptr;
                    continue;
                }
                else if(self->expect(".")){
                    if(!is_c_id_top_usable(self->achar())){
                        delete ret;
                        return nullptr;
                    }
                    Buf id;
                    self->readwhile(id,untilincondition,is_c_id_usable<char>);
                    if(id.size()==0){
                        delete ret;
                        return nullptr;
                    }
                    auto tmp1=make_tree(id,nullptr,nullptr);
                    if(!tmp1){
                        delete ret;
                        return nullptr;
                    }
                    auto tmp2=make_tree(".",ret,tmp1);
                    if(!tmp2){
                        delete ret;
                        delete tmp1;
                        return nullptr;
                    }
                    ret=tmp2;
                    continue;
                }
                else if(self->expectp("++",expected)||self->expectp("--",expected)){
                    auto tmp=make_tree(expected,nullptr,ret);
                    if(!tmp){
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

        Tree* parse_arg(Tree* ret,Reader<Buf>* self,const char* symbol,const char* end){
            auto hold=make_tree(symbol,nullptr,ret);
            if(!hold){
                delete ret;
                return nullptr;
            }
            ret=hold;
            while(true){
                if(self->expect(end))break;
                auto tmp=expression<Tree,Str>(self,*this);
                if(!tmp||(!self->expect(",")&&!self->ahead(end))){
                    delete tmp;
                    delete ret;
                    return nullptr;
                }
                ret->arg.push_back(tmp);
            }
            return ret;
        }

        Tree* parse_closure(Reader<Buf>* self){
            BasicBlock<Buf> block(false,false,true);
            bool needless=false;
            Buf closure="$";
            if(self->expect("[")){
                closure+="[";
                self->readwhile(closure,until,']');
                if(!self->expect("]"))return nullptr;
                closure+="]";
                needless=true;
            }
            if(self->ahead("(")){
                closure+="(";
                if(!self->readwhile(closure,depthblock_withbuf,block))
                    return nullptr;
                closure+=")";
            }
            if(self->expect("->")){
                closure+="->";
                self->readwhile(closure,until,'{');
            }
            block.bracket=false;
            block.c_block=true;
            closure+="{";
            if(!self->readwhile(closure,depthblock_withbuf,block))return nullptr;
            closure+="}";
            return make_tree(closure,nullptr,nullptr,Kind::closure);
        }
    };
}