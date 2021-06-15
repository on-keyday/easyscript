/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"reader.h"
#include<cstring>
#include<algorithm>
#include<string>
namespace PROJECT_NAME{

    template<class Buf>
    struct Refer{
    private:
        Buf& buf;
    public:
        Buf* operator->(){
            return &buf;
        }

        decltype(buf[0]) operator[](size_t s)const{
            return buf[s];
        }

        decltype(buf[0]) operator[](size_t s){
            return buf[s];
        }

        size_t size()const{
            return buf.size();
        }
        Refer(Refer&& in):buf(in.buf){}
        Refer(Buf& in):buf(in){}
    };

    template<class Char>
    struct NumberContext{
        int radix=0;
        bool floatf=false;
        bool expf=false;
        bool failed=false;
        bool (*judgenum)(Char)=nullptr;
    };

    struct LinePosContext{
        size_t line=0;
        size_t pos=0;
        size_t nowpos=0;
        bool crlf=false;
        bool cr=false;
    };
    

    template<class Char>
    bool is_digit(Char c){
        return c>=(Char)'0'&&c<=(Char)'9';
    }

    template<class Char>
    bool is_oct(Char c){
        return c>=(Char)'0'&&c<=(Char)'7';
    }

    template<class Char>
    bool is_hex(Char c){
        return c>=(Char)'a'&&c<=(Char)'f'||
               c>=(Char)'A'&&c<=(Char)'F'||
               is_digit(c);
    }

    template<class Char>
    bool is_bin(Char c){
        return c==(Char)'0'||c==(Char)'1';
    }

    template<class Char>
    bool is_alphabet(Char c){
        return c>=(Char)'a'&&c<=(Char)'z'||
               c>=(Char)'A'&&c<=(Char)'Z';
    }

    template<class Char>
    bool is_alpha_or_num(Char c){
        return is_alphabet(c)||is_digit(c);
    }

    template<class Char>
    bool is_c_id_usable(Char c){
        return is_alpha_or_num(c)||c==(Char)'_';    
    }

    template<class Char>
    bool is_c_id_top_usable(Char c){
        return is_alphabet(c)||c==(Char)'_';    
    }

    template<class Char>
    bool is_url_host_usable(Char c){
        return is_alpha_or_num(c)||c==(Char)'-'||c==(Char)'.';
    }

    template<class Char>
    bool is_url_scheme_usable(Char c){
        return is_alpha_or_num(c)||c==(Char)'+';
    }

    template<class Buf,class Ret,class Char>
    bool number(Reader<Buf>* self,Ret& ret,NumberContext<Char>*& ctx,bool begin){
        if(!ctx)return !begin;
        if(!self){
            return true;
        }
        auto n=self->achar();
        auto increment=[&]{
            ret.push_back(n);
            self->increment();
            n=self->achar();
        };
        if(begin){
            ctx->expf=false;
            ctx->failed=false;
            ctx->floatf=false;
            ctx->judgenum=nullptr;
            ctx->radix=0;
            if(is_digit(n)){
                if(n!=(Char)'0'){
                    ctx->radix=10;
                    ctx->judgenum=is_digit;
                }
                increment();
                return true;
            }
            else{
                ctx->failed=true;
                return false;
            }
        }
        bool proc=true,must=false;
        if(ctx->radix==0){
            if(n==(Char)'x'||n==(Char)'X'){
                ctx->radix=16;
                ctx->judgenum=is_hex;
            }
            else if(n==(Char)'b'||n==(Char)'B'){
                ctx->radix=2;
                ctx->judgenum=is_bin;
            }
            else if(is_oct(n)){
                ctx->radix=8;
                ctx->judgenum=is_oct;
                proc=false;
            }
            else if(n==(Char)'.'){
                ctx->floatf=true;
                ctx->judgenum=is_digit;
                ctx->radix=10;
            }
            else{
                return true;
            }
            if(proc){
                increment();
                must=true;
            }
        }
        else if(n==(Char)'.'){
            if(ctx->floatf){
                ctx->failed=true;
                return true;
            }
            ctx->floatf=true;
            increment();
            must=true;
        }
        else if((ctx->radix==10&&(n==(Char)'e'||n==(Char)'E'))||
                (ctx->radix==16&&(n==(Char)'p'||n==(Char)'P'))){
            if(ctx->expf){
                ctx->failed=true;
                return true;
            }
            ctx->expf=true;
            ctx->floatf=true;
            increment();
            if(n!=(Char)'+'&&n!=(Char)'-'){
                ctx->failed=true;
                return true;
            }
            ctx->radix=10;
            increment();
            must=true;
        }
        if(proc){
            if(!ctx->judgenum)return true;
            if(!ctx->judgenum(n)){
                if(must)ctx->failed=true;
                return true;
            }
        }
        ret.push_back(n);
        return false;
    }

