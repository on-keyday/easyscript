/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "extension_operator.h"
#include "project_name.h"
#ifdef _WIN32
#include <Windows.h>

#include <string_view>
#include <vector>
#endif
namespace PROJECT_NAME {

    template <class C, class String, template <class...> class Vec>
    struct ArgArray {
        Vec<C *> _argv;
        Vec<String> base;

        template <class Input>
        void push_back(Input &&str) {
            String out;
            Reader(std::move(str)) >> out;
            base.push_back(std::move(out));
        }

        template <class Input>
        void push_back(const Input &str) {
            String out;
            Reader(str) >> out;
            base.push_back(std::move(out));
        }

        template <class Input>
        void translate(Input &&vec) {
            for (auto &&i : vec) {
                push_back(std::forward<decltype(i)>(i));
            }
        }

        C **argv(int &argc) {
            _argv.clear();
            for (auto &i : base) {
                _argv.push_back(i.data());
            }
            _argv.push_back(nullptr);
            argc = (int)(_argv.size() - 1);
            return _argv.data();
        }
    };

    template <class String, template <class...> class Vec>
    struct U8Arg {
#ifdef _WIN32
       private:
        ArgArray<char, String, Vec> base;
        char **baseargv;
        int baseargc;
        char ***baseargv_ptr;
        int *baseargc_ptr;

        bool get_warg(int &wargc, wchar_t **&wargv) {
            return (bool)(wargv = CommandLineToArgvW(GetCommandLineW(), &wargc));
        }

       public:
        explicit U8Arg(int &argc, char **&argv)
            : baseargc(argc), baseargv(argv), baseargc_ptr(&argc), baseargv_ptr(&argv) {
            int wargc = 0;
            wchar_t **wargv = nullptr;
            if (!get_warg(wargc, wargv)) {
                throw "system is broken";
            }
            for (auto i = 0; i < wargc; i++) {
                //String arg;
                //StrStream(std::wstring_view(wargv[i]))>>u8filter>>arg;
                base.push_back(wargv[i]);
            }
            LocalFree(wargv);
            /*for(auto& i:base){
                u8argv.push_back(i.data());
            }
            u8argv.push_back(nullptr);
            argc=(int)u8argv.size()-1;
            argv=u8argv.data();*/
            argv = base.argv(argc);
        }

        ~U8Arg() {
            *baseargv_ptr = baseargv;
            *baseargc_ptr = baseargc;
        }
#else
        explicit U8Arg(int &argc, char **&argv) {}
        ~U8Arg() {}
#endif
    };

    using ArgChange = U8Arg<std::string, std::vector>;

}  // namespace PROJECT_NAME