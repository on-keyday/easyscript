#pragma once
#include<reader.h>
#include<basic_helper.h>
#include<string>
#include<vector>
#include<assert.h>
#include"base.h"
#include"ast_obj.h"
#include"typeast.h"
namespace ast{

    template<class Buf=std::string,class TyPool=ast::type::TypePool>
    struct AstReader{
    private:
        PROJECT_NAME::Reader<Buf> r;
        std::vector<AstToken*> temp;
        TyPool& pool;
        type::TypeAstReader<AstReader,Buf> tr;


        AstToken* new_AstToken(){
            auto ret=ast::new_<AstToken>();
            temp.push_back(ret);
            return ret;
        }


        bool error(const char* str){
            throw str;
        }


        bool Call(AstToken*& tok){
            if(r.expect("(")){
                AstToken* tmp=new_AstToken();
                tmp->str="()";
                tmp->kind=AstKind::op_call;
                tmp->right=tok;
                tok=tmp;
                if(!r.expect(")")){
                    while(true){
                        tmp=nullptr;
                        Expr(tmp);
                        assert(tmp);
                        tok->param.push_back(tmp);
                        if(r.expect(","))continue;
                        if(!r.expect(")")){
                            return error("call:expected , or ) but not");
                        }
                        break;
                    }
                }
                return true;
            }
            return false;
        }

        bool Index(AstToken*& tok){
            if(r.expect("[")){
                AstToken* tmp=new_AstToken();
                tmp->str="[]";
                tmp->kind=AstKind::op_index;
                tmp->right=tok;
                tok=tmp;
                Expr(tok->left);
                if(!r.expect("]")){
                    return error("index:expected ] but not");
                }
                return true;
            }
            return false;
        }

        bool After(AstToken*& tok){
            assert(tok);
            bool f=false;
            while(Call(tok)||Index(tok)){
                assert(tok);
                f=true;
            }
            return f;
        }

        bool String(AstToken*& tok){
            if(r.achar()=='\"'||r.achar()=='\''){
                auto ret=new_AstToken();
                ret->kind=AstKind::string;
                if(!r.string(ret->str))return error("string:invalid string");
                tok=ret;
                return true;
            }
            return false;
        }

        bool Number(AstToken*& tok){
            if(PROJECT_NAME::is_digit(r.achar())){
                auto ret=new_AstToken();
                PROJECT_NAME::NumberContext<char> c;
                r.readwhile(ret->str,PROJECT_NAME::number,&c);
                if(c.failed)return error("number:invalid number");
                if(c.floatf){
                    ret->kind=AstKind::floatn;
                }
                else{
                    ret->kind=AstKind::intn;
                }
                tok=ret;
                return true;
            }
            return false;
        }

