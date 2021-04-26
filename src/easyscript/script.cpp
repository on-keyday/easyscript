#define DLL_EXPORT __declspec(dllexport)
#include"interpret.h"
#include"script.h"
#include<cstdio>

struct Script
{
    PROJECT_NAME::Reader<std::string> reader;
    ValType result;
};


Script* STDCALL make_script(){
    return make<Script>();
}

int STDCALL add_sourece(Script* self,const char* str,size_t len){
    if(!self||!str||!len)return 0;
    self->reader.ref().append(str,len);
    return 1;
}

int STDCALL add_sourece_from_file(Script* self,const char* filename){
    if(!self||!filename)return 0;
#if !_WIN32||defined __GNUC__
#define fopen_s(pfp,filename,mode) (*pfp=fopen(filename,mode))
#endif
    FILE* fp=nullptr;
    fopen_s(&fp,filename,"r");
    if(!fp)return 0;
    while(true){
        auto c=fgetc(fp);
        if(c==EOF)break;
        self->reader.ref()+=c;
    }
    fclose(fp);
    return 1;
}

int STDCALL execute(Script* self,unsigned char flag){
    if(!self)return 0;
    self->reader.set_ignore(PROJECT_NAME::ignore_c_comments);
    auto beginpos=self->reader.readpos();
    auto res=interpreter::interpret(self->reader,&self->result);
    if(res!=0&&flag&0x01){
        self->reader.seek(beginpos);
    }
    return res;
}

int STDCALL delete_script(Script* self){
    if(!self)return 0;
    delete self;
    return 1;
}