/*license*/
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
    if(res!=0){
        LinePosContext ctx;
        self->reader.readwhile(linepos,ctx);
        self->result.first+="("+std::to_string(ctx.line+1)+","+std::to_string(ctx.pos+1)+")";
        if(flag&0x01){
            self->reader.seek(beginpos);
        }
    }
    return res;
}

const char* STDCALL get_result(Script* self){
    if(!self)return nullptr;
    return self->result.first.c_str();
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
            /*ref.erase(0,1);
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
                    else if(ref[i]=='a'){
                        tmp+="\a";
                    }
                    else if(ref[i]=='b'){
                        tmp+="\b";
                    }
                    else if(ref[i]=='v'){
                        tmp+="\v";
                    }
                    else if(ref[i]=='f'){
                        tmp+="\f";
                    }
                    /*else if(ref[i]=='e'){
                        tmp+="\e";
                    }*
                    else{
                        tmp+=ref[i];
                    }
                }
                else{
                    tmp+=ref[i];
                }
            }
            ref=tmp;
        }*/
            if(!interpreter::deescape_str(ref))return nullptr;
        }
        return ref.c_str();
    }
    else{
        return arg->args[i].first.c_str();
    }
}

size_t STDCALL get_arg_len(ArgContext* arg){
    return !arg?0:arg->args.size();
}

int STDCALL set_result_error(ArgContext* arg,const char* str){
    if(!arg||!str)return 0;
    arg->result.first=str;
    arg->result.second=EvalType::error;
    return 1;
}

int STDCALL set_result_bool(ArgContext* arg,int flag){
    if(!arg)return 0;
    arg->result.first=flag?"true":"false";
    arg->result.second=EvalType::boolean;
    return 1;
}

int STDCALL set_result_int(ArgContext* arg,long long num){
    if(!arg)return 0;
    arg->result.first=std::to_string(num);
    arg->result.second=EvalType::integer;
    return 1;
}

int STDCALL set_result_float(ArgContext* arg,double num){
    if(!arg)return 0;
    arg->result.first=std::to_string(num);
    arg->result.second=EvalType::floats;
    return 1;
}

int STDCALL set_result_str_len(ArgContext* arg,const char* str,size_t len){
    if(!arg||!str)return 0;
    arg->result.first.assign(str,len);
    interpreter::enescape_str(arg->result.first);
    arg->result.second=EvalType::str;
    return 1;
}

int STDCALL set_result_str(ArgContext* arg,const char* str){
    if(!arg||!str)return 0;
    return set_result_str_len(arg,str,strlen(str));
}

int STDCALL set_result_none(ArgContext* arg){
    if(!arg)return 0;
    arg->result.first="none";
    arg->result.second=EvalType::none;
    return true;
}

size_t STDCALL get_instance_id(ArgContext* arg){
    if(!arg)return ~0;
    return arg->instance->id;
}