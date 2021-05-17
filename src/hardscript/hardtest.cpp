/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#define DLL_EXPORT __declspec(dllexport)
#include"hardtest.h"
#include"control.h"

using namespace PROJECT_NAME;
using namespace control;

template<class T>
T testfunc(){
    auto i=T();
    return i;
}

template<class T,class Func=decltype(testfunc<T>)>
T test2(Func f=testfunc<T>){
    return f();
}


int STDCALL hardtest(){
    auto code=
R"(
    /*decl printf(string,...);
    //decl call([]string)->int;
    func main(argv string...)-> (int,error) -> void {
        if argv.len() < 1 {
            return 0;
        }
        printf(argv[0]);
        return ;
    }

    call:=main|0;

    a=[0,1,2,3,4][4];

    call();*/

    $(i,e)->void{print(i,e);};
)";
    Reader<std::string> test(code,ignore_c_comments);
    std::vector<Control> globalvec;
    auto t=parse_all(test,globalvec);
    LinePosContext ctx;
    test.readwhile(linepos,ctx);

    return 0;
}

