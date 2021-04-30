#pragma once
#ifndef DLL_EXPORT 
#define DLL_EXPORT __declspec(dllimport)
#endif
#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

#ifdef __cplusplus
extern"C"{
#endif
    DLL_EXPORT int STDCALL hardtest();
#ifdef __cplusplus
}
#endif