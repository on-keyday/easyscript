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
#define MEMBER_CAPTURE(f) [this](auto... in)->ast::type::SType{return f(in...);}
        template<class SyntaxReader,class Buf>
        struct TypeAstReader{
        private:
            PROJECT_NAME::Reader<Buf>& r;
            SyntaxReader& s;
            TypePool& pool;


            SObject new_Object(){
                return pool.new_Object();
            }

            bool error(const char* err){
                return s.errorp(&r,err);
            }

            template<class Str>
            bool idread(Str& in){
                return s.idreadp(&r,in);
            }

            bool expr(SAstToken& tok){
                return s.exprp(&r,tok);
            }

            template<class Func,class... Args>
            inline bool TypeGet(SType& t,Func f,const char* err,Args... args){
                if(r.ahead(",")||r.ahead(")")){
                    t=pool.generic();
                }
                else{
                    TypeAstReader::Type(t);
                }
                t=f(t,args...);
                return true;
            }

            bool Pointer(SType& t){
                if(r.expect("*")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.pointer_to),"pointer");
                }
                return false;
            }

            bool Reference(SType& t){
                if(r.expect("&")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.reference_to),"reference");
                }
                return false;
            }

            bool Temporary(SType& t){
                if(r.expect("&&")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.temporary_of),"temporary");
                }
                return false;   
            }


            bool Array(SType& t){
                if(r.expect("[")){
                    if(r.expect("]")){
                        return TypeGet(t,MEMBER_CAPTURE(pool.vector_of),"vector");
                    }
                    SAstToken tok;
                    expr(tok);
                    if(!r.expect("]")){
                        return error("array:expected ] but not");
                    }
                    return TypeGet(t,MEMBER_CAPTURE(pool.array_of),"array",tok);
                }
                return false;
            }

            bool ParseParameter(AstList<Object>& params,bool generic,bool& genty){
                if(!r.expect(")")){
                    while(true){
                        SObject param=new_Object();
                        if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                            do{
                                auto bpos=r.readpos();
                                auto tmp=r.set_ignore(nullptr);
                                std::string name;
                                idread(name);
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
                        assert(param->type);
                        if(param->type->generic_relative){
                            genty=true;
                        }
                        if(r.expect("=")){
                            expr(param->init);
                        }
                        params.push_back(std::move(param));
                        if(r.expect(","))continue;
                        if(!r.expect(")")){
                            return error("param:expected ) or , but not");
                        }
                        break;
                    }
                }
                return true;
            }

            bool FuncParse(SType& t,bool gen){
                if(!r.expect("(")){
                    return error("function:expected ( but not");
                }
                AstList<Object> params;
                bool genty=false;
                ParseParameter(params,gen,genty);
                if(r.expect("->")){
                    read_type(t);
                    assert(t);
                    genty=genty||t->generic_relative;
                }
                t=pool.function(t,params);
                t->generic_relative=genty;
                return true;
            }

            bool Function(SType& t){
                if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
                    FuncParse(t,true);
                    return true;
                }
                return false;
            }

            bool ParseStructs(AstList<Object>& params){
                while(!r.expect("}")){
                    SObject param=new_Object();
                    if(r.expect("_",PROJECT_NAME::is_c_id_usable)){}
                    else if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                        idread(param->name);    
                    }
                    else{
                        return error("struct:expected identifier but not");
                    }
                    read_type(param->type);
                    if(r.expect("=")){
                        expr(param->init);
                    }
                    /*if(!r.expect(";")){
                        return error("struct:expected ; but not");
                    }*/
                    params.push_back(std::move(param));
                }
                return true;
            }

            bool StructParse(SType& t){
                t=pool.struct_();
                ParseStructs(t->param);
                return true;
            }

            bool Struct(SType& t){
                if(r.expect("struct",PROJECT_NAME::is_c_id_usable)){
                    if(!r.expect("{")){
                        return error("struct:expected { but not");
                    }
                    return StructParse(t);
                }
                return false;
            }

            bool KeyWord(SType& t){
                const char* expected=nullptr;
                auto e=[&,this](auto c){return r.expectp(c,expected,PROJECT_NAME::is_c_id_usable);};
                if(e("float")||e("double")||e("int")||e("uint")||e("bool")||
                   e("int8")||e("int16")||e("int32")||e("int64")||
                   e("uint8")||e("uint16")||e("uint32")||e("uint64"))
                {
                    t=pool.keyword(expected);
                    return true;
                }
                else if(e("const")){
                    return TypeGet(t,MEMBER_CAPTURE(pool.const_of),"const");
                }
                else if(e("typeof")){
                    t=pool.typeofexpr();
                    if(!r.expect("(")){
                        return error("typeof:expected ( but not");
                    }
                    expr(t->token);
                    if(!r.expect(")")){
                        return error("typeof:expected ) but not");
                    }
                    return true;
                }
                return false;
            }

            bool Identifier(SType& t){
                Scope* search=nullptr;
                auto idf=[&,this]{
                    std::string id;
                    while(true){
                        idread(id);
                        if(r.expect("::")){
                            if(!PROJECT_NAME::is_c_id_top_usable(r.achar())){
                                return error("type:identifier:expected identifier but not");
                            }
                            search=pool.get_scope(search,id.c_str());
                            if(!search){
                                return error("type:identifier:undecleared namespace");
                            }
                            id="";
                            continue;
                        }
                        break;
                    }
                    if(!search){
                        search=pool.get_scope(nullptr,nullptr);
                    }
                    t=pool.identifier(search,id.c_str());
                    return true;
                };
                if(r.expect("::")){
                    search=pool.get_scope(nullptr,"::");
                    return idf();
                }
                else if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    return idf();
                }
                return false;
            }

            bool Type(SType& t){
                return Pointer(t)||Array(t)||Function(t)||Struct(t)||KeyWord(t)||Identifier(t);
            }

        public:
            bool read_type(ast::type::SType& t){
                return Temporary(t)||Reference(t)||Type(t);
            }

            bool func_read(ast::type::SType& t,bool gen){
                return FuncParse(t,gen);
            }
            TypeAstReader(SyntaxReader& sr,PROJECT_NAME::Reader<Buf>& rr,TypePool& ip):s(sr),r(rr),pool(ip){}

        };
#undef MEMBER_CAPTURE
    }
}