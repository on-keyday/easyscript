/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"project_name.h"
#include<cstddef>
#include<utility>
//#include<bit>
namespace PROJECT_NAME{

    template<class T>
    T translate_byte_as_is(const char* s) {
        T res = 0;
        char* res_p = (char*)&res;
        for (auto i = 0u; i < sizeof(T); i++) {
            res_p[i] = s[i];
        }
        return res;
    }

    template<class T>
    T translate_byte_reverse(const char* s) {
        T res = 0;
        char* res_p = (char*)&res;
        auto k = 0ull;
        for (auto i = sizeof(T)-1;;i--) {
            res_p[i] = s[k];
            if (i == 0)break;
            k++;
        }
        return res;
    }

    template<class T>
    T translate_byte_net_and_host(const char* s){
        int i=1;
        char* t=(char*)&i;
        if(t[0]){
            return translate_byte_reverse<T>(s);
        }
        else{
            return translate_byte_as_is<T>(s);
        }
    }

    
    template<class T>
    using remove_cv_ref=std::remove_cv_t<std::remove_reference_t<T>>;

    template<class Buf>
    struct Reader{
    private:
        Buf buf;
        using Char=remove_cv_ref<decltype(buf[0])>;
    public:
        using IgnoreHandler=bool(*)(Buf&,size_t&);
        using not_expect_default=bool(*)(Char);
        using cmp_default=bool(*)(Char,Char);
    private:
        size_t pos=0;
        IgnoreHandler ignore_cb=nullptr;
        bool stop=false;
        bool refed=false;

        static bool default_cmp(Char c1,Char c2){
            return c1==c2;
        }

        template<class Str>
        inline size_t strlen(Str str){
            size_t i=0;
            while(str[i]!=0){
                if(str[i]==0)return i;
                if(str[i+1]==0)return i+1;
                if(str[i+2]==0)return i+2;
                if(str[i+3]==0)return i+3;
                if(str[i+4]==0)return i+4;
                i+=5;
            }
            return i;
        }

        size_t buf_size(Char* str)const{
            if(!str)return 0;
            return strlen(str);
        }

        size_t buf_size(const Char* str)const{
            if(!str)return 0;
            return strlen(str);
        }

        template<class Str>
        size_t buf_size(Str& str)const{
            return str.size();
        }

        bool endbuf(int ofs=0) const{
            return (buf_size(buf)<=(size_t)(pos+ofs));
        }

        bool refcheck(){
            if(refed){
                release();
                refed=false;
                return true;
            }
            return false;
        }

        bool ignore(){
            if(endbuf())return false;
            refcheck();
            if(stop)return false;
            if(ignore_cb){
                stop=!ignore_cb(buf,pos);
                return stop==false;
            }
            return true;
        }

        bool on_end(const Char* s,size_t pos){
            return s[pos]== 0;
        }

        bool on_end(Char* s,size_t pos){
            return s[pos]==0;
        }

        template<class Str>
        bool on_end(Str& s,size_t pos){
            return s.size()<=pos;
        }

        template<class T>
        bool invoke_not_expect(bool(*not_expect)(T t),Char c){
            if(!not_expect)return false;
            return not_expect(c);
        } 

        template<class Func>
        bool invoke_not_expect(Func& not_expect,Char c){
            return not_expect(c);
        }

        bool invoke_not_expect(std::nullptr_t,Char){
            return false;
        }

        template<class Str,class NotExpect=not_expect_default,class Cmp=cmp_default>
        size_t ahead_detail(Str& str,NotExpect not_expect=NotExpect(),Cmp cmp=default_cmp){
            //if(!cmp)return 0;
            if(!ignore())return 0;
            size_t i=0;
            for(;!on_end(str,i);i++){
                if(endbuf(i))return 0;
                if(!cmp(buf[pos+i],str[i]))return 0;
            }
            if(i==0)return 0;
            if(invoke_not_expect(not_expect,buf[pos+i]))return 0;
            return i;
        }

    public:
        using char_type=Char;
        using buf_type=Buf;
        Reader(Reader&)=delete;
        Reader(Reader&&)=delete;

        Reader(IgnoreHandler cb=nullptr){
            ignore_cb=cb;
        }

        Reader(Buf& in,IgnoreHandler cb=nullptr){
            buf=in;
            ignore_cb=cb;
        }

        Reader(Buf&& in,IgnoreHandler cb=nullptr){
            buf=std::move(in);
            ignore_cb=cb;
        }

        template<class Str,class NotExpect=not_expect_default,class Cmp=cmp_default>
        size_t ahead(Str& str,NotExpect not_expect=NotExpect(),Cmp cmp=default_cmp){
            return ahead_detail(str,not_expect,cmp);
        }