    template<class Buf,class Ctx>
    bool depthblock(Reader<Buf>* self,Ctx& check,bool begin){
        if(!self||!begin)return true;
        if(!check(self,true))return false;
        size_t dp=0;
        bool ok=false;
        while (!self->eof()) {  
            if (check(self,true)) {
                dp++;
            }                
            if (check(self,false)) {
                if (dp == 0){
                    ok=true;
                    break;
                }
                dp--;
            }
        }
        return ok;
    }

    template<class Buf,class Ret,class Ctx>
    bool depthblock_withbuf(Reader<Buf>* self,Ret& ret,Ctx& check,bool begin){
        if(!self||!begin)return true;
        if(!check(self,true))return false;
        size_t beginpos=self->readpos(),endpos=beginpos;
        size_t dp=0;
        bool ok=false;
        while (!self->eof()) {
            endpos=self->readpos();
            if (check(self,true)) {
                dp++;
            }                
            if (check(self,false)) {
                if (dp == 0){
                    ok=true;
                    break;
                }
                dp--;
            }
            self->increment();
        }
        if(ok){
            auto finalpos=self->readpos();
            self->seek(beginpos);
            while(self->readpos()!=endpos){
                ret.push_back(self->achar());
                self->increment();
            }
            self->seek(finalpos);
        }
        return ok;
    }

    template<class Buf>
    struct BasicBlock{
        bool c_block=true;
        bool arr_block=true;
        bool bracket=true;
        bool operator()(Reader<Buf>* self,bool begin){
            bool ret=false;
            if(c_block){
                if(begin){
                    ret=self->expect("{");
                }
                else{
                    ret=self->expect("}");
                }
            }
            if(!ret&&arr_block){
                if(begin){
                    ret=self->expect("[");
                }
                else{
                    ret=self->expect("]");
                }
            }
            if(!ret&&bracket){
                if(begin){
                    ret=self->expect("(");
                }
                else{
                    ret=self->expect(")");
                }
            }
            return ret;
        }
        BasicBlock(bool block=true,bool array=true,bool bracket=true):
        c_block(block),arr_block(array),bracket(bracket){}
    };

    template<class Buf,class Ret,class Char> 
    bool until(Reader<Buf>* self,Ret& ret,Char& ctx,bool begin){
        if(begin)return true;
        if(!self)return true;
        if(self->achar()==ctx)return true;
        ret.push_back(self->achar());
        return false;
    }

    template<class Buf,class Char>
    bool untilnobuf(Reader<Buf>* self,Char ctx,bool begin){
        if(begin)return true;
        if(!self)return true;
        if(self->achar()==ctx)return true;
        return false;
    }

    template<class Buf,class Ret,class Ctx>
    bool untilincondition(Reader<Buf>* self,Ret& ret,Ctx& judge,bool begin){
        if(begin)return true;
        if(!self)return true;
        if(!judge(self->achar()))return true;
        ret.push_back(self->achar());
        return false;
    }

    template<class Buf,class Ctx>
    bool untilincondition_nobuf(Reader<Buf>* self,Ctx& judge,bool begin){
        if(begin)return true;
        if(!self)return true;
        if(!judge(self->achar()))return true;
        return false;
    }

    template<class Buf,class Ret,class Ctx>
    bool addifmatch(Reader<Buf>* self,Ret& ret,Ctx&judge,bool begin){
        if(begin)return true;
        if(!self)return true;
        if(judge(self->achar())){
            ret.push_back(self->achar());
        }
        return false;
    }

    template<class Buf,class Ctx>
    bool linepos(Reader<Buf>* self,Ctx& ctx,bool begin){
        if(!self)return true;
        if(begin){
            ctx.nowpos=self->readpos();
            ctx.line=0;
            ctx.pos=0;
            self->seek(0);
            return true;
        }
        if(self->readpos()>=ctx.nowpos){
            self->seek(ctx.nowpos);
            return true;
        }
        ctx.pos++;
        if(ctx.crlf){
            while(self->expect("\r\n")){
                ctx.line++;
                ctx.pos=0;
            }
        }
        else if(ctx.cr){
            while(self->expect("\r")){
                ctx.line++;
                ctx.pos=0;
            }
        }
        else{
            while(self->expect("\n")){
                ctx.line++;
                ctx.pos=0;
            }
        }
        return false;
    }

    template<class Buf,class Ret>
    bool cmdline_read(Reader<Buf>* self,Ret& ret,int*& res,bool begin){
        if(begin)return true;
        if(!self)return true;
        auto c=self->achar();
        if(c=='\"'||c=='\''||c=='`'){
            if(!self->string(ret)){
                *res=0;
            }
            else{
                *res=-1;
            }
        }
        else {
            auto prev=ret.size();
            self->readwhile(ret,untilincondition,[](char c){return c!=' '&&c!='\t';});
            if(ret.size()==prev){
                *res=0;
            }
            else{
                *res=1;
            }
        }
        return true;
    }
}