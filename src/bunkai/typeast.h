#pragma once
#include<reader.h>
#include<string>
#include"base.h"
#include"ast_obj.h"
#include<assert.h>
#include<map>
#include<vector>
#include<basic_helper.h>
namespace ast{
    namespace type{
#define MEMBER_CAPTURE(f) [this](auto... in)->ast::type::Type*{return f(in...);}
        template<class SyntaxReader,class Buf>
        struct TypeAstReader{
        private:
            PROJECT_NAME::Reader<Buf>& r;
            SyntaxReader& s;
            TypePool& pool;


            Object* new_Object(){
                return pool.new_Object();
            }

            bool error(const char* err){
                return s.errorp(&r,err);
            }

            template<class Func,class... Args>
            inline bool TypeGet(Type*& t,Func f,const char* err,Args... args){
                TypeAstReader::Type(t);
                t=f(t,args...);
                return true;
            }

            bool Pointer(Type*& t){
                if(r.expect("*")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.pointer_to),"pointer");
                }
                return false;
            }

            bool Reference(Type*& t){
                if(r.expect("&")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.reference_to),"reference");
                }
                return false;
            }

            bool Temporary(Type*& t){
                if(r.expect("&&")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.temporary_of),"temporary");
                }
                return false;   
            }

            bool Array(Type*& t){
                if(r.expect("[")){
                    if(r.expect("]")){
                        return TypeGet(t,MEMBER_CAPTURE(pool.vector_of),"vector");
                    }
                    AstToken* tok;
                    s.expr(tok);
                    if(!r.expect("]")){
                        return error("array:expected ] but not");
                    }
                    return TypeGet(t,MEMBER_CAPTURE(pool.array_of),"array",tok);
                }
                return false;
            }

            bool ParseParameter(AstList<Object>& params,bool generic){
                if(!r.expect(")")){
                    while(true){
                        Object* param=new_Object();
                        if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                            do{
                                auto bpos=r.readpos();
                                auto tmp=r.set_ignore(nullptr);
                                std::string name;
                                r.readwhile(name,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
                                if(r.expect("::")){
                                    r.seek(bpos);
                                    r.set_ignore(tmp);
                                    break;
                                }
                                r.set_ignore(tmp);
                                if(r.ahead(",")||r.ahead(")")){
                                    if(!generic){
                                        r.seek(bpos);
                                        break;
                                    }
                                    param->type=pool.generic();
                                }
                                param->name=std::move(name);
                            }while(0);
                        }
                        if(!param->type){
                            read_type(param->type);
                        }
                        if(param->name!=""&&r.expect("=")){
                            s.expr(param->init);
                        }
                        params.push_back(param);
                        if(r.expect(","))continue;
                        if(!r.expect(")")){
                            return error("param:expected ) but not");
                        }
                        break;
                    }
                }
                return true;
            }

            bool FuncParse(Type*& t,bool gen){
                AstList<Object> params;
                ParseParameter(params,gen);
                if(r.expect("->")){
                    read_type(t);
                }
                t=pool.function(t,params);
                return true;
            }

            bool Function(Type*& t){
                if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
                    if(!r.expect("(")){
                        return error("function:expected ( but not");
                    }
                    FuncParse(t,true);
                    return true;
                }
                return false;
            }

            bool ParseStructs(AstList<Object>& params){
                while(!r.expect("}")){
                    Object* param=new_Object();
                    if(r.expect("_",PROJECT_NAME::is_c_id_usable)){}
                    else if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                        r.readwhile(param->name,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
                        assert(param->name!="");
                    }
                    else{
                        return error("struct:expected identifier but not");
                    }
                    read_type(param->type);
                    if(param->name!=""&&r.expect("=")){
                        s.expr(param->init);
                    }
                    if(!r.expect(";")){
                        return error("struct:expected ; but not");
                    }
                }
                return true;
            }

            bool StructParse(Type*& t){
                t=pool.struct_();
                ParseStructs(t->param);
                return true;
            }

            bool Struct(Type*& t){
                if(r.expect("struct",PROJECT_NAME::is_c_id_usable)){
                    if(!r.expect("{")){
                        return error("struct:expected { but not");
                    }
                    return StructParse(t);
                }
                return false;
            }

            bool Type(Type*& t){
                return Pointer(t)||Array(t)||Function(t)||Struct(t);
            }

        public:
            bool read_type(ast::type::Type*& t){
                return Temporary(t)||Reference(t)||Type(t);
            }

            bool func_read(ast::type::Type*& t,bool gen){
                return FuncParse(t,gen);
            }
            TypeAstReader(SyntaxReader& sr,PROJECT_NAME::Reader<Buf>& rr,TypePool& ip):s(sr),r(rr),pool(ip){}

        };
#undef MEMBER_CAPTURE
    }
}