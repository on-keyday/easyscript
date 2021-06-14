#pragma once
#include<iostream>
namespace ast{

    static size_t alloced=0;

    template<class T>
    T* new_(){
        alloced++;
        return new T();
    }

    template<class T>
    void delete_(T* t){
        if(!t)return;
        alloced--;
        delete t;
    }

    inline void check_assert(){
        if(alloced){
            std::cout << "remain "<< alloced << "\n";
            assert(false);
        }
    }

    template<class T,class... Arg>
    T* new_(Arg&&... a){
        return new T(a...);
    }
}