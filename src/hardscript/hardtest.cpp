#define DLL_EXPORT __declspec(dllexport)
#include"hardtest.h"
#include"control.h"

using namespace PROJECT_NAME;
using namespace control;

int STDCALL hardtest(){
    Reader<std::string> test("1?true:0?true:false",ignore_c_comments);
    auto i=expr_parse(test);
    delete i;
    return 0;
}
