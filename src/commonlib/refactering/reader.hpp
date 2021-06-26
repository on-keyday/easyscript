#pragma once
#include"typeutil.hpp"
namespace commonlib{

    template<class Buffer>
    struct reader_base{
    private:
        Buffer buf;
    public:
        using char_type=arrelm_remove_cvref_t<Buffer>;
        using buffer_type=Buffer;
        
        constexpr size_t size() const{
            return buf.size();
        }

        constexpr char_type operator[](size_t s){
            return buf[s];
        }

        constexpr char_type operator[](size_t s)const{
            return buf[s];
        }

        buffer_type& bufctrl(){
            return buf;
        }
        constexpr reader_base():buf(buffer_type()){}
        constexpr reader_base(const buffer_type& in):buf(in){}
        constexpr reader_base(buffer_type&& in):buf(std::forward<buffer_type>(in)){}
    };

    template<class Str>
    constexpr size_t str_length(Str str){
        size_t i=0;
        while(str[i]!=0){
            if(str[i+1]==0)return i+1;
            if(str[i+2]==0)return i+2;
            if(str[i+3]==0)return i+3;
            if(str[i+4]==0)return i+4;
            if(str[i+5]==0)return i+5;
            if(str[i+6]==0)return i+6;
            if(str[i+7]==0)return i+7;
            if(str[i+8]==0)return i+8;
            if(str[i+9]==0)return i+9;
            if(str[i+10]==0)return i+10;
            if(str[i+11]==0)return i+11;
            if(str[i+12]==0)return i+12;
            if(str[i+13]==0)return i+13;
            if(str[i+14]==0)return i+14;
            if(str[i+15]==0)return i+15;
            i+=16;
        }
        return i;
    }

    template<class Buffer>
    struct reader{
        using Base=reader_base<Buffer>;
        using char_type=typename Base::char_type;
        using buffer_type=typename Base::buffer_type;
        using ignore_cb_type=bool(*)(const Base&,size_t&);
        using nexpdef_t=bool(*)(char_type);
        template<class C>
        using cmpdef_t=bool(*)(larger_type<char_type,C> l,larger_type<char_type,C> r);
        template<class Str>
        using cmpdef_bt=cmpdef_t<arrelm_remove_cvref_t<Str>>;

        template<class C>
        constexpr static bool cmpdef(larger_type<char_type,C> l,larger_type<char_type,C> r){
            return l==r;
        }
    private:
        template<class T>
        using elty=arrelm_remove_cvref_t<T>;
        Base buf;
        size_t position=0;
        ignore_cb_type ignore_cb=nullptr;

        constexpr size_t size()const{return buf.size();}
        
        template<class Str,class Cmp>
        constexpr size_t peek_impl(size_t bufsize,size_t start,const Str& target,
                                   size_t target_size,Cmp&& cmp)const{
            size_t pos=start;
            size_t i=0;
            for(;i<target_size&&pos+i<bufsize;i++){
                if(!cmp(buf[pos+i],target[i]))return i;
            }
            return i;
        }

        template<class Str,class Cmp>
        constexpr size_t filt_impl(size_t bufsize,size_t start,size_t tgsize,
                                   const Str& target,Cmp&& cmp)const{
            size_t cmpsize=peek_impl(bufsize,start,target,tgsize,std::forward<Cmp>(cmp));
            if(tgsize!=cmpsize)return 0;
            return tgsize;
        }

        template<class O>
        constexpr static bool invokeable(bool(*&&nexp)(O i)){
            if(!nexp)return false;
            return true;
        }

        template<class Func>
        constexpr static bool invokeable(Func&& func){
            return true;
        }

        template<class Str,class Cmp,class NotExpect>
        constexpr size_t ahead_impl(size_t start,size_t tgsize,const Str& target,NotExpect&& nexp,Cmp&& cmp)const{
            size_t bufsize=size();
            if(auto s=filt_impl(bufsize,start,tgsize,target,std::forward<Cmp>(cmp))){
                if(invokeable(std::forward<NotExpect>(nexp))&&
                   (start+s>=bufsize||nexp(buf[start+s])))return 0;
                return s;
            }
            return 0;
        }

