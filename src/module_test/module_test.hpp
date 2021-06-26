module;
#include<reader.h>
export module commonlib;

export template<class T>
T add(T a,T b){
    return a+b;
}

export namespace commonlib_mod{
    template<class T>
    using Reader=PROJECT_NAME::Reader<T>;
}
