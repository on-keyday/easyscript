/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#ifndef DLL_EXPORT 
#define DLL_EXPORT __declspec(dllimport)
#endif
#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif
extern"C"{
    typedef struct Script Script;

    typedef struct ArgContext ArgContext;

    DLL_EXPORT Script* STDCALL make_script();

    DLL_EXPORT int STDCALL add_sourece(Script* self,const char* str,size_t len);

    DLL_EXPORT int STDCALL add_sourece_from_file(Script* self,const char* filename);

    typedef int(*ObjProxy)(void* ctx,const char* membname,ArgContext* arg);

    DLL_EXPORT int STDCALL add_builtin_object(Script* self,const char* objname,ObjProxy proxy,void* ctx);

    DLL_EXPORT int STDCALL execute(Script* self,unsigned char flag);

    DLL_EXPORT const char* STDCALL get_result(Script* self);

    DLL_EXPORT int STDCALL delete_script(Script* self);

    DLL_EXPORT const char* STDCALL get_arg_index(ArgContext* arg,size_t i);
}