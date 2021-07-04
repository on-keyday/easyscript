#pragma once
#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllimport) 
#endif
#define STDCALL __stdcall

#ifdef __cplusplus
extern "C"{
#endif
    DLL_EXPORT int STDCALL command(const char* input);
    DLL_EXPORT int STDCALL command_wchar(const wchar_t* input);
    DLL_EXPORT int STDCALL command_argv(int argc,char** argv);
#ifdef __cplusplus
}
#endif