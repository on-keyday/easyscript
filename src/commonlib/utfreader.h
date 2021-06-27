#include"utf_helper.h"
#include<string>
namespace PROJECT_NAME{
    template<class Buf>
    using b_char_type=typename Reader<Buf>::char_type;
    
    template<class Buf,int N=sizeof(b_char_type<Buf>)>
    struct ToUTF32;

    template<class Buf>
    struct ToUTF32_impl{
        mutable Reader<Buf> r;
        mutable bool err=false;
        mutable size_t prev=0;
        mutable char32_t chache=0;

        template<class Func>
        char32_t get_achar(Func func)const{
            int ierr=0;
            auto c=func(&r,&ierr);
            r.increment();
            if(ierr){
                err=true;
            }
            return c;
        }

        template<class Func>
        size_t count(Func func)const{
            r.seek(0);
            size_t c=0;
            while(!r.ceof()){
                get_achar(func);
                if(err){
                    return 0;
                }
                c++;
            }
            r.seek(0);
            return c;
        }

        template<class Func>
        bool seekminus(Func func,long long ofs)const{
            if(ofs>=0)return false;
            for(auto i=-1;i>ofs;i--){
                if(!func(&r)){
                    err=true;
                    return false;
                }
            }
            return true;
        }

        template<class Func1,class Func2>
        char32_t get_position(Func1 func1,Func2 func2,size_t pos)const{
            if(prev-1==pos){
                return chache;
            }
            if(pos<prev){
                seekminus(func1,static_cast<long long>(pos-prev-1));
            }
            else{
                for(auto i=prev;i<pos-prev;i++){
                    get_achar(func2);
                    if(err)return char32_t();
                }
            }
            chache=get_achar(func2);
            if(err)return char32_t();
            prev=pos+1;
            return chache;
        }

        size_t reset(size_t pos=0){
            err=false;
            r.seek(0);
            auto ret=count();
            r.seek(pos);
            return ret;
        }

        Buf& ref(){
            return r.ref();
        }

        ToUTF32_impl(){}

        ToUTF32_impl(Buf&& in):r(std::forward<Buf>(in)){}
        ToUTF32_impl(const Buf& in):r(in){}
    };

    template<class Buf,class Tmp>
    struct FromUTF32_impl{
        using char_type=std::make_unsigned_t<b_char_type<Buf>>;
        mutable Tmp minbuf;
        mutable Reader<Buf> r;
        mutable size_t ofs=0;
        mutable bool err=false;
        mutable size_t prev=0;

        template<class Func>
        bool parse(Func func)const{
            minbuf.reset();
            int ctx=0;
            int* ctxp=&ctx;
            func(&r,minbuf,ctxp,false);
            if(ctx){
                err=true;
                return false;
            }
            ofs=0;
            return true;
        }

        template<class Func>
        bool increment(Func func)const{
            ofs++;
            if(ofs>=minbuf.size()){
                r.increment();
                if(r.ceof())return false;
                if(!parse(func))return false;
            }
            return true;
        }

        template<class Func>
        bool decrement(Func func)const{
            if(ofs==0){
                r.decrement();
                ofs=0;
                minbuf.reset();
                parse(func);
                if(err)return false;
                ofs=minbuf.size()-1;
            }
            else{
                ofs--;
            }
            return true;
        }

        template<class Func>
        size_t count(Func func){
            size_t c=0;
            while(!r.ceof()){
                increment(func);
                if(err)return 0;
                c+=minbuf.size();
                ofs=minbuf.size();
            }
            r.seek(0);
            ofs=0;
            minbuf.reset();
            parse(func);
            return c;
        }

        template<class Func>
        char_type get_position(Func func,size_t pos)const{
            if(prev==pos){
                return minbuf[ofs];
            }
            if(prev<pos){
                for(auto i=0;i<pos-prev;i++){
                    increment(func);
                    if(err)return char();
                }
            }
            else{
                for(auto i=0;i<prev-pos;i++){
                    decrement(func);
                    if(err)return char();
                }
            }
            prev=pos;
            return minbuf[ofs];
        }

        size_t ofset_from_head(){
            return ofs;
        }

        size_t offset_to_next(){
            return minbuf.size()-ofs;
        }

        FromUTF32_impl(){}

        FromUTF32_impl(Buf&& in):r(std::forward<Buf>(in)){}
        FromUTF32_impl(const Buf& in):r(in){}

        Buf& ref(){
            return r.ref();
        }
    };

