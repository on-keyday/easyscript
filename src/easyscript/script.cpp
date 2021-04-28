/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#define DLL_EXPORT __declspec(dllexport)
#include"interpret.h"
#include"script.h"
#include<cstdio>


using namespace PROJECT_NAME;
struct Script
{
    PROJECT_NAME::Reader<std::string> reader;
    ValType result;
    interpreter::RegisterMap base;
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

int STDCALL add_builtin_object(Script* self,const char* objname,ObjProxy proxy,void* ctx){
    if(!self||!objname||!proxy||!ctx){
        return 0;
    }
    PROJECT_NAME::Reader<std::string> check(objname);
    if(!is_c_id_top_usable(check.achar()))return 0;
    check.readwhile(untilincondition_nobuf,is_c_id_usable<char>);
    if(!check.ceof())return 0;
    self->base[objname]={proxy,ctx};
    return 1;
}

int STDCALL execute(Script* self,unsigned char flag){
    if(!self)return 0;
    self->reader.set_ignore(ignore_c_comments);
    auto beginpos=self->reader.readpos();
    auto res=interpreter::interpret(self->reader,&self->result,self->base);
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

const char* STDCALL get_arg_index(ArgContext* arg,size_t i){
    if(!arg)return nullptr;
    if(arg->args.size()<i)return nullptr;
    if(arg->args[i].second==EvalType::str){
        auto& ref=arg->args[i].first;
        if(ref[0]=='\"'||ref[0]=='\''){
            ref.erase(0,1);
            ref.pop_back();
            std::string tmp;
            for(int i=0;i<ref.size();i++){
                if(ref[i]=='\\'){
                    i++;
                    if(ref[i]=='n'){
                        tmp+="\n";
                    }
                    else if(ref[i]=='r'){ 
                        tmp+="\r";
                    }
                    else if(ref[i]=='t'){
                        tmp+="\t";
                    }
                    else{
                        tmp+=ref[i];
                    }
                }
                else{
                    tmp+=ref[i];
                }
            }
            ref=tmp;
        }
        return ref.c_str();
    }
    else{
        return arg->args[i].first.c_str();
    }
}