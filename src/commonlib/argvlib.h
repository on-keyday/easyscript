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

    


}