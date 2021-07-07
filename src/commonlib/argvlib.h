#pragma once
#include"project_name.h"
#ifdef _WIN32
#include"extension_operator.h"
#include<vector>
#include<string_view>
#include<Windows.h>
#endif
namespace PROJECT_NAME{

    template<class String,template<class ...>class Vec>
    struct U8Arg{
#ifdef _WIN32
    private:
        Vec<char*> u8argv;
        Vec<String> base;
        char** baseargv;
        int baseargc;
        char*** baseargv_ptr;
        int* baseargc_ptr;

        bool get_warg(int& wargc,wchar_t**& wargv){
            return (bool)(wargv=CommandLineToArgvW(GetCommandLineW(),&wargc));
        }
    public:
        explicit U8Arg(int& argc,char**& argv)
        :baseargc(argc),baseargv(argv),baseargc_ptr(&argc),baseargv_ptr(&argv){
            int wargc=0;
            wchar_t** wargv=nullptr;
            if(!get_warg(wargc,wargv)){
                throw "system is broken";
            }
            for(auto i=0;i<wargc;i++){
                String arg;
                StrStream(std::wstring_view(wargv[i]))>>u8filter>>arg;
                base.push_back(std::move(arg));
            }
            LocalFree(wargv);
            for(auto& i:base){
                u8argv.push_back(i.data());
            }
            u8argv.push_back(nullptr);
            argc=(int)u8argv.size()-1;
            argv=u8argv.data();
        }

        ~U8Arg(){
            *baseargv_ptr=baseargv;
            *baseargc_ptr=baseargc;
        }
#else
        explicit U8Arg(int& argc,char**& argv){

        }
        ~U8Arg(){
        }
#endif
    };

    using ArgChange=U8Arg<std::string,std::vector>;


//以下stdの実装模倣


    struct Anything{
        struct BaseTy{
            virtual ~BaseTy(){}
        };

        template<class T>
        struct HolderTy:public BaseTy{
            T t;
            HolderTy(T&& in):t(std::forward<T>(in)){}
            HolderTy(const T& in):t(in){}
            ~HolderTy(){}
        };

        BaseTy* hold=nullptr;
        size_t refsize=0;
        size_t Tsize=0;

        Anything(){}
        
        template<class T>
        Anything(const T& in){
            hold=new HolderTy(in);
            refsize=sizeof(HolderTy<T>);
            Tsize=sizeof(T);
        }

        template<class T>
        Anything(T&& in){
            hold=new HolderTy(std::forward<T>(in));
            refsize=sizeof(HolderTy<T>);
            Tsize=sizeof(T);
        }

        Anything& operator=(Anything&& in){
            delete hold;
            hold=in.hold;
            in.hold=nullptr;
            refsize=in.refsize;
            in.refsize=0;
            Tsize=in.Tsize;
            in.Tsize=0;
            return *this;
        }

        Anything& operator=(const Anything& in)=delete;
    };

    

    template<class T,class... Arg>
    struct Tuple{
    private:
        T val;
        Tuple<Arg...> next;
        constexpr static size_t _size=sizeof...(Arg)+1;

        constexpr size_t size()const{return _size;}
    public:
        constexpr Tuple(T&& in,Arg&&... arg):val(std::forward<T>(in)),next(std::forward<Arg>(arg)...){}

        constexpr Tuple(const Tuple& arg):val(arg.val),next(arg.next){}

        constexpr bool operator==(const Tuple& in)const{
            return val==in.val&&next==in.next;
        }

        template<size_t pos,class Ret=std::enable_if_t<pos==0,T>>
        constexpr Ret& get(){
            return val;
        }

        template<size_t pos,class Ret=std::enable_if_t<pos!=0,T>>
        constexpr decltype(auto) get(){
            return next.template get<pos-1>();
        }

        template<class Func>
        void for_each(Func func){
            func(val);
            next.for_each(func);
        }

        template<class Func>
        auto put(Func func){
            return func(val,next.put(func));
        }
    };

    template<class T>
    struct Tuple<T>{
    private:
        T val;

        constexpr static size_t _size=1;
    public:
        constexpr Tuple(T&& in):val(std::forward<T>(in)){}

        constexpr Tuple(const Tuple& arg):val(arg.val){}

        constexpr size_t size()const{return _size;}

        constexpr bool operator==(const Tuple& in)const{
            return val==in.val;
        }

        template<size_t pos,class Ret=std::enable_if_t<pos==0,T>>
        constexpr Ret& get(){
            return val;
        }

        template<class Func>
        void for_each(Func func){
            func(val);
        }

        template<class Func>
        auto put(Func func){
            return val;
        }
    };

    template<class... Arg>
    struct Tuple<void,Arg...>;

    template<>
    struct Tuple<void>;

    template<class Func,class C=void*,class... Arg1,class... Arg2>
    void tuple_op(Func func,const Tuple<Arg1...>& t1,const Tuple<Arg2...>& t2,C ctx=nullptr){
        func(t1.val,t2.val,ctx);
        tuple_op(func,t1.next,t2.next,ctx);
    }

    template<class Func,class C=void*,class Arg1,class Arg2>
    void tuple_op(Func func,const Tuple<Arg1>& t1,const Tuple<Arg2>& t2,C ctx=nullptr){
        func(ctx,t1.val,t2.val,ctx);
    }
}