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
        //std::vector<SAstToken> temp;
        TyPool& pool;
        type::TypeAstReader<AstReader,Buf> tr;

        SAstToken new_AstToken(){
            auto ret=ast::new_<AstToken>();
            //temp.push_back(ret);
            return SAstToken(ret,ast::delete_<AstToken>);
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
               e("null")||e("true")||e("false")||e("cast")||e("type")||e("use")
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

        bool Call(SAstToken& tok){
            if(r.expect("(")){
                SAstToken tmp=new_AstToken();
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

        bool Index(SAstToken& tok){
            if(r.expect("[")){
                SAstToken tmp=new_AstToken();
                tmp->str="[]";
                tmp->kind=AstKind::op_index;
                tmp->right=std::move(tok);
                tok=std::move(tmp);
                Expr(tok->left);
                if(!r.expect("]")){
                    return error("index:expected ] but not");
                }
                return true;
            }
            return false;
        }

        bool AfterIncrement(SAstToken& tok){
            if(r.expect("++")){
                SAstToken tmp=new_AstToken();
                tmp->str="++";
                tmp->kind=AstKind::op_add_eq;
                tmp->left=std::move(tok);
                tok=std::move(tmp);
                tok->right=new_AstToken();
                tok->right->kind=AstKind::intn;
                tok->right->str="1";
                return true;
            }
            return false;
        }

        bool AfterDecrement(SAstToken& tok){
            if(r.expect("--")){
                SAstToken tmp=new_AstToken();
                tmp->str="-=";
                tmp->kind=AstKind::op_sub_eq;
                tmp->left=std::move(tok);
                tok=std::move(tmp);
                tok->right=new_AstToken();
                tok->right->kind=AstKind::intn;
                tok->right->str="1";
                return true;
            }
            return false;
        }

        bool Member(SAstToken& tok){
            assert(tok);
            if(r.expect(".")){
                SAstToken tmp=new_AstToken();
                tmp->kind=AstKind::op_member;
                tmp->str=".";
                tmp->left=std::move(tok);
                tok=std::move(tmp);
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

        bool After(SAstToken& tok){
            assert(tok);
            bool f=false;
            while(Call(tok)||Index(tok)||Member(tok)||AfterIncrement(tok)||AfterDecrement(tok)){
                assert(tok);
                f=true;
            }
            return f;
        }

        bool String(SAstToken& tok){
            if(r.achar()=='\"'||r.achar()=='\''){
                auto ret=new_AstToken();
                ret->kind=AstKind::string;
                if(!r.string(ret->str))return error("string:invalid string");
                tok=ret;
                return true;
            }
            return false;
        }

        bool NumberSuffix(SAstToken& tok,bool floatf,int radix){
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

        bool Number(SAstToken& tok){
            if(PROJECT_NAME::is_digit(r.achar())){
                tok=new_AstToken();
                auto tmp=r.set_ignore(nullptr);
                PROJECT_NAME::NumberContext<char> c;
                r.readwhile(tok->str,PROJECT_NAME::number,&c);
                if(c.failed)return error("number:invalid number");
                NumberSuffix(tok,any(c.flag&PROJECT_NAME::NumberFlag::floatf),c.radix);
                r.set_ignore(tmp);
                return true;
            }
            return false;
        }

        bool BoolLiteral(SAstToken& tok){
            if(r.expect("true",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="true";
                tok->kind=AstKind::boolean;
                tok->type=pool.keyword("bool");
                return true;
            }
            else if(r.expect("false",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="false";
                tok->kind=AstKind::boolean;
                tok->type=pool.keyword("bool");
                return true;
            }
            return false;
        }

        bool NullLiteral(SAstToken& tok){
            if(r.expect("null",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->str="null";
                tok->kind=AstKind::null;
                tok->type=pool.keyword("null");
                return true;
            }
            return false;
        }

        bool Func(SAstToken& tok){
            if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::func_literal;
                bool id=false;
                if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    IdRead(tok->str);
                    id=true;
                }
                tr.func_read(tok->type,true);
                if(!r.expect("{")){
                    return error("func literal:expected { but not");
                }
                if(id){
                    auto nm="func "+tok->str;
                    if(!pool.makescope(nm)){
                        return error("func literal:func name conflict");
                    }
                    pool.enterscope(nm);
                }
                else{
                    tok->scope=pool.unnamed_scope("func ");
                }
                Block(tok->block);
                auto f=pool.leavescope();
                assert(f);
                return true;
            }
            return false;
        }

        bool Identifier(SAstToken& tok){
            if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                tok=new_AstToken();
                tok->kind=AstKind::identifier;
                IdRead(tok->str);
                assert(tok->str.size());
                return true;
            }
            return false;
        }

        bool ScopedIdentifier(SAstToken& tok){
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

        bool GlobalScopedIdentifier(SAstToken& tok){
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

        bool Primary(SAstToken& tok){
            return (String(tok)||Number(tok)||BoolLiteral(tok)||NullLiteral(tok)||Func(tok)||
                   ScopedIdentifier(tok)||GlobalScopedIdentifier(tok)||
                   error("primary:expect primary object but not"))&&
                   (After(tok)||true);
        }

        bool Increment(SAstToken& tok){
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

        bool Decrement(SAstToken& tok){
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

        bool BitNot(SAstToken& tok){
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

        bool LogicalNot(SAstToken& tok){
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

        bool SinglePlus(SAstToken& tok){
            if(r.expect("+")){
                return Unary(tok);
            }
            return false;
        }

        bool SingleMinus(SAstToken& tok){
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

        bool Address(SAstToken& tok){
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

        bool Dereference(SAstToken& tok){
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

        bool Cast(SAstToken& tok){
            if(r.expect("cast",PROJECT_NAME::is_c_id_usable)){
                if(!r.expect("<"))return error("cast:expected < but not");
                tok=new_AstToken();
                tok->kind=AstKind::op_cast;
                tr.read_type(tok->type);
                if(!r.expect(">"))return error("cast:expected > but not");
                if(!r.expect("("))return error("cast:expected ( but not");
                Expr(tok->right);
                if(!r.expect(")"))return error("cast:expected ) but not");
                return true;
            }
            return false;
        }

        bool Unary(SAstToken& tok){
            return Increment(tok)||Decrement(tok)||SinglePlus(tok)||BitNot(tok)||LogicalNot(tok)||
                   SingleMinus(tok)||Address(tok)||Dereference(tok)||Cast(tok)||Primary(tok);
        }

        template<class Func>
        inline bool Bin(AstKind kind,const char* str,const char* err,Func func,SAstToken& tok){
            assert(tok);
            if(r.expect(str)){
                auto n=new_AstToken();
                n->str=str;
                n->kind=kind;
                n->left=tok;
                tok=std::move(n);
                if(!func(tok->right)){
                    return error(err);
                }
                return true;
            }
            return false;
        }

        template<class Func,class Not>
        inline bool NEqBin(AstKind kind,const char* str,const char* err,Func func,Not no,SAstToken& tok){
            assert(tok);
            if(r.expect(str,no)){
                auto n=new_AstToken();
                n->str=str;
                n->kind=kind;
                n->left=tok;
                tok=std::move(n);
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
#define MEMBER_CAPTURE(f) [this](SAstToken& tok){return f(tok);}

        bool Mul(SAstToken& tok){
            return NEqBin(AstKind::op_mul,"*","mul",MEMBER_CAPTURE(Unary),not_eq_s,tok);
        }

        bool Div(SAstToken& tok){
            return NEqBin(AstKind::op_div,"/","div",MEMBER_CAPTURE(Unary),not_eq_s,tok);
        }

        bool Mod(SAstToken& tok){
            return NEqBin(AstKind::op_mod,"%","mod",MEMBER_CAPTURE(Unary),not_eq_s,tok);
        }

        bool Bin_Mul(SAstToken& tok){
            if(Unary(tok)){
                while(Mul(tok)||Div(tok)||Mod(tok));
                return true;
            }
            return false;
        }

        bool Add(SAstToken& tok){
            return NEqBin(AstKind::op_add,"+","add",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool Sub(SAstToken& tok){
            return NEqBin(AstKind::op_sub,"-","sub",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool ShiftLeft(SAstToken& tok){
            return NEqBin(AstKind::op_shl,"<<","shift left",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool ShiftRight(SAstToken& tok){
            return NEqBin(AstKind::op_shr,">>","shift right",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool BitOr(SAstToken& tok){
            return NEqBin(AstKind::op_bitor,"|","bit or",MEMBER_CAPTURE(Bin_Mul),[](char c){return c=='='||c=='|';},tok);
        }

        bool BitAnd(SAstToken& tok){
            return NEqBin(AstKind::op_bitand,"&","bit and",MEMBER_CAPTURE(Bin_Mul),[](char c){return c=='='||c=='&';},tok);
        }

        bool BitXor(SAstToken& tok){
            return NEqBin(AstKind::op_bitxor,"^","bit xor",MEMBER_CAPTURE(Bin_Mul),not_eq_s,tok);
        }

        bool Bin_Add(SAstToken& tok){
            if(Bin_Mul(tok)){
                while(Add(tok)||Sub(tok)||ShiftLeft(tok)||ShiftRight(tok)||BitOr(tok)||
                      BitAnd(tok)||BitXor(tok));
                return true;
            }
            return false;
        }

        bool Equal(SAstToken& tok){
            return Bin(AstKind::op_equ,"==","equal",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool NotEqual(SAstToken& tok){
            return Bin(AstKind::op_neq,"!=","not equal",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftGreaterRight(SAstToken& tok){
            return Bin(AstKind::op_lgr,">","left greater right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftLesserRight(SAstToken& tok){
            return Bin(AstKind::op_llr,"<","left lesser right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftEqualLesserRight(SAstToken& tok){
            return Bin(AstKind::op_lelr,"<=","left equal lesser right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool LeftEqualGreaterRight(SAstToken& tok){
            return Bin(AstKind::op_legr,">=","left equal greater right",MEMBER_CAPTURE(Bin_Add),tok);
        }

        bool Bin_Compare(SAstToken& tok){
            if(Bin_Add(tok)){
                while(Equal(tok)||NotEqual(tok)||LeftEqualGreaterRight(tok)||LeftEqualLesserRight(tok)||
                      LeftGreaterRight(tok)||LeftLesserRight(tok));
                return true;
            }
            return false;
        }

        bool Bin_LogicalAnd(SAstToken& tok){
            if(Bin_Compare(tok)){
                while(Bin(AstKind::op_logicaland,"&&","logical and",MEMBER_CAPTURE(Bin_Compare),tok));
                return true;
            }
            return false;
        }

        bool Bin_LogicalOr(SAstToken& tok){
            if(Bin_LogicalAnd(tok)){
                while(Bin(AstKind::op_logicalor,"||","logical or",MEMBER_CAPTURE(Bin_LogicalAnd),tok));
                return true;
            }
            return false;
        }

        bool Assign(SAstToken& tok){
            return Bin(AstKind::op_assign,"=","assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool AddAssign(SAstToken& tok){
            return Bin(AstKind::op_add_eq,"+=","add assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool SubAssign(SAstToken& tok){
            return Bin(AstKind::op_sub_eq,"-=","sub assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool MulAssign(SAstToken& tok){
            return Bin(AstKind::op_mul_eq,"*=","mul assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool DivAssign(SAstToken& tok){
            return Bin(AstKind::op_div_eq,"/=","div assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool ModAssign(SAstToken& tok){
            return Bin(AstKind::op_mod_eq,"%=","mod assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool ShiftLeftAssign(SAstToken& tok){
            return Bin(AstKind::op_shl_eq,"<<=","shift left assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool ShiftRightAssign(SAstToken& tok){
            return Bin(AstKind::op_shr_eq,">>=","shift right assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool BitOrAssign(SAstToken& tok){
            return Bin(AstKind::op_bitor_eq,"|=","bit or assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool BitAndAssign(SAstToken& tok){
            return Bin(AstKind::op_bitand_eq,"&=","bit and assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool BitXorAssign(SAstToken& tok){
            return Bin(AstKind::op_bitxor_eq,"|=","bit xor assign",MEMBER_CAPTURE(Bin_Assign),tok);
        }

        bool Bin_Assign(SAstToken& tok){
            if(Bin_LogicalOr(tok)){
                while(Assign(tok)||AddAssign(tok)||SubAssign(tok)||MulAssign(tok)||DivAssign(tok)||ModAssign(tok)||
                      ShiftRightAssign(tok)||ShiftLeftAssign(tok)||BitOrAssign(tok)||BitAndAssign(tok)||BitXorAssign(tok));
                return true;
            }
            return false;
        }

        bool Cond(SAstToken& tok){
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

        bool Init(SAstToken& tok){
            if(Cond(tok)){
                Bin(AstKind::op_init,":=","init",MEMBER_CAPTURE(Cond),tok);
                return true;
            }
            return false;
        }

        bool Expr(SAstToken& tok){
            return Cond(tok);
        }

        bool ExprStmt(SAstToken& tok){
            if(Init(tok)){
                if(!r.expect(";")){
                    return error("expr statement:expected ; but not");
                }
                return true;
            }
            return false;
        }

        bool IfStmt(SAstToken& tok){
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
                    tok->scope=pool.unnamed_scope("if ");
                    Block(tok->block);
                    auto f=pool.leavescope();
                    assert(f);
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

        bool ForStmt(SAstToken& tok){
            if(r.expect("for",PROJECT_NAME::is_c_id_usable)){
                auto finally=[&tok,this]{
                    tok->scope=pool.unnamed_scope("for ");
                    Block(tok->block);
                    auto f=pool.leavescope();
                    assert(f);
                    return true;
                };
                tok=new_AstToken();
                tok->kind=AstKind::for_stmt;
                if(r.expect("{")){
                    return finally();
                }
                Init(tok->cond);
                if(r.expect("{")){
                    return finally();
                }
                else if(!r.expect(";")){
                    return error("for statement:expected ; or { but not");
                }
                tok->left=tok->cond;
                tok->cond=nullptr;
                if(r.expect(";")){}
                else{
                    Expr(tok->cond);
                    if(!r.expect(";")){
                        return error("for statement:expected ; but not");
                    }
                }
                if(r.expect("{")){
                    return finally();
                }
                Expr(tok->right);
                if(!r.expect("{")){
                    return error("for statement:expected { but not");
                }
                return finally();
            }
            return false;
        }

        bool Block(AstList<AstToken>& tok){
            while(!r.expect("}")){
                if(r.eof()){
                    return error("block:unexpected EOF");
                }
                SAstToken stmt=nullptr;
                Stmt(stmt);
                tok.push_back(std::move(stmt));
            }
            return true;
        }

        bool GlobalBlock(AstList<AstToken>& tok){
            while(!r.expect("}")){
                if(r.eof()){
                    return error("block:unexpected EOF");
                }
                SAstToken stmt=nullptr;
                GlobalStmt(stmt);
                tok.push_back(std::move(stmt));
            }
            return true;
        }

        bool BlockStmt(SAstToken& tok){
            if(r.expect("{")){
                tok=new_AstToken();
                tok->kind=AstKind::block_stmt;
                tok->scope=pool.unnamed_scope("block ");
                Block(tok->block);
                auto f=pool.leavescope();
                assert(f);
                return true;
            }
            return false;
        }

        bool CaseBlock(AstList<AstToken>& tok){
            SAstToken current=nullptr;
            while(!r.expect("}")){
                if(r.eof()){
                    return error("case block:unexpected EOF");
                }
                if(r.expect("case",PROJECT_NAME::is_c_id_usable)){
                    current=new_AstToken();
                    current->kind=AstKind::case_stmt;
                    Expr(current->cond);
                    if(!r.expect(":"))return error("case block:expected : but not");
                    tok.push_back(current);
                }
                else if(r.expect("default",PROJECT_NAME::is_c_id_usable)){
                    current=new_AstToken();
                    current->kind=AstKind::default_stmt;
                    if(!r.expect(":"))return error("case block:expected : but not");
                    tok.push_back(current);
                }
                else{
                    if(!current){
                        current=new_AstToken();
                        current->str="init";
                        current->kind=AstKind::sw_init_stmt;
                        tok.push_back(current);
                    }
                    SAstToken tmp=nullptr;
                    Stmt(tmp);
                    current->block.push_back(std::move(tmp));
                }
            }
            return true;
        }

        bool SwitchStmt(SAstToken& tok){
            if(r.expect("switch",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::switch_stmt;
                tok->str="switch";
                Expr(tok->cond);
                if(!r.expect("{"))return error("switch:expected { but not");
                CaseBlock(tok->block);
                return true;
            }
            return false;
        }
        

        bool VarStmt(SAstToken& tok){
            if(r.expect("var",PROJECT_NAME::is_c_id_usable)){
                bool bracketf=false;
                if(r.expect("(")){
                    bracketf=true;
                }
                tok=new_AstToken();
                tok->kind=AstKind::var_stmt;
                tok->str="var";
                do{
                    AstList<AstToken> list;
                    size_t count=0;
                    SAstToken tmp=nullptr;
                    bool first=true;
                    do{
                        if(!Identifier(tmp))return error("var:expected identifier but not");
                        if(first){
                            tmp->kind=AstKind::init_border;
                        }
                        else{
                            tmp->kind=AstKind::init_var;
                        }
                        list.push_back(std::move(tmp));
                        count++;
                    }while(!r.expect(","));
                    if(!r.ahead("=")){
                        ast::type::SType type=nullptr;
                        tr.read_type(type);
                        foreach_node(p,list){
                            p->type=type;
                        }
                    }
                    if(r.expect("=")){
                        first=true;
                        foreach_node(p,list){
                            if(!first&&!r.expect(",")){
                                return error("var:expected , but not");
                            }
                            Expr(p->right);
                            if(first){
                                if(!r.ahead(",")){
                                    break;
                                }
                                first=false;
                            }
                        }
                    }
                    tok->block.push_back(std::move(list.node));
                }while(bracketf&&!r.expect(")"));
                return true;
            }
            return false;
        }

        bool FuncStmt(SAstToken& tok){
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

        bool DeclStmt(SAstToken& tok){
            if(r.expect("decl",PROJECT_NAME::is_c_id_usable)){
                if(!PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    return error("decl statement:expected identifier but not");
                }
                tok=new_AstToken();
                tok->kind=AstKind::decl_stmt;
                IdRead(tok->str);
                tr.func_read(tok->type,false);
                if(!r.expect(";")){
                    return error("decl statement:expected ; but not");
                }
                return true;
            }
            return false;
        }

        bool ReturnStmt(SAstToken& tok){
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

        bool NameSpaceStmt(SAstToken& tok){
            if(r.expect("namespace",PROJECT_NAME::is_c_id_usable)){
                size_t i=0;
                tok=new_AstToken();
                tok->kind=AstKind::namespace_stmt;
                while(true){
                    if(!PROJECT_NAME::is_c_id_top_usable(r.achar())){
                        return error("namespace:expected identifier but not");
                    }
                    std::string name;
                    IdRead(name);
                    i++;
                    if(!pool.enterscope(name)){
                        pool.makescope(name);
                        auto f=pool.enterscope(name);
                        assert(f);
                    }
                    tok->str+=name;
                    if(r.expect("::"))continue;
                    break;
                }
                if(!r.expect("{")){
                    return error("namespace:expected { but not");
                }
                GlobalBlock(tok->block);
                while(i){
                    auto g=pool.leavescope();
                    assert(g);
                    i--;
                }
                return true;
            }
            return false;
        }

        bool TypeStmt(SAstToken& tok){
            if(r.expect("type",PROJECT_NAME::is_c_id_usable)){
                tok=new_AstToken();
                tok->kind=AstKind::type_stmt;     
                if(!PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    return error("type statement:expected identifier but not");
                }
                IdRead(tok->str);
                tok->type=pool.alias(tok->str.c_str());
                pool.register_type(tok->str,tok->type);
                tr.read_type(tok->type->base);
                if(!r.expect(";")){
                    return error("type statement:expected ; but not");
                }
                return true;
            }
            return false;
        }

        bool Stmt(SAstToken& tok){
            return TypeStmt(tok)||IfStmt(tok)||ForStmt(tok)||ReturnStmt(tok)||SwitchStmt(tok)||BlockStmt(tok)||FuncStmt(tok)||
            VarStmt(tok)||DeclStmt(tok)||r.expect(";")||ExprStmt(tok)||error("statement:not match for any statement");
        }

        bool GlobalStmt(SAstToken& tok){
            return NameSpaceStmt(tok)||Stmt(tok);
        }

        bool Stmts(AstList<AstToken>& tok){
            while(!r.eof()){
                SAstToken stmt=nullptr;
                GlobalStmt(stmt);
                tok.push_back(std::move(stmt));
            }
            return true;
        }

        bool Program(SAstToken& tok){
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

        bool parse(SAstToken& tok,const char** err=nullptr){
            r.set_ignore(PROJECT_NAME::ignore_c_comments);
            bool ret=true;
            auto tmpdel=[this]{
                /*for(auto p:temp){
                    delete_(p);
                }
                pool.delall();*/
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
            //temp.resize(0);
            return ret;
        }

        bool linepos(PROJECT_NAME::LinePosContext& ctx){
            r.readwhile(PROJECT_NAME::linepos,ctx);
            return true;
        }

        bool exprp(PROJECT_NAME::Reader<Buf>* p,SAstToken& tok){
            if(p!=&r)return error("");
            //auto b=temp.size();
            Expr(tok);
            /*if(b<temp.size()){
                temp.erase(temp.begin()+b,temp.begin()+temp.size());
            }*/
            return true;
        } 
    };
#undef MEMBER_CAPTURE
}
