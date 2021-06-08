#pragma once
#include<project_name.h>
#define POSIX_SOURCE 200809L
#include<stdio.h>
#include<sys/stat.h>
#include<stddef.h>
#include<mutex>

#define COMMONLIB_MSB_FLAG(x) ((x)1<<(sizeof(x)*8-1))

#if defined(_WIN32)
#if defined(__GNUC__)
#define fopen_s(pfp,filename,mode) ((*pfp)=::fopen(filename,mode))
#define COMMONLIB_FILEIO_STRUCT_STAT struct stat
#define COMMONLIB_FILEIO_FUNC_FSTAT ::fstat
#else
#define COMMONLIB_FILEIO_FUNC_FSTAT ::_fstat64
#define COMMONLIB_FILEIO_STRUCT_STAT struct _stat64
#endif
#define COMMONLIB_FILEIO_FUNC_FSEEK ::_fseeki64
#define COMMONLIB_FILEIO_FSEEK_CAST(x) (long long)(x)
#else
#define fopen_s(pfp,filename,mode) ((*pfp)=fopen(filename.mode))
#define COMMONLIB_FILEIO_STRUCT_STAT struct stat
#define COMMONLIB_FILEIO_FUNC_FSTAT ::fstat
#define COMMONLIB_FILEIO_FUNC_FSEEK ::fseek
#define COMMONLIB_FILEIO_FSEEK_CAST(x) (long)(x)
#endif
        

namespace PROJECT_NAME{
    struct FileInput{
    private:
        ::FILE* file=nullptr;
        mutable size_t size_cache=0;
        mutable bool got_size=false;
        mutable size_t prevpos=0;
        mutable char samepos_cache=0;
    public:

        FileInput(){}

        FileInput(const FileInput&)=delete;

        FileInput(FileInput&& in)noexcept{
            file=in.file;
            in.file=nullptr;
            size_cache=in.size_cache;
            in.samepos_cache=0;
            got_size=in.got_size;
            in.got_size=false;
            prevpos=in.prevpos;
            in.prevpos=0;
            samepos_cache=in.samepos_cache;
            in.samepos_cache=0;
        }

        FileInput(const char* filename){
            open(filename);
        }


        ~FileInput(){
            close();
        }



        bool set_fp(FILE* fp){
            if(file){
                close();
            }
            file=fp;
            return true;
        }

        bool open(const char* filename){
            if(!filename)return false;
            FILE* tmp=nullptr;
            fopen_s(&tmp,filename,"rb");
            if(!tmp)return false;
            if(file){
                close();
            }
            file=tmp;
            return true;
        }

        bool close(){
            if(!file)return false;
            ::fclose(file);
            file=nullptr;
            size_cache=0;
            got_size=false;
            prevpos=0;
            samepos_cache=0;
            return true;
        }

        size_t size()const{
            if(!file)return 0;
            if(!got_size){
                auto fd=_fileno(file);
                COMMONLIB_FILEIO_STRUCT_STAT status;
                if(COMMONLIB_FILEIO_FUNC_FSTAT(fd,&status)==-1){
                    return 0;
                }
                size_cache=(size_t)status.st_size;
                got_size=true;
            }
            return size_cache;
        }

        char operator[](size_t p)const{
            if(!file)return 0;
            if(size_cache<=p)return 0;
            if(COMMONLIB_MSB_FLAG(size_t)&p)return 0;
            if(prevpos<p){
                if(COMMONLIB_FILEIO_FUNC_FSEEK(file,COMMONLIB_FILEIO_FSEEK_CAST(p-prevpos),
                    SEEK_CUR)!=0){
                    return 0;
                }
            }
            else if(prevpos-1==p){
                return samepos_cache;
            }
            else if(prevpos>p){
                if(COMMONLIB_FILEIO_FUNC_FSEEK(file,COMMONLIB_FILEIO_FSEEK_CAST(p),
                    SEEK_SET)!=0){
                    return 0;
                }
            }
            prevpos=p+1;
            auto c=fgetc(file);
            if(c==EOF){
                return 0;
            }
            samepos_cache=(char)c;
            return c;
        }
    };

    template<class Buf,class Mutex=std::mutex>
    struct ThreadSafe{
    private:
        Buf buf;
        mutable Mutex lock;
    public:

        ThreadSafe():buf(Buf()){}
        ThreadSafe(ThreadSafe&& in):buf(std::forward<Buf>(in.buf)){}
        ThreadSafe(const Buf&)=delete;
        ThreadSafe(Buf&& in):buf(std::forward<Buf>(in)){}
        

        Buf& get(){
            lock.lock();
            return buf;
        }

        bool release(Buf& ref){
            if(&buf!=&ref)return false;
            lock.unlock();
            return true;
        }

        auto operator[](size_t p)const
        ->decltype(buf[p]){
            std::scoped_lock<std::mutex> locker(lock);
            return buf[p];
        }

        auto operator[](size_t p)
        ->decltype(buf[p]){
            std::scoped_lock<std::mutex> locker(lock);
            return buf[p];
        }

        size_t size()const{
            std::scoped_lock<std::mutex> locker(lock);
            return buf.size();
        }

    };

    
    template<class Buf,class Mutex>
    struct ThreadSafe<Buf&,Mutex>{
    private:
        using INBuf=std::remove_reference_t<Buf>;
        INBuf& buf;
        mutable Mutex lock;
    public:
        ThreadSafe(ThreadSafe&& in):buf(in.buf){}
        ThreadSafe(INBuf& in):buf(in){}

        Buf& get(){
            lock.lock();
            return buf;
        }

        bool release(Buf& ref){
            if(&buf!=&ref)return false;
            lock.unlock();
            return true;
        }

        auto operator[](size_t p)const
        ->decltype(buf[p]){
            std::scoped_lock<std::mutex> locker(lock);
            return buf[p];
        }

        auto operator[](size_t p)
        ->decltype(buf[p]){
            std::scoped_lock<std::mutex> locker(lock);
            return buf[p];
        }

        size_t size()const{
            std::scoped_lock<std::mutex> locker(lock);
            return buf.size();
        }
    };
}