        bool BoolLiteral(AstToken*& tok){
            if(r.expect("true",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="true";
                tok->kind=AstKind::boolean;
                return true;
            }
            else if(r.expect("true",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="false";
                tok->kind=AstKind::boolean;
                return true;
            }
            return false;
        }

        bool NullLiteral(AstToken*& tok){
            if(r.expect("null",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="null";
                tok->kind=AstKind::null;
                return true;
            }
            return false;
        }

        bool Func(AstToken*& tok){
            if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::func_literal;
                if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    r.readwhile(tok->str,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
                }
                if(!r.expect("(")){
                    return error("func literal:expected ( but not");
                }
                tr.func_read(tok->type,true);
                if(!r.expect("{")){
                    return error("func literal:expected { but not");
                }
                Block(tok->block);
                return true;
            }
            return false;
        }

        bool Identifier(AstToken*& tok){
            if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                tok=new_AstToken();
                r.readwhile(tok->str,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
                tok->kind=AstKind::identifier;
                return true;
            }
            return false;
        }

        bool ScopedIdentifier(AstToken*& tok){
            if(Identifier(tok)){
                if(r.expect("::")){
                    auto n=new_AstToken();
                    tok->str="::";
                    n->left=tok;
                    tok=n;
                    if(!ScopedIdentifier(tok->right)){
                        return error("scope:expected identifier but not");
                    }
                }
                return true;
            }
            return false;
        }

        bool GlobalScopedIdentifier(AstToken*& tok){
            if(r.expect("::")){
                tok=new_AstToken();
                tok->kind=AstKind::op_scope;
                if(!ScopedIdentifier(tok->right)){
                    return error("scope:expected identifier but not");
                }
            }
            return false;
        }

        bool Primary(AstToken*& tok){
            return (String(tok)||Number(tok)||BoolLiteral(tok)||NullLiteral(tok)||Func(tok)||
                   ScopedIdentifier(tok)||GlobalScopedIdentifier(tok)||
                   error("primary:expect primary object but not"))&&
                   (After(tok)||true);
        }

        bool Increment(AstToken*& tok){
            if(r.expect("++")){
                tok=new_AstToken();
                tok->str="+=";
                tok->kind=AstKind::op_add_eq;
                if(!Unary(tok->left)){
                    return error("increment");
                }
                tok->right=new_AstToken();
                tok->right->kind=AstKind::intn;
                tok->right->str="1";
                return true;
            }
            return false;
        }

        bool Decrement(AstToken*& tok){
            if(r.expect("--")){
                tok=new_AstToken();
                tok->str="-=";
                tok->kind=AstKind::op_sub_eq;
                if(!Unary(tok->left)){
                    return error("decrement");
                }
                tok->right=new_AstToken();
                tok->right->kind=AstKind::intn;
                tok->right->str="1";
                return true;
            }
            return false;
        }

        bool BitNot(AstToken*& tok){
            if(r.expect("~")){
                tok=new_AstToken();
                tok->kind=AstKind::op_bitnot;
                tok->str="~";
                if(!Unary(tok->right)){
                    return error("bitnot");
                }
                return true;
            }
            return false;
        }

        bool LogicalNot(AstToken*& tok){
            if(r.expect("!")){
                tok=new_AstToken();
                tok->kind=AstKind::op_logicalnot;
                tok->str="!";
                if(!Unary(tok->right)){
                    return error("logicalnot");
                }
                return true;
            }
            return false;
        }

        bool SinglePlus(AstToken*& tok){
            if(r.expect("+")){
                return Unary(tok);
            }
            return false;
        }

        bool SingleMinus(AstToken*& tok){
            if(r.expect("-")){
                tok=new_AstToken();
                tok->kind=AstKind::op_sub;
                tok->str="-";
                if(!Unary(tok->right)){
                    return error("single minus");
                }
                tok->left=new_AstToken();
                tok->left->kind=AstKind::intn;
                tok->left->str="0";
                return true;
            }
            return false;
        }

        bool Address(AstToken*& tok){
            if(r.expect("&")){
                tok=new_AstToken();
                tok->kind=AstKind::op_addr;
                tok->str="&";
                if(!Unary(tok->right)){
                    return error("address");
                }
                return true;
            }
            return false;
        }

        bool Dereference(AstToken*& tok){
            if(r.expect("*")){
                tok=new_AstToken();
                tok->kind=AstKind::op_deref;
                tok->str="*";
                if(!Unary(tok->right)){
                    return error("dereference");
                }
                return true;
            }
            return false;
        }

        bool Unary(AstToken*& tok){
            return Increment(tok)||Decrement(tok)||SinglePlus(tok)||BitNot(tok)||LogicalNot(tok)||
                   SingleMinus(tok)||Address(tok)||Dereference(tok)||Primary(tok);
        }

        template<class Func>
        inline bool Bin(AstKind kind,const char* str,const char* err,Func func,AstToken*& tok){
            assert(tok);
            if(r.expect(str)){
                auto n=new_AstToken();
                n->str=str;
                n->kind=kind;
                n->left=tok;
                tok=n;
                if(!func(tok->right)){
                    return error(err);
                }
                return true;
            }
            return false;
        }

        template<class Func,class Not>
        inline bool NEqBin(AstKind kind,const char* str,const char* err,Func func,Not no,AstToken*& tok){
            assert(tok);
            if(r.expect(str,no)){
                auto n=new_AstToken();
                n->str=str;
                n->kind=kind;
                n->left=tok;
                tok=n;
                if(!func(tok->right)){
                    return error(err);
                }
                return true;
            }
            return false;
        }

        static bool not_eq_s(char c){
            return c=='=';
        }
#define MEMBER_CAPTURE(f) [this](AstToken*& tok){return f(tok);}

        bool Mul(AstToken*& tok){
            return NEqBin(AstKind::op_mul,"*","mul",MEMBER_CAPTURE(Unary),not_eq_s,tok);
        }

        bool Div(AstToken*& tok){
            return NEqBin(AstKind::op_div,"/","div",MEMBER_CAPTURE(Unary),not_eq_s,tok);
        }

        bool Mod(AstToken*& tok){
            return NEqBin(AstKind::op_mod,"%","mod",MEMBER_CAPTURE(Unary),not_eq_s,tok);
        }

        bool Bin_Mul(AstToken*& tok){
            if(Unary(tok)){
                while(Mul(tok)||Div(tok)||Mod(tok));
                return true;
            }
            return false;
        }

        bool Add(AstToken*& tok){
            return NEqBin(AstKind::op_add,"+","add",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool Sub(AstToken*& tok){
            return NEqBin(AstKind::op_sub,"-","sub",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool ShiftLeft(AstToken*& tok){
            return NEqBin(AstKind::op_shl,"<<","shift left",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool ShiftRight(AstToken*& tok){
            return NEqBin(AstKind::op_shr,">>","shift right",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool BitOr(AstToken*& tok){
            return NEqBin(AstKind::op_bitor,"|","bit or",MEMBER_CAPTURE(Bin_Mul),[](char c){return c=='='||c=='|';},tok);
        }

        bool BitAnd(AstToken*& tok){
            return NEqBin(AstKind::op_bitand,"&","bit and",MEMBER_CAPTURE(Bin_Mul),[](char c){return c=='='||c=='&';},tok);
        }

        bool BitXor(AstToken*& tok){
            return NEqBin(AstKind::op_bitxor,"^","bit xor",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool Bin_Add(AstToken*& tok){
            if(Bin_Mul(tok)){
                while(Add(tok)||Sub(tok)||ShiftLeft(tok)||ShiftRight(tok)||BitOr(tok)||
                      BitAnd(tok)||BitXor(tok));
                return true;
            }
            return false;
        }

        bool Equal(AstToken*& tok){
            return Bin(AstKind::op_equ,"==","equal",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool NotEqual(AstToken*& tok){
            return Bin(AstKind::op_neq,"!=","not equal",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftGreaterRight(AstToken*& tok){
            return Bin(AstKind::op_lgr,">","left greater right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftLesserRight(AstToken*& tok){
            return Bin(AstKind::op_llr,"<","left lesser right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftEqualLesserRight(AstToken*& tok){
            return Bin(AstKind::op_lelr,"<=","left equal lesser right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftEqualGreaterRight(AstToken*& tok){
            return Bin(AstKind::op_legr,">=","left equal greater right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool Bin_Compare(AstToken*& tok){
            if(Bin_Add(tok)){
                while(Equal(tok)||NotEqual(tok)||LeftEqualGreaterRight(tok)||LeftEqualLesserRight(tok)||
                      LeftGreaterRight(tok)||LeftLesserRight(tok));
                return true;
            }
            return false;
        }

        bool Bin_LogicalAnd(AstToken*& tok){
            if(Bin_Compare(tok)){
                while(Bin(AstKind::op_logicaland,"&&","logical and",MEMBER_CAPTURE(Bin_Compare),tok));
                return true;
            }
            return false;
        }

        bool Bin_LogicalOr(AstToken*& tok){
            if(Bin_LogicalAnd(tok)){
                while(Bin(AstKind::op_logicalor,"||","logical or",MEMBER_CAPTURE(Bin_LogicalAnd),tok));
                return true;
            }
            return false;
        }

        bool Assign(AstToken*& tok){
            return Bin(AstKind::op_assign,"=","assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool AddAssign(AstToken*& tok){
            return Bin(AstKind::op_add_eq,"+=","add assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool SubAssign(AstToken*& tok){
            return Bin(AstKind::op_sub_eq,"-=","sub assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool MulAssign(AstToken*& tok){
            return Bin(AstKind::op_mul_eq,"*=","mul assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool DivAssign(AstToken*& tok){
            return Bin(AstKind::op_div_eq,"/=","div assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool ModAssign(AstToken*& tok){
            return Bin(AstKind::op_mod_eq,"%=","mod assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool ShiftLeftAssign(AstToken*& tok){
            return Bin(AstKind::op_shl_eq,"<<=","shift left assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool ShiftRightAssign(AstToken*& tok){
            return Bin(AstKind::op_shr_eq,">>=","shift right assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool BitOrAssign(AstToken*& tok){
            return Bin(AstKind::op_bitor_eq,"|=","bit or assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool BitAndAssign(AstToken*& tok){
            return Bin(AstKind::op_bitand_eq,"&=","bit and assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool BitXorAssign(AstToken*& tok){
            return Bin(AstKind::op_bitxor_eq,"|=","bit xor assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool Bin_Assign(AstToken*& tok){
            if(Bin_LogicalOr(tok)){
                while(Assign(tok)||AddAssign(tok)||SubAssign(tok)||MulAssign(tok)||DivAssign(tok)||ModAssign(tok)||
                      ShiftRightAssign(tok)||ShiftLeftAssign(tok)||BitOrAssign(tok)||BitAndAssign(tok)||BitXorAssign(tok));
                return true;
            }
            return false;
        }

        bool Cond(AstToken*& tok){
            if(Bin_Assign(tok)){
                assert(tok);
                if(r.expect("?")){
                    auto n=new_AstToken();
                    n->str="?:";
                    n->kind=AstKind::op_cond;
                    n->cond=tok;
                    if(!Cond(tok->left)){
                        return error("cond");
                    }
                    if(!r.expect(":")){
                        return error("cond:expected : but not");
                    }
                    if(!Cond(tok->right)){
                        return error("cond");
                    }
                }
                return true;
            }
            return false;
        }

        bool Init(AstToken*& tok){
            if(Cond(tok)){
                Bin(AstKind::op_init,":=","init",MEMBER_CAPTURE(Cond),tok);
                return true;
            }
            return false;
        }

        bool Expr(AstToken*& tok){
            return Cond(tok);
        }

        bool ExprStmt(AstToken*& tok){
            if(Init(tok)){
                if(!r.expect(";")){
                    return error("expr statement:expected ; but not");
                }
                return true;
            }
            return false;
        }

        bool IfStmt(AstToken*& tok){
            if(r.expect("if",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::if_stmt;
                Init(tok->cond);
                if(r.expect(";")){
                    tok->left=tok->cond;
                    tok->cond=nullptr;
                    Expr(tok->cond);
                }
                if(r.expect("{")){
                    Block(tok->block);
                }
                else if(!r.expect(";")){
                    return error("if statement:expected ; or { but not");
                }
                if(r.expect("else",PROJECT_NAME::is_c_id_usable)){
                    if(r.ahead("if",PROJECT_NAME::is_c_id_usable)){
                        auto f=IfStmt(tok->right);
                        assert(f);
                    }
                    else if(r.ahead("{")){
                        auto f=BlockStmt(tok->right);
                        assert(f);
                    }
                    else if(!r.expect(";")){
                        return error("else statement:expected if or { or ; but not");
                    }
                }
                return true;
            }
            return false;
        }

        bool ForStmt(AstToken*& tok){
            if(r.expect("for",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::for_stmt;
                if(r.expect("{")){
                    Block(tok->block);
                    return true;
                }
                Init(tok->cond);
                if(r.expect("{")){
                    Block(tok->block);
                    return true;
                }
                else if(!r.expect(";")){
                    return error("for statement:expected ; or { but not");
                }
                tok->left=tok->cond;
                tok->cond=nullptr;
                if(r.expect(";")){

                }
                else{
                    Expr(tok->cond);
                    if(!r.expect(";")){
                        return error("for statement:expected ; but not");
                    }
                }
                if(r.expect("{")){
                    Block(tok->block);
                    return true;
                }
                Expr(tok->right);
                if(!r.expect("{")){
                    return error("for statement:expected { but not");
                }
                Block(tok->block);
                return true;
            }
            return false;
        }

        bool Block(AstList<AstToken>& tok){
            while(!r.expect("}")){
                if(r.eof()){
                    return error("block:unexpected EOF");
                }
                AstToken* stmt=nullptr;
                Stmt(stmt);
                tok.push_back(stmt);
            }
            return true;
        }

        bool BlockStmt(AstToken*& tok){
            if(r.expect("{")){
                tok=new_AstToken();
                tok->kind=AstKind::block_stmt;
                Block(tok->block);
                return true;
            }
            return false;
        }

        bool VarStmt(AstToken*& tok){
            if(r.expect("var",PROJECT_NAME::is_c_id_usable)){
                
            }
            return false;
        }

        bool FuncStmt(AstToken*& tok){
            if(Func(tok)){
                if(After(tok)){
                    if(!r.expect(";")){
                        return error("function statement:expected ; but not");
                    }
                }
                return true;
            }
            return false;
        }

        bool ReturnStmt(AstToken*& tok){
            if(r.expect("return",PROJECT_NAME::is_c_id_usable)){

            }
        }

        bool Stmt(AstToken*& tok){
            return IfStmt(tok)||ForStmt(tok)||BlockStmt(tok)||FuncStmt(tok)||
            ExprStmt(tok)||r.expect(";")||error("statement:not match for any statement");
        }

        bool Stmts(AstList<AstToken>& tok){
            while(!r.eof()){
                AstToken* stmt=nullptr;
                Stmt(stmt);
                tok.push_back(stmt);
            }
            return true;
        }

        bool Program(AstToken*& tok){
            tok=new_AstToken();
            tok->kind=AstKind::program;
            Stmts(tok->block);
            return true;
        }
    public:
        AstReader(TyPool& inp):tr(*this,r,inp),pool(inp){}

        AstReader(Buf&& buf,TyPool& inp):r(std::forward<Buf>(buf)),tr(*this,r,inp),pool(inp){}

        bool errorp(PROJECT_NAME::Reader<Buf>* p,const char* err){
            if(&r==p){
                return error(err);
            }
            return false;
        }

        Buf& bufctrl(){
            return r.ref();
        }

        bool parse(AstToken*& tok,const char** err=nullptr){
            r.set_ignore(PROJECT_NAME::ignore_c_comments);
            bool ret=true;
            auto tmpdel=[this]{
                for(auto p:temp){
                    delete_(p);
                }
                pool.delall();
            };
            try{
                Program(tok);
            }
            catch(const char* e){
                if(err){
                    *err=e;
                }
                tmpdel();
            }
            catch(...){
                if(err){
                    *err="system error:compiler broken";
                }
                tmpdel();
            }
            temp.resize(0);
            return ret;
        }

        bool expr(AstToken*& tok){
            return Expr(tok);
        } 
    };
#undef MEMBER_CAPTURE
}
