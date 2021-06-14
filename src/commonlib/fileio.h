#pragma once
#include<project_name.h>
#define POSIX_SOURCE 200809L
#include<stdio.h>
#include<sys/stat.h>
#include<stddef.h>
#include<mutex>

#ifdef _WIN32
#include<Windows.h>
#elif defined(__linux__)
#include<sys/mman.h>
#include <fcntl.h>
#include<unistd.h>
#else
    #error "unsurpported platform"
#endif

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
#define fopen_s(pfp,filename,mode) ((*pfp)=fopen(filename,mode))
#define COMMONLIB_FILEIO_STRUCT_STAT struct stat
#define COMMONLIB_FILEIO_FUNC_FSTAT ::fstat
#define COMMONLIB_FILEIO_FUNC_FSEEK ::fseek
#define COMMONLIB_FILEIO_FSEEK_CAST(x) (long)(x)
#define _fileno fileno
#endif
        

namespace PROJECT_NAME{
    inline bool getfilesizebystat(int fd,size_t& size){
        COMMONLIB_FILEIO_STRUCT_STAT status;
        if(COMMONLIB_FILEIO_FUNC_FSTAT(fd,&status)==-1){
            return false;
        }
        size=(size_t)status.st_size;
        return true;
    } 

    struct FileInput{
    private:
        ::FILE* file=nullptr;
        size_t size_cache=0;
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
            //got_size=in.got_size;
            //in.got_size=false;
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
            auto fd=_fileno(tmp);
            if(getfilesizebystat(fd,size_cache)){
                fclose(tmp);
                return 0;
            }
            close();
            file=tmp;
            return true;
        }

        bool close(){
            if(!file)return false;
            ::fclose(file);
            file=nullptr;
            size_cache=0;
            //got_size=false;
            prevpos=0;
            samepos_cache=0;
            return true;
        }

        size_t size()const{
            if(!file)return 0;
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


    struct FileMap{
    private:
    #if defined(_WIN32)
        HANDLE file=INVALID_HANDLE_VALUE;
        HANDLE maph=nullptr;

        bool open_detail(const char* name){
            HANDLE tmp=CreateFileA(
                name,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

            if(tmp==INVALID_HANDLE_VALUE){
                return false;
            }

            DWORD high=0;
            DWORD low=GetFileSize(tmp,&high);
            uint64_t tmpsize=((uint64_t)high<<32)+low;

            HANDLE tmpm=CreateFileMappingA(tmp,NULL,PAGE_READONLY,0,0,NULL);

            if(tmpm==NULL){
                CloseHandle(tmp);
                return false;
            }
            char* rep=(char*)MapViewOfFile(tmpm,FILE_MAP_READ,0,0,0);
            if(!rep){
                CloseHandle(tmpm);
                CloseHandle(tmp);
                return false;
            }
            close_detail();
            file=tmp;
            maph=tmpm;
            place=rep;
            _size=(size_t)tmpsize;
            return true;
        }

        bool close_detail(){
            if(file==INVALID_HANDLE_VALUE)return false;
            UnmapViewOfFile((LPCVOID)place);
            CloseHandle(maph);
            CloseHandle(file);
            file=INVALID_HANDLE_VALUE;
            maph=nullptr;
            place=nullptr;
            _size=0;
            return true;
        }

        bool move_detail(FileMap&& in){
            file=in.file;
            in.file=nullptr;
            maph=in.maph;
            in.maph=nullptr;
            return true;
        }
    #elif defined(__linux__)
        int fd=-1;
        long maplen=0;
        bool open_detail(const char* in){
            int tmpfd=::open(in,O_RDONLY);
            if(tmpfd==-1){
                return false;
            }
            size_t tmpsize=0;
            if(!getfilesizebystat(tmpfd,tmpsize)){
                ::close(fd);
                return false;
            }
            long pagesize=::getpagesize(),mapsize=0;
            mapsize=(tmpsize/pagesize+1)*pagesize;
            char* tmpmap=(char*)mmap(nullptr,mapsize,PROT_READ,MAP_SHARED,tmpfd,0);
            if(tmpmap==MAP_FAILED){
                ::close(tmpfd);
                return false;
            }
            close_detail();
            fd=tmpfd;
            _size=tmpsize;
            maplen=mapsize;
            place=tmpmap;
            return false;
        }

        bool close_detail(){
            if(fd==-1)return false;
            munmap(place,maplen);
            ::close(fd);
            fd=-1;
            maplen=0;
            place=nullptr;
            _size=0;
            return true;
        }

        bool move_detail(FileMap&& in){
            fd=in.fd;
            in.fd=-1;
            return true;
        }
    #endif
        char* place=nullptr;
        size_t _size=0;

        bool move_common(FileMap&& in){
            move_detail(std::forward<FileMap>(in));
            place=in.place;
            in.place=nullptr;
            _size=in._size;
            in._size=0;
            return true;
        }
    public:
        FileMap(const char* name){
            open(name);
        }

        ~FileMap(){
            close();
        }

        FileMap(FileMap&& in){
            move_common(std::forward<FileMap>(in));
        }

        bool open(const char* name){
            if(!name)return false;
            return open_detail(name);
        }

        bool close(){
            return close_detail();
        }

        size_t size() const{
            return _size;
        }

        char operator[](size_t pos)const{
            if(!place||_size<=pos)return char();
            return place[pos];
        }

    };
#ifdef fileno
#undef fileno
#endif
}