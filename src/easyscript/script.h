/*license*/
#pragma once
#ifdef _WIN32
#ifndef DLL_EXPORT 
#define DLL_EXPORT __declspec(dllimport)
#endif
#define STDCALL __stdcall
#else
#define STDCALL
#define DLL_EXPORT
#endif
#ifdef __cplusplus
extern"C"{
#endif
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

    DLL_EXPORT size_t STDCALL get_arg_len(ArgContext* arg);

    DLL_EXPORT int STDCALL set_result_error(ArgContext* arg,const char* str);  

    DLL_EXPORT int STDCALL set_result_bool(ArgContext* arg,int flag);

    DLL_EXPORT int STDCALL set_result_int(ArgContext* arg,long long num);

    DLL_EXPORT int STDCALL set_result_float(ArgContext* arg,double num);

    DLL_EXPORT int STDCALL set_result_str_len(ArgContext* arg,const char* str,size_t len);

    DLL_EXPORT int STDCALL set_result_str(ArgContext* arg,const char* str);

    DLL_EXPORT int STDCALL set_result_none(ArgContext* arg);

    DLL_EXPORT size_t STDCALL get_instance_id(ArgContext* arg);
#ifdef __cplusplus
}
#endif