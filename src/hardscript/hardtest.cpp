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
    auto code=
R"(
    var call ([]string)->int = null;
    func main(argv []string)->int{
        if argv.len() < 1 {
            return 0;
        }
        printf(argv[0]);
        return 0;
    }

    call=main;

    call();
)";
    Reader<std::string> test(code,ignore_c_comments);
    std::vector<Control> globalvec;
    auto t=parse_all(test,globalvec);
    LinePosContext ctx;
    test.readwhile(linepos,ctx);
    return 0;
}

