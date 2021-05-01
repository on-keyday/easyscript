#define DLL_EXPORT __declspec(dllexport)
#include"hardtest.h"
#include"control.h"

using namespace PROJECT_NAME;
using namespace control;

int STDCALL hardtest(){
    Reader<std::string> test("1?true:0?\ntrue:\nfalse",ignore_c_comments);
    auto i=expr_parse(test);
    delete i;
    LinePosContext ctx;
    test.readwhile(linepos,ctx);
    return 0;
}
