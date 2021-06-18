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

    template<class Buf=std::string>
    struct AstReader{
        using TyPool=ast::type::TypePool;
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

        template<class Str>
        bool KeyWordCheck(Str& str){
            auto e=[&](auto e){return str==e;};
            if(e("func")||e("var")||e("decl")||
               e("break")||e("continue")||e("if")||e("for")||e("return")||e("switch")||e("select")||
               e("namespace")||e("struct")||e("union")||e("interface")||e("enum")||
               e("const")||e("align")||e("atomic")||e("thread_local")||e("local")||e("global")||
               e("float")||e("double")||e("int")||e("uint")||e("bool")||e("void")||
               e("int8")||e("int16")||e("int32")||e("int64")||
               e("uint8")||e("uint16")||e("uint32")||e("uint64")||
               e("null")||e("true")||e("false")||e("cast")
            ){
                return error("identifier:keyword is not usable for identifier");
            }
            return true;
        }

        template<class Str>
        bool IdRead(Str& str){
            r.readwhile(str,PROJECT_NAME::untilincondition,PROJECT_NAME::is_c_id_usable<char>);
            KeyWordCheck(str);
            return true;
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

        bool AfterIncrement(AstToken*& tok){
            if(r.expect("++")){
                AstToken* tmp=new_AstToken();
                tmp->str="++";
                tmp->kind=AstKind::op_add_eq;
                tmp->left=tok;
                tok=tmp;
                tok->right=new_AstToken();
                tok->right->kind=AstKind::intn;
                tok->right->str="1";
                return true;
            }
            return false;
        }

        bool AfterDecrement(AstToken*& tok){
            if(r.expect("--")){
                AstToken* tmp=new_AstToken();
                tmp->str="-=";
                tmp->kind=AstKind::op_sub_eq;
                tmp->left=tok;
                tok=tmp;
                tok->right=new_AstToken();
                tok->right->kind=AstKind::intn;
                tok->right->str="1";
                return true;
            }
            return false;
        }

        bool Member(AstToken*& tok){
            assert(tok);
            if(r.expect(".")){
                AstToken* tmp=new_AstToken();
                tmp->kind=AstKind::op_member;
                tmp->str=".";
                tmp->left=tok;
                tok=tmp;
                if(!Identifier(tok->right)){
                    return error("member:expected identifier but not");
                }
                if(tok->right->str=="_"){
                    return error("member:_ is placeholder symbol so not usable for member");
                }
                return true;
            }
            return false;
        }

        bool After(AstToken*& tok){
            assert(tok);
            bool f=false;
            while(Call(tok)||Index(tok)||AfterIncrement(tok)||AfterDecrement(tok)){
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

        bool NumberSuffix(AstToken*& tok,bool floatf,int radix){
            int size=4;
            bool sign=true;
            if(r.expect("h")||r.expect("H")){
                size=2;
            }
            if(r.expect("f")||r.expect("F")){
                floatf=true;
                size=4;
            }
            if(floatf){
                tok->kind=AstKind::floatn;
                if(size==4){
                    tok->type=pool.keyword("float");
                }
                else{
                    tok->type=pool.keyword("double");
                }
            }
            else{
                tok->kind=AstKind::intn;
                if(r.expect("h")||r.expect("H")){
                    size=1;
                }
                if(r.expect("u")||r.expect("U")){
                    sign=false;
                }
                if(size==4&&(r.expect("l")||r.expect("L"))){
                    size=8;
                }
                int ofs=0;
                if(radix==16||radix==2){
                    ofs=2;
                }
                else if(radix==8){
                    ofs=1;
                }
                size_t t=0;
                if(!PROJECT_NAME::parse_int(
                    &tok->str.data()[ofs],&tok->str.data()[tok->str.size()],
                    t,radix)){
                    return error("undecodable number");
                }
                auto signv=PROJECT_NAME::need_bytes(t,false);
                auto unsignv=PROJECT_NAME::need_bytes(t,true);
                if(signv==-1){
                    sign=false;
                }
                if(sign){
                    if(size<signv){
                        size=signv;
                    }
                    switch (size<signv?signv:size)
                    {
                    case 1:
                        tok->type=pool.keyword("int8");
                        break;
                    case 2:
                        tok->type=pool.keyword("int16");
                        break;
                    case 4:
                        tok->type=pool.keyword("int32");
                        break;
                    case 8:
                        tok->type=pool.keyword("int64");
                        break;
                    default:
                        assert(false&&"unreachable");
                    }
                }
                else{
                    switch (size<unsignv?unsignv:size)
                    {
                    case 1:
                        tok->type=pool.keyword("uint8");
                        break;
                    case 2:
                        tok->type=pool.keyword("uint16");
                        break;
                    case 4:
                        tok->type=pool.keyword("uint32");
                        break;
                    case 8:
                        tok->type=pool.keyword("uint64");
                        break;
                    default:
                        assert(false&&"unreachable");
                    }
                }
            }
            return true;
        }

        bool Number(AstToken*& tok){
            if(PROJECT_NAME::is_digit(r.achar())){
                tok=new_AstToken();
                auto tmp=r.set_ignore(nullptr);
                PROJECT_NAME::NumberContext<char> c;
                r.readwhile(tok->str,PROJECT_NAME::number,&c);
                if(c.failed)return error("number:invalid number");
                NumberSuffix(tok,c.floatf,c.radix);
                r.set_ignore(tmp);
                return true;
            }
            return false;
        }

        bool BoolLiteral(AstToken*& tok){
            if(r.expect("true",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="true";
                tok->kind=AstKind::boolean;
                tok->type=pool.keyword("bool");
                return true;
            }
            else if(r.expect("true",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="false";
                tok->kind=AstKind::boolean;
                tok->type=pool.keyword("bool");
                return true;
            }
            return false;
        }

        bool NullLiteral(AstToken*& tok){
            if(r.expect("null",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="null";
                tok->kind=AstKind::null;
                tok->type=pool.keyword("null");
                return true;
            }
            return false;
        }

        bool Func(AstToken*& tok){
            if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::func_literal;
                if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    IdRead(tok->str);
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
                tok->kind=AstKind::identifier;
                IdRead(tok->str);
                assert(tok->str.size());
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
                return true;
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
                        return error("expr:expected ; but not");
                    }
                }
                else{
                    tok->kind=AstKind::func_stmt;
                }
                return true;
            }
            return false;
        }

        bool DeclStmt(AstToken*& tok){
            if(r.expect("decl",PROJECT_NAME::is_c_id_usable)){
                if(!PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    return error("decl statement:expected identifier but not");
                }
                tok=new_AstToken();
                tok->kind=AstKind::decl_stmt;
                IdRead(tok->str);
                tr.func_read(tok->type,false);
                if(r.expect(";")){
                    return error("decl statement:expected ; but not");
                }
                return true;
            }
            return false;
        }

        bool ReturnStmt(AstToken*& tok){
            if(r.expect("return",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::return_stmt;
                if(r.expect(";"))return true;
                Expr(tok->right);
                if(!r.expect(";")){
                    return error("return statement:expected ; but not");
                }
                return true;
            }
            return false;
        }

        bool Stmt(AstToken*& tok){
            return IfStmt(tok)||ForStmt(tok)||ReturnStmt(tok)||BlockStmt(tok)||FuncStmt(tok)||
            DeclStmt(tok)||ExprStmt(tok)||r.expect(";")||error("statement:not match for any statement");
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
            return &r==p?error(err):false;
        }

        Buf& bufctrl(){
            return r.ref();
        }

        template<class Str>
        bool idreadp(PROJECT_NAME::Reader<Buf>* p,Str& str){
            return &r==p?IdRead(str):false;
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
                ret=false;
            }
            catch(...){
                if(err){
                    *err="system error:compiler broken";
                }
                tmpdel();
                ret=false;
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
