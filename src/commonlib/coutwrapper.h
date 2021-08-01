#include"project_name.h"
#include"extension_operator.h"
#include<fstream>
#include<sstream>

#ifdef _WIN32
#include<io.h>
#include<fcntl.h>
#endif

namespace PROJECT_NAME{
    struct CoutWrapper{
    private:
        static bool Init_impl(bool sync,const char*& err){
#ifdef _WIN32
            if(_setmode(_fileno(stdin),_O_U16TEXT)==-1){
                err="error:text input mode change failed";
                return false;
            }
            if(_setmode(_fileno(stdout),_O_U8TEXT)==-1){
                err="error:text output mode change failed\n";
                return false;
            }
            if(_setmode(_fileno(stderr),_O_U8TEXT)==-1){
                err="error:text error mode change failed\n";
                return false;
            }
#endif
            std::ios_base::sync_with_stdio(sync);
            return true;
        }

        std::ofstream file;
        std::ostringstream ss;
        bool onlybuffer=false;
#ifdef _WIN32
        std::wostream& out;
#else
        std::ostream& out;
#endif
    public:
        static bool Init(bool sync=false,const char** err=nullptr){
            static const char* errmsg=nullptr;
            static bool result=Init_impl(sync,errmsg);
            if(err)*err=errmsg;
            return result;
        }

        
        CoutWrapper(decltype(out) st):out(st){}
        

        template<class T>
        CoutWrapper& operator<<(const T& in){
            if(onlybuffer){
                ss<<in;
            }
            else if(file.is_open()){
                file << in;
            }
            else{
#ifdef _WIN32 
                //auto tmp= ss.flags();
                ss.str("");
                //ss.setf(tmp);
                std::wstring str;
                ss<<in;
                Reader(ss.str())>>str;
                out << str;
        
#else   
                out << in;
#endif  
            }
            return *this;
        }

        CoutWrapper& operator<<(std::ios_base&(*in)(std::ios_base&)){
            if(onlybuffer){
                ss<<in;
            }
            else if(file.is_open()){
                file << in;
            }
            else{
#ifdef _WIN32
                ss << in;
#else
                out << in;
#endif 
            }
            return *this;
        }

        template<class C>
        bool open(const C& in){
            file.open(in);
            return (bool)file;
        }

        bool stop_out(bool stop){
            auto ret=onlybuffer;
            onlybuffer=stop;
            return ret;
        }

        bool stop_out(){
            return onlybuffer;
        }

        std::string buf_str(){
            return ss.str();
        }

        void reset_buf(){
            ss.str("");
            ss.clear();
        }
    };


}

