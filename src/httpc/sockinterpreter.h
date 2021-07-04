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
    DLL_EXPORT int STDCALL netclient_str(const char* str);

    DLL_EXPORT int STDCALL netclient_argv(int argc,char** argv);

    DLL_EXPORT int STDCALL http2_test();
#ifdef __cplusplus
}
#endif