        template<class NotExpect=not_expect_default,class Cmp=cmp_default>
        size_t ahead(const Char* str,NotExpect not_expect=NotExpect(),Cmp cmp=default_cmp){
            if(!str)return false;
            return ahead_detail(str,not_expect,cmp);
        }

        template<class NotExpect=not_expect_default,class Cmp=cmp_default>
        size_t ahead(Char* str,NotExpect not_expect=NotExpect(),Cmp cmp=default_cmp){
            if(!str)return false;
            return ahead_detail(str,not_expect,cmp);
        }
        
        template<class Str,class NotExpect=not_expect_default,class Cmp=cmp_default>
        bool expect(Str& str,NotExpect not_expect=NotExpect(),Cmp cmp=default_cmp){
            auto size=ahead(str,not_expect,cmp);
            if(size==0)return false;
            pos+=size;
            ignore();
            return true;
        }

        template<class Str,class Ctx,class NotExpect=not_expect_default,class Cmp=cmp_default>
        bool expectp(Str& str,Ctx& ctx,NotExpect not_expect=NotExpect(),Cmp cmp=default_cmp){
            if(expect<Str,NotExpect,Cmp>(str,not_expect,cmp)){
                ctx=str;
                return true;
            }
            return false;
        }

        Char achar() const{
            return offset(0);
        }

        bool seek(size_t pos,bool strict=false){
            if(buf.size()<=pos){
                if(strict)return false;
                pos=buf.size();
            }
            this->pos=pos;
            if(!endbuf())stop=false;
            return true;
        }

        bool increment(){
            return seek(pos+1);
        }

        Char offset(int ofs) const{
            if(endbuf(ofs))return Char();
            return buf[pos+ofs];
        }

        bool ceof(int ofs=0) const {
            return stop||endbuf(ofs);
        }

        bool eof() {
            ignore();
            return ceof();
        }

        bool release(){
            if(endbuf())return false;
            stop=false;
            return true;
        }

        size_t readpos() const {
            return pos;
        }

        size_t readable() {
            if(eof())return 0;
            return buf.size()-pos;
        }

        IgnoreHandler set_ignore(IgnoreHandler hander){
            auto ret=this->ignore_cb;
            this->ignore_cb=hander;
            return ret;
        }
        /*
        template<class T>
        using AddHandler=bool(*)(Buf&,T);
        
        template<class T>
        bool add(T toadd,AddHandler<T> add_handler){
            if(!add_handler)return false;
            if(!add_handler(buf,toadd))return false;
            return release();
        }*/
        

        template<class T>
        size_t read_byte(T* res=nullptr,size_t size=sizeof(T),
        T (*endian_handler)(const char*)=translate_byte_as_is,bool strict=false){
            static_assert(sizeof(decltype(buf[0]))==1);
            if(strict&&readable()<size)return 0;
            size_t beginpos=pos;
            if(res){
                if (!endian_handler)return 0;
                size_t willread=size/sizeof(T)+(size%sizeof(T)?1:0);
                size_t ofs=0;
                while (!eof()&&ofs<willread) {
                    char pass[sizeof(T)]={0};
                    size_t remain=size-sizeof(T)*ofs;
                    size_t max=remain<sizeof(T)?remain:sizeof(T);
                    for(auto i=0;i<max;i++){
                        pass[i]=buf[pos];
                        pos++;
                        if(endbuf()){
                            break;
                        }
                    }
                    res[ofs] = endian_handler(pass);
                    ofs++;
                }
            }
            else{
                pos+=size;
            }
            if(endbuf()){
                pos=buf.size();
            }
            return pos-beginpos;
        }

        Buf& ref(){refed=true;return buf;}

        /*
        bool depthblock(bool(*check)(Reader*,bool)=
            [](auto self,auto isbegin)->bool{
                if(isbegin){
                    return self->expect("{");
                }
                else{
                    return self->expect("}");
                }
            }
            ){
            if(!check)return false;
            if(!check(this,true))return false;
            size_t dp=0;
            bool ok=false;
            while (!eof()) {  
                if (check(this,true)) {
                    dp++;
                }                
                if (check(this,false)) {
                    if (dp == 0){
                        ok=true;
                        break;
                    }
                    dp--;
                }
                
                pos++;
            }
            return ok;
        }*/

