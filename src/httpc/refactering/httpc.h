#pragma once
#ifdef _WIN32
#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllimport) 
#endif
#define STDCALL __stdcall
#else
#define DLL_EXPORT
#define STDCALL
#endif

#ifdef __cplusplus
extern "C"{
#endif
    DLL_EXPORT int STDCALL command(const char* input,int internal);
    DLL_EXPORT int STDCALL command_wchar(const wchar_t* input,int internal);
    DLL_EXPORT int STDCALL command_argv(int argc,char** argv);
#ifdef __cplusplus
}
#endif