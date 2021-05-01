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

int STDCALL hardtest(){
    auto code="for i:=0;i<10;i++{break;} var(f &*(socket int,buf []byte,host string)->int real,imag float = sqrt(-1))";
    Reader<std::string> test(code,ignore_c_comments);
    std::vector<Control> vec;
    control_parse(test,vec);
    LinePosContext ctx;
    test.readwhile(linepos,ctx);
    return 0;
}
