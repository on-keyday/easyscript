/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include <fstream>
#include <iostream>
#include <sstream>

#include "extension_operator.h"
#include "project_name.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace PROJECT_NAME {
    struct IOWrapper {
       private:
        static bool& Flag() {
            static bool flag = false;
            return flag;
        }

        static bool Init_impl(bool sync, const char*& err) {
            if (Flag()) return true;
            Flag() = true;
#ifdef _WIN32
            if (_setmode(_fileno(stdin), _O_U16TEXT) == -1) {
                err = "error:text input mode change failed";
                return false;
            }
            if (_setmode(_fileno(stdout), _O_U8TEXT) == -1) {
                err = "error:text output mode change failed\n";
                return false;
            }
            if (_setmode(_fileno(stderr), _O_U8TEXT) == -1) {
                err = "error:text error mode change failed\n";
                return false;
            }
#endif
            std::ios_base::sync_with_stdio(sync);
            return true;
        }

       public:
        static bool Init(bool sync = false, const char** err = nullptr) {
            static const char* errmsg = nullptr;
            static bool result = Init_impl(sync, errmsg);
            if (err) *err = errmsg;
            return result;
        }

        static bool Initialized() {
            return Flag();
        }

       protected:
        static bool Able_continue() {
#ifdef _WIN32
            return Initialized() && Init(false, nullptr);
#else
            Flag() = true;
            return true;
#endif
        }
    };

    struct CinWrapper : IOWrapper {
       private:
#ifdef _WIN32
        std::wistream& in;
#else
        std::istream& in;
#endif
       public:
        using char_type = typename std::remove_reference_t<decltype(in)>::char_type;

        CinWrapper(decltype(in) st)
            : in(st) {}

        template <class T>
        CinWrapper& operator>>(T& out) {
            if (!Able_continue()) throw std::runtime_error("not called IOWrapper::Init() before io function");
            in >> out;
            return *this;
        }

        CinWrapper& operator>>(std::string& out) {
            if (!Able_continue()) throw std::runtime_error("not called IOWrapper::Init() before io function");
#ifdef _WIN32
            std::basic_string<char_type> tmp;
            in >> tmp;
            Reader(tmp) >> out;
#else
            in >> out;
#endif
            return *this;
        }

        CinWrapper& getline(std::string& out) {
            if (!Able_continue()) throw std::runtime_error("not called IOWrapper::Init() before io function");
#ifdef _WIN32
            std::basic_string<char_type> tmp;
            std::getline(in, tmp);
            Reader(tmp) >> out;
#else
            std::getline(in, out);
#endif
            return *this;
        }
    };

    struct CoutWrapper : IOWrapper {
       private:
        std::ofstream file;
        std::ostringstream ss;
        bool onlybuffer = false;
#ifdef _WIN32
        std::wostream& out;
#else
        std::ostream& out;
#endif
       public:
        using char_type = typename std::remove_reference_t<decltype(out)>::char_type;
        CoutWrapper(decltype(out) st)
            : out(st) {}

        template <class T>
        CoutWrapper& operator<<(const T& in) {
            if (onlybuffer) {
                ss << in;
                return *this;
            }

            if (file.is_open()) {
                file << in;
                return *this;
            }

            if (!Able_continue()) throw std::runtime_error("not called IOWrapper::Init() before io function");

#ifdef _WIN32
            //auto tmp= ss.flags();
            ss.str("");
            //ss.setf(tmp);
            std::wstring str;
            ss << in;
            Reader(ss.str()) >> str;
            out << str;

#else
            out << in;
#endif

            return *this;
        }

        CoutWrapper& operator<<(std::ios_base& (*in)(std::ios_base&)) {
            if (onlybuffer) {
                ss << in;
                return *this;
            }
            if (file.is_open()) {
                file << in;
                return *this;
            }
            if (!Able_continue()) throw std::runtime_error("not called IOWrapper::Init() before io function");
#ifdef _WIN32
            ss << in;
#else
            out << in;
#endif
            return *this;
        }

        bool open(const std::string& in) {
#if _WIN32
            std::wstring tmp;
            Reader(in) >> tmp;
            file.open(tmp);
#else
            file.open(in);
#endif
            return (bool)file;
        }

        bool stop_out(bool stop) {
            auto ret = onlybuffer;
            onlybuffer = stop;
            return ret;
        }

        bool stop_out() {
            return onlybuffer;
        }

        std::string buf_str() {
            return ss.str();
        }

        void reset_buf() {
            ss.str("");
            ss.clear();
        }
    };

    inline CinWrapper& cin_wrapper() {
#ifdef _WIN32
        static CinWrapper Cin(std::wcin);
#else
        static CinWrapper Cin(std::cin);
#endif
        return Cin;
    }

    inline CoutWrapper& cout_wrapper() {
#ifdef _WIN32
        static CoutWrapper Cout(std::wcout);
#else
        static CoutWrapper Cout(std::cout);
#endif
        return Cout;
    }

    inline CoutWrapper& cerr_wrapper() {
#ifdef _WIN32
        static CoutWrapper Cerr(std::wcerr);
#else
        static CoutWrapper Cerr(std::cerr);
#endif
        return Cerr;
    }

    inline CoutWrapper& clog_wrapper() {
#ifdef _WIN32
        static CoutWrapper Clog(std::wclog);
#else
        static CoutWrapper Clog(std::clog);
#endif
        return Clog;
    }

}  // namespace PROJECT_NAME
