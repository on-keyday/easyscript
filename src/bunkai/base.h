#pragma once
#include<fstream>
#include<iostream>
namespace ast{

    static size_t alloced=0;
    static size_t size=0;

    static std::map<void*,size_t> refs;

    static std::ofstream profile("memory.log");

    template<class O>
    void memprint(O& out,void* p,long long in){
        out << "now:" << (uintptr_t)p << ",size:"<< in <<",sum:" << size <<",remain:"<< alloced <<  "\n";
    }

    inline void peek_mem(void* p,long long in){
        memprint(std::cout,p,in);
        memprint(profile,p,in);
    }

    template<class T>
    T* new_(){
        alloced++;
        size+=sizeof(T);
        auto p= new T();
        refs[(void*)p]=sizeof(T);
        peek_mem(p,sizeof(T));
        if(alloced==84){
            return p;
        }
        return p;
    }

    template<class T>
    void delete_(T* t){
        if(!t)return;
        alloced--;
        size-=sizeof(T);
        peek_mem(t,-static_cast<long long>(sizeof(T)));
        refs.erase((void*)t);
        delete t;
    }

    inline void check_assert(){
        if(alloced){
            auto print=[](auto& o,auto& v){
                o<< "\"pointer\":" << (uintptr_t)v.first << ",size:" <<v.second << "\n";
            };
            for(auto& v:refs){
                print(std::cout,v);
                print(profile,v);
            }
            //assert(false);
        }
    }

    template<class T,class... Arg>
    T* new_(Arg&&... a){
        return new T(a...);
    }
}