/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once 
#include"reader.h"
namespace PROJECT_NAME{
    

    inline unsigned char utf8bits(int i){
        const unsigned char maskbits[]={
            //first byte mask
            0b10000000,
            0b11000000,
            0b11100000,
            0b11110000,
            0b11111000,
            
            //i>=5,second byte later mask
            0b00011100, //two byte must
            0b00001111,0b00100000, //three byte must
            0b00000111,0b00110000, //four byte must
        };
        return i>=0&&i<sizeof(maskbits)?maskbits[i]:0;
    }

    inline unsigned char utf8mask(unsigned char c,int i){
        if(i<1||i>4)return 0;
        return (utf8bits(i)&c)==utf8bits(i-1)?utf8bits(i-1):0;
    }

    template<class Buf,class Ret>
    bool utf8_read(Reader<Buf>* self,Ret& ret,int& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        if(begin)return true;
        if(!self)return true;
        auto r=[&](int ofs=0){
            return (unsigned char)self->offset(ofs);
        };
        auto mask=[&r](auto i,int ofs=0){
            return utf8mask(r(ofs),i);
        };
        auto needbits=[&r](int ofs,int masknum){
            return r(ofs)&utf8bits(masknum);
        };
        int range=0;
        if(r()<0x80){
            range=1;
        }
        else if(mask(2)){
            range=2;
            if(!needbits(0,5)){
                ctx=1;
                return true;
            }
        }
        else if(mask(3)){
            range=3;
            if(!needbits(0,6)&&!needbits(1,7)){
                ctx=12;
                return true;
            }
        }
        else if(mask(4)){
            range=4;
            if(!needbits(0,8)&&!needbits(1,9)){
                ctx=12;
            }
        }
        else{
            ctx=1;
            return true;
        }
        for(auto i=0;i<range;i++){
            if(i){
                self->increment();
                if(!mask(1)){
                    ctx=i+1;
                    return true;
                }
            }
            ret.push_back(r());
        }
        return false;
    }

    template<class Buf,class Ret,class Str=Buf>
    bool utf8toutf32(Reader<Buf>* self,Ret& ret,int& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        static_assert(sizeof(ret[0])==4);
        if(begin)return true;
        if(!self)return true;
        int errpos=0;
        Str buf;
        utf8_read(self,buf,errpos,false);
        if(errpos){
            ctx=errpos;
            return true;
        }
        using Char32=remove_cv_ref<decltype(ret[0])>;
        auto len=(int)buf.size();
        Char32 c=0;
        auto maskbit=[](int i){
            return ~utf8bits(i);
        };
        auto make=[&maskbit,&buf,&len](){
            Char32 ret=0;
            for(int i=0;i<len;i++){
                auto mul=(len-i-1);
                auto shift=6*mul;
                unsigned char masking=0;
                if(i==0){
                    masking=buf[i]&maskbit(len);
                }
                else {
                    masking=buf[i]&maskbit(1);
                }
                ret|=masking<<shift;
            }
            return ret;
        };
        if(len==1){
            ret.push_back((Char32)buf[0]);
            return false;
        }
        else {
            ret.push_back(make());
        }
        return false;
    }
}