        template<class Ret>
        static bool read_string(Reader* self,Ret& buf,bool& noline,bool begin){
            if(!self)return false;
            if(begin){
                if(self->achar()!=(Char)'\''&&self->achar()!=(Char)'"')return false;
                buf.push_back(self->achar());
                self->increment();
                //ctx=self->set_ignore(nullptr);
                return true;
            }
            else{
                if(self->achar()==*buf.begin()){
                    if(*(buf.end()-1)!='\\'){
                        buf.push_back(self->achar());
                        self->increment();
                        //self->set_ignore(ctx);
                        return true;
                    }
                }
                if(noline){
                    if(self->achar()==(Char)'\n'||self->achar()==(Char)'\r'){
                        noline=false;
                        return true;
                    }
                }
                buf.push_back(self->achar());
                return false;
            }
        }

        template<class Ctx=void*,class Ret>
        bool readwhile(Ret& ret,bool(*check)(Reader*,Ret&,Ctx&,bool),Ctx ctx=0,bool usecheckres=false){
            if(!check)return false;
            ignore();
            if(!check(this,ret,ctx,true))return false;
            auto ig=set_ignore(nullptr);
            bool ok=false;
            while(!endbuf()){
                if(check(this,ret,ctx,false)){
                    ok=true;
                    break;
                }
                pos++;
            }
            if(endbuf()){
                auto t=check(nullptr,ret,ctx,false);
                if(usecheckres)ok=t;
            }
            set_ignore(ig);
            return ok;
        }

        template<class Ctx>
        bool readwhile(bool(*check)(Reader*,Ctx&,bool),Ctx& ctx,bool usecheckres=false){
            if(!check)return false;
            if(!check(this,ctx,true))return false;
            auto ig=set_ignore(nullptr);
            bool ok=false;
            while(!endbuf()){
                if(check(this,ctx,false)){
                    ok=true;
                    break;
                }
                pos++;
            }
            if(endbuf()){
                auto t=check(nullptr,ctx,false);
                if(usecheckres)ok=t;
            }
            set_ignore(ig);
            return ok;
        }

        template<class Ctx>
        bool readwhile(bool(*check)(Reader*,Ctx,bool),Ctx ctx,bool usecheckres=false){
            if(!check)return false;
            if(!check(this,ctx,true))return false;
            auto ig=set_ignore(nullptr);
            bool ok=false;
            while(!endbuf()){
                if(check(this,ctx,false)){
                    ok=true;
                    break;
                }
                pos++;
            }
            if(endbuf()){
                auto t=check(nullptr,ctx,false);
                if(usecheckres)ok=t;
            }
            set_ignore(ig);
            return ok;
        }


        template<class Ret>
        bool string(Ret& ret,bool noline=true){
            auto first=noline;
            auto res=readwhile(ret,read_string,noline);
            if(res){
                if(noline!=first)return false;
            }
            return res;
        }
    };

    template<class Buf>
    bool ignore_space(Buf& buf,size_t& pos){
        using Char=remove_cv_ref<decltype(buf[0])>;
        while(buf.size()>pos){
            if(buf[pos]==(Char)' '||buf[pos]==(Char)'\t'){
                pos++;
                continue;
            }
            break;
        }
        return buf.size()>pos;
    }
    
    template<class Buf>
    bool ignore_space_line(Buf& buf,size_t& pos){
        using Char=remove_cv_ref<decltype(buf[0])>;
        while(buf.size()>pos){
            auto c=buf[pos];
            if(c==(Char)' '||c==(Char)'\t'||c==(Char)'\n'||c==(Char)'\r'){
                pos++;
                continue;
            }
            break;
        }
        return buf.size()>pos;
    }

    template<class Buf>
    bool ignore_c_comments(Buf& buf,size_t& pos){
        using Char=remove_cv_ref<decltype(buf[0])>;
        auto size= [&](int ofs=0){return buf.size()>(pos+ofs);};
        while(size()){
            auto c=buf[pos];
            if(c==(Char)' '||c==(Char)'\t'||c==(Char)'\n'||c==(Char)'\r'){
                pos++;
                continue;
            }
            else if(c==(Char)'/'&&size(1)&&buf[pos+1]==(Char)'*'){ 
                pos+=2;
                bool ok=false;
                while(size()){
                    if(buf[pos]==(Char)'*'&&size(1)&&buf[pos+1]==(Char)'/'){
                        ok=true;
                        pos+=2;
                        break;
                    }
                    pos++;
                }
                if(!ok)return false;
                continue;
            }
            else if(c==(Char)'/'&&size(1)&&buf[pos+1]==(Char)'/'){
                pos+=2;
                while(size()){
                    if(buf[pos]==(Char)'\n'||buf[pos]==(Char)'\r'){
                        pos++;
                        break;
                    }
                    pos++;
                }
                continue;
            }
            break;
        }
        return size();
    }
    
    template<class Buf>
    using cmpf_t=typename Reader<Buf>::cmp_default;

    template<class Buf>
    using nexpf_t=typename Reader<Buf>::not_expect_default;
}