        template<class Str,class Cmp,class NotExpect>
        inline size_t ahead_invoker(size_t start,size_t tgsize,const Str& str,NotExpect nexp,Cmp cmp){
            return ahead_impl(start,tgsize,str,
            std::forward<NotExpect>(nexp),std::forward<Cmp>(cmp));
        } 

        template<class Str,class Cmp,class NotExpect>
        inline size_t ahead_invoke(size_t start,const Str& str,NotExpect nexp,Cmp cmp){
            return ahead_invoker(start,str.size(),str,nexp,cmp);
        }

        template<class C,class Cmp,class NotExpect>
        inline size_t ahead_invoke(size_t start,const C* str,NotExpect nexp,Cmp cmp){
            if(!str)return 0;
            return ahead_invoker(start,str_length(str),str,nexp,cmp);
        }

        template<class C,class Cmp,class NotExpect>
        inline size_t ahead_invoke(size_t start,C* str,NotExpect nexp,Cmp cmp){
            if(!str)return 0;
            return ahead_invoker(start,str_length(str),str,nexp,cmp);
        }

        inline bool ignore(size_t& pos){
            return ignore_cb?ignore_cb(buf,pos):true;
        }

    public:
        constexpr reader(ignore_cb_type cb=nullptr):buf(Base()){ignore_cb=cb;}
        //constexpr reader(const Base& in,ignore_cb_type cb=nullptr):buf(in){ignore_cb=cb;}
        //constexpr reader(Base&& in,ignore_cb_type cb=nullptr):buf(std::forward<Base>(in)){ignore_cb=cb;}
        constexpr reader(const buffer_type& in,ignore_cb_type cb=nullptr):buf(in){ignore_cb=cb;}
        constexpr reader(buffer_type&& in,ignore_cb_type cb=nullptr):buf(std::forward<Base>(in)){ignore_cb=cb;}


        template<class Str,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        size_t aheadpos(size_t& pos,const Str& str,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            if(!ignore(pos))return 0;
            return ahead_invoke(pos,str,nexp,cmp);
        }

        template<class Str,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        size_t ahead(const Str& str,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            return aheadpos(position,str,nexp,cmp);
        }

        template<class Str,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        bool consumepos(size_t& pos,const Str& str,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            if(auto sz=aheadpos(pos,str,nexp,cmp)){
                pos+=sz;
                ignore(pos);
                return true;
            }
            return false;
        }

        template<class Str,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        bool consume(const Str& str,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            return consumepos(position,str,nexp,cmp);
        }

        template<class Str,class Ctx,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        bool consumep(const Str& str,Ctx& ctx,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            if(consume(str,nexp,cmp)){
                ctx=str;
                return true;
            }
            return false;
        }

        template<class Str,class Ctx,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        inline bool expectp(const Str& str,Ctx& ctx,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            return consumep(str,ctx,nexp,cmp);
        }

        template<class Str,class Cmp=cmpdef_bt<Str>,class NotExpect=nexpdef_t>
        inline bool expect(const Str& str,NotExpect nexp=NotExpect(),Cmp cmp=cmpdef< elty<Str> >){
            return consume(str,nexp,cmp);
        }

        constexpr bool seek(size_t pos,bool strict=false){
            size_t bufsize=size();
            if(pos>bufsize){
                if(strict)return false;
                pos=bufsize;
            }
            position=pos;
            return true;
        }

        constexpr void seekend(){
            position=size();
        }

        constexpr void seekbegin(){
            position=0;
        }

        constexpr bool increment(){
            return seek(position+1,true);
        }

        constexpr bool decrement(){
            return seek(position-1,true);
        }

        constexpr size_t readpos()const{return position;}

        constexpr bool ceofpos(size_t p)const{
            return size()<=position;
        }

        constexpr bool ceof()const{
            return ceof(position);
        }

        bool eofpos(size_t& pos){
            ignore(pos);
            return ceofpos(pos);
        }

        inline bool eof(){
            return eofpos(position);
        }

        constexpr buffer_type& ref(){return buf.bufctrl();}

        char_type offset(long long i)const{
            size_t pos=static_cast<size_t>(position+i);
            if(ceofpos(pos))return char_type();
            return buf[pos];
        }

        char_type achar()const{
            return ceof()?char_type():buf[position];
        }
    };
}