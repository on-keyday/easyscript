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
    DLL_EXPORT int STDCALL hardtest();
#ifdef __cplusplus
}
#endif