    template<class Buf>
    struct ToUTF32<Buf,1>{
    private:
        ToUTF32_impl<Buf> impl;
        //static_assert(sizeof(b_char_type<Tmp>)==1,"");
        /*mutable Reader<Buf> r;
        mutable bool err=false;
        mutable size_t prev=0;
        mutable char32_t chache=0;*/
        size_t _size=0;
        /*char32_t get_achar()const{
            int ierr=0;
            auto c=utf8toutf32_impl(&r,&ierr);
            r.increment();
            if(ierr){
                err=true;
            }
            return c;
        }

        size_t count()const{
            size_t c=0;
            while(!r.ceof()){
                auto q=get_achar();
                if(err){
                    return 0;
                }
                c++;
            }
            r.seek(0);
            return c;
        }

        bool seekminus(long long ofs)const{
            if(ofs>=0)return false;
            for(auto i=-1;i>ofs;i--){
                if(!utf8seek_minus(&r)){
                    err=true;
                    return false;
                }
            }
            return true;
        }

        char32_t get_position(size_t pos)const{
            if(prev-1==pos){
                return chache;
            }
            if(pos<prev){
                seekminus(static_cast<long long>(pos-prev-1));
            }
            else{
                for(auto i=prev;i<pos-prev;i++){
                    get_achar();
                    if(err)return char32_t();
                }
            }
            chache=get_achar();
            if(err)return char32_t();
            prev=pos+1;
            return chache;
        }*/

        
        
    public:
        ToUTF32(){
            _size=impl.count(utf8toutf32_impl<Buf>);
        }

        ToUTF32(Buf&& in):impl(std::forward<Buf>(in)){
            _size=impl.count(utf8toutf32_impl<Buf>);
        }

        ToUTF32(const Buf& in):impl(in){
            _size=impl.count(utf8toutf32_impl<Buf>);
        }

        char32_t operator[](size_t s)const{
            if(impl.err)return char32_t();
            return _size<=s?char32_t():
            impl.get_position(utf8seek_minus<Buf>,utf8toutf32_impl<Buf>, s);
        }

        size_t size()const{
            return _size;
        }

        Buf* operator->()const{
            return std::addressof(impl.ref());
        }

        bool reset(size_t pos=0){
            /*err=false;
            _size=count();
            r.seek(pos);
            return true;*/
            _size=impl.reset(pos);
            return impl.err;
        }
    };

    template<class Buf>
    struct ToUTF32<Buf,2>{
        ToUTF32_impl<Buf> impl;
        size_t _size=0;

        /*char32_t get_achar()const{
            int ierr=0;
            auto c=utf16toutf32_impl(&r,ierr);
            if(ierr){
                err=true;
            }
            return 0;
        }

        bool seekminus(long long ofs)const{
            if(ofs>=0)return false;
            for(auto i=-1;i>ofs;i--){
                if(!utf16seek_minus(&r)){
                    err=true;
                    return false;
                }
            }
            return true;
        }*/
    public:
        ToUTF32(){
            _size=impl.count(utf16toutf32_impl<Buf>);
        }

        ToUTF32(const Buf& in):impl(in){}
        
        ToUTF32(Buf&& in):impl(std::forward<Buf>(in)){}

        char32_t operator[](size_t s)const{
            if(impl.err)return char32_t();
            return _size<=s?char32_t():
                  impl.get_position(utf16seek_minus<Buf>,utf16toutf32_impl<Buf>,s);
        }

        size_t size()const{
            return _size;
        }
    };

    

    template<class Buf,int N=sizeof(b_char_type<Buf>)>
    struct ToUTF8;

    

    template<class Buf>
    struct ToUTF8<Buf,4>{
    private:
       /* mutable U8MiniBuffer minbuf;
        mutable Reader<Buf> r;
        mutable size_t ofs=0;
        mutable bool err=false;
        mutable size_t prev=0;*/
        FromUTF32_impl<Buf,U8MiniBuffer> impl;
        size_t _size=0;
        /*
        bool parse()const{
            minbuf.reset();
            int ctx=0;
            int* ctxp=&ctx;
            utf32toutf8(&r,minbuf,ctxp,false);
            if(ctx){
                err=true;
                return false;
            }
            ofs=0;
            return true;
        }

        bool increment()const{
            ofs++;
            if(ofs>=minbuf.size()){
                r.increment();
                if(r.ceof())return false;
                if(!parse())return false;
            }
            return true;
        }

        bool decrement()const{
            if(ofs==0){
                r.decrement();
                ofs=0;
                minbuf.reset();
                parse();
                if(err)return false;
                ofs=minbuf.size()-1;
            }
            else{
                ofs--;
            }
            return true;
        }

        size_t count(){
            size_t c=0;
            while(!r.ceof()){
                increment();
                if(err)return 0;
                c+=minbuf.size();
                ofs=minbuf.size();
            }
            r.seek(0);
            ofs=0;
            minbuf.reset();
            parse();
            return c;
        }
        
        unsigned char get_position(size_t pos)const{
            if(prev==pos){
                return minbuf[ofs];
            }
            if(prev<pos){
                for(auto i=0;i<pos-prev;i++){
                    increment();
                    if(err)return char();
                }
            }
            else{
                for(auto i=0;i<prev-pos;i++){
                    decrement();
                    if(err)return char();
                }
            }
            prev=pos;
            return minbuf[ofs];
        }*/
    public:
        ToUTF8(){
            _size=impl.count(utf32toutf8<Buf,U8MiniBuffer>);
        }

        ToUTF8(const Buf& in):impl(in){
            _size=impl.count(utf32toutf8<Buf,U8MiniBuffer>);
        }

        ToUTF8(Buf&& in):impl(std::forward<Buf>(in)){
            _size=impl.count(utf32toutf8<Buf,U8MiniBuffer>);
        }

        unsigned char operator[](size_t s)const{
            if(impl.err)return char();
            return s>=_size?char():
            (unsigned char)impl.get_position(utf32toutf8<Buf,U8MiniBuffer>,s);
        }

        size_t size()const{
            return _size;
        }

        size_t ofset_from_head(){
            return impl.ofset_from_head();
        }

        size_t offset_to_next(){
            return impl.offset_to_next();
        }

        Buf* operator->(){
            return std::addressof(impl.ref());
        }
    };

    template<class Buf,int N=sizeof(b_char_type<Buf>)>
    struct ToUTF16;
    
    template<class Buf>
    struct ToUTF16<Buf,4>{
    private:
        FromUTF32_impl<Buf,U16MiniBuffer> impl;
        size_t _size=0;
    public:
        ToUTF16(){
            _size=impl.count(utf32toutf16<Buf,U16MiniBuffer>);
        }

        ToUTF16(const Buf& in):impl(in){
            _size=impl.count(utf32toutf16<Buf,U16MiniBuffer>);
        }

        ToUTF16(Buf&& in):impl(std::forward<Buf>(in)){
            _size=impl.count(utf32toutf16<Buf,U16MiniBuffer>);
        }

        char16_t operator[](size_t s)const{
            if(impl.err)return char();
            return s>=_size?char():
            (char16_t)impl.get_position(utf32toutf16<Buf,U16MiniBuffer>,s);
        }

        size_t size()const{
            return _size;
        }

        size_t ofset_from_head(){
            return impl.ofset_from_head();
        }

        size_t offset_to_next(){
            return impl.offset_to_next();
        }

        Buf* operator->(){
            return std::addressof(impl.ref());
        }
    };

    template<class Buf>
    struct ToUTF16<Buf,1>{
    private:
        FromUTF32_impl<Buf,U16MiniBuffer> impl;
        size_t _size=0;
    public:
        ToUTF16(){
            _size=impl.count(utf8toutf16<Buf,U16MiniBuffer>);
        }

        ToUTF16(const Buf& in):impl(in){
            _size=impl.count(utf8toutf16<Buf,U16MiniBuffer>);
        }

        ToUTF16(Buf&& in):impl(std::forward<Buf>(in)){
            _size=impl.count(utf8toutf16<Buf,U16MiniBuffer>);
        }

        char16_t operator[](size_t s)const{
            if(impl.err)return char();
            return s>=_size?char():
            (char16_t)impl.get_position(utf8toutf16<Buf,U16MiniBuffer>,s);
        }

        size_t size()const{
            return _size;
        }

        size_t ofset_from_head(){
            return impl.ofset_from_head();
        }

        size_t offset_to_next(){
            return impl.offset_to_next();
        }

        Buf* operator->(){
            return std::addressof(impl.ref());
        }
    };

    template<class Buf>
    struct ToUTF8<Buf,2>{
    private:
        FromUTF32_impl<Buf,U16MiniBuffer> impl;
        size_t _size=0;
    public:
        ToUTF8(){
            _size=impl.count(utf16toutf8<Buf,U8MiniBuffer>);
        }

        ToUTF8(const Buf& in):impl(in){
            _size=impl.count(utf16toutf8<Buf,U8MiniBuffer>);
        }

        ToUTF8(Buf&& in):impl(std::forward<Buf>(in)){
            _size=impl.count(utf16toutf8<Buf,U8MiniBuffer>);
        }

        char16_t operator[](size_t s)const{
            if(impl.err)return char();
            return s>=_size?char():
            (char16_t)impl.get_position(utf16toutf8<Buf,U8MiniBuffer>,s);
        }

        size_t size()const{
            return _size;
        }

        size_t ofset_from_head(){
            return impl.ofset_from_head();
        }

        size_t offset_to_next(){
            return impl.offset_to_next();
        }

        Buf* operator->(){
            return std::addressof(impl.ref());
        }
    };
}