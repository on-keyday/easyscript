#pragma once
#include<iostream>
namespace ast{

    static size_t alloced=0;
    static size_t size=0;

    inline void peek_mem(){
        std::cout << "remain:"<< alloced << ",size:" << size << "\n";
    }

    template<class T>
    T* new_(){
        alloced++;
        size+=sizeof(T);
        peek_mem();
        return new T();
    }

    template<class T>
    void delete_(T* t){
        if(!t)return;
        alloced--;
        size-=sizeof(T);
        peek_mem();
        delete t;
    }

    inline void check_assert(){
        if(alloced){
            assert(false);
        }
    }

    template<class T,class... Arg>
    T* new_(Arg&&... a){
        return new T(a...);
    }
}