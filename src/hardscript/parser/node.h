/*license*/
#pragma once
#include"operators.h"
#include<json_util.h>

namespace node{
    template<class N,class Str>
    struct Error{
        N* ret=nullptr;
        Str err;
        Error(){}
        Error(Error&& from){
            ret=from.ret;
            from.ret=nullptr;
            err=from.err;
        }
        Error(N* r):ret(r){}
        Error(const char* e):err(e){}
        Error& operator =(Error& from){
            if(ret!=nullptr&&ret!=from.ret){
                delete ret;
            }
            ret=from.ret;
            from.ret=nullptr;
            err=from.err;
            return *this;
        }

        Error& operator =(Error&& from){
            if(ret!=nullptr&&ret==from.ret)return *this;
            delete ret;
            ret=from.ret;
            from.ret=nullptr;
            err=std::move(from.err);
            return *this;
        }
        //Error(std::nullptr_t):err("parse error"){}
        N* release(){
            auto tmp=ret;
            ret=nullptr;
            return tmp;
        }

        void append(const char* str){
            err+=str;
        }
        operator bool(){
            return ret!=nullptr;
        }
        ~Error(){
            delete ret;
        }
    };


    template<class N>
    struct NodeList{
        N* node=nullptr;
        N* last=nullptr;
        void push_back(N* n){
            if(!n)return;
            if(!last){
                node=n;
                last=n;
            }
            else{
                last->next=n;
                last=n;
            }
        }
    };

    
    
    template<class Str>
    struct Node{
        node::NodeKind kind=node::NodeKind::unknown;
        Str symbol=Str();
        Node* next=nullptr;
        NodeList<Node> arg;
        NodeList<Node> inblock;
        union{
            Node* left=nullptr;
            Node* init;
            Node* than;
        };
        Node* cond=nullptr;
        union{
            Node* right=nullptr;
            Node* els;
            Node* cont;
        };
        size_t blockbegin=0;
        ~Node(){
            delete left;
            delete right;
            delete cond;
            delete arg.node;
            delete inblock.node;
            delete next;
        }
    };

    template<class Tree>
    Tree* new_node(const char* symbol,node::NodeKind kind){
        try{
            auto t=new Tree();
            t->kind=kind;
            t->symbol=symbol;
            return t;
        }
        catch(...){
            return nullptr;
        }
    }

    template<class Buf,class Tree,class Str,class Ctx>
    Tree* cond(PROJECT_NAME::Reader<Buf>& r,Ctx& ctx){
        auto ret=PROJECT_NAME::expression<Tree,Str>(&r,ctx);
        if(!ret)return nullptr;
        if(r.expect("?")){
            decltype(ret) than=nullptr,els=nullptr,tmp=nullptr;
            than=cond<Buf,Tree,Str>(r,ctx);
            if(!than||!r.expect(":")||!(els=cond<Buf,Tree,Str>(r,ctx))||
              !(tmp=ctx.make_tree("?:",nullptr,nullptr))){
                delete els;
                delete than;
                delete ret;
                return nullptr;
            }
            tmp->cond=ret;
            tmp->than=than;
            tmp->els=els;
            ret=tmp;
        }
        return ret;
    }

    template<class Buf,class Tree,class Str,class Ctx>
    Tree* init(PROJECT_NAME::Reader<Buf>& r,Ctx& ctx){
        auto ret=cond<Buf,Tree,Str>(r,ctx);
        if(!ret)return nullptr;
        if(r.expect(":=")){
            decltype(ret) tmp=nullptr;
            tmp=cond<Buf,Tree,Str>(r,ctx);
            if(!tmp){
                delete ret;
                return nullptr;
            }
            auto res=ctx.make_tree(":=",ret,tmp);
            if(!res){
                delete ret;
                delete tmp;
                return nullptr;
            }
            ret=res;
        }
        return ret;
    }    


    template<class Buf,class Tree,class Str>
    Error<Tree,Str> expr(PROJECT_NAME::Reader<Buf>& r){
        auto rc=[](PROJECT_NAME::Reader<Buf>* self,auto& ctx){
            return cond<Buf,Tree,Str>(*self,ctx);
        };
        using CTX = node::Check<Tree,Buf,decltype(rc),Error<Tree,Str>,Str>;
        typename hard_expr::HardExprTree<Tree,Str,node::NodeKind,Buf,CTX> c(CTX(rc),7);
        auto ret=init<Buf,Tree,Str>(r,c);
        if(!ret){
            if(c.cus.errbuf.err.size()==0){
                return "bin:memory is full";
            }
            Error<Tree,Str> tmp=std::move(c.cus.errbuf);
            return tmp;
        }
        return ret;
    }



    template<class Buf,class Tree,class Str>
    bool block(PROJECT_NAME::Reader<Buf>& r,NodeList<Tree>& n,const char* end,Error<Tree,Str>& err){
        while(!r.eof()){
            if(end&&r.ahead(end)){
                break;
            }
            auto tmp=stmt<Buf,Tree,Str>(r);
            if(!tmp){
                err=std::move(tmp);
                return false;
            }
            n.push_back(tmp.release());
        }
        if(end&&!r.expect(end)){
            return false;
        }
        return true;
    }

    template<class Buf,class Tree,class Str>
    bool case_block(PROJECT_NAME::Reader<Buf>& r,NodeList<Tree>& n,Error<Tree,Str>& err){
        bool def=false;
        Tree* current=nullptr;
        while(!r.eof()){
            if(r.ahead("}")){
                break;
            }
            if(r.expect("case",PROJECT_NAME::is_c_id_usable)){
                auto tmp=expr<Buf,Tree,Str>(r);
                if(!tmp){
                    err=std::move(tmp);
                    return false;
                }
                if(!r.expect(":")){
                    err="case:expected : but not";
                    return false;
                }
                current=new_node<Tree>("case",node::NodeKind::ctrl_case);
                if(!current){
                    err="case:memory is full";
                    return false;
                }
                current->cond=tmp.release();
                n.push_back(current);
            }
            else if(r.expect("default",PROJECT_NAME::is_c_id_usable)){
                if(def){
                    err="default:default statment is already defined";
                    return false;
                }
                if(!r.expect(":")){
                    err="default:expected : but not";
                    return false;
                }
                current=new_node<Tree>("default",node::NodeKind::ctrl_case);
                if(!current){
                    err="default:memory is full";
                    return false;
                }
                n.push_back(current);
                def=true;
            }
            else{
                if(!current){
                    current=new_node<Tree>("init",node::NodeKind::ctrl_case);
                    if(!current){
                        err="case:memory is full";
                        return false;
                    }
                }
                auto tmp=stmt<Buf,Tree,Str>(r);
                if(!tmp){
                    err=std::move(tmp);
                    return false;
                }
                current->inblock.push_back(tmp.release());
            }
        }
        if(!r.expect("}")){
            err="case:expected } but not";
            return false;
        }
        return true;
    }

    template<class Buf,class Tree,class Str>
    Error<Tree,Str> stmt(PROJECT_NAME::Reader<Buf>& r){
        if(r.expect("for",PROJECT_NAME::is_c_id_usable)){
            return for_stmt<Buf,Tree,Str>(r);
        }
        else if(r.expect("if",PROJECT_NAME::is_c_id_usable)){
            return if_stmt<Buf,Tree,Str>(r);
        }
        else if(r.expect("break",PROJECT_NAME::is_c_id_usable)){
            return r.expect(";")?
                   new_node<Tree>("break",node::NodeKind::ctrl_break):nullptr;
        }
        else if(r.expect("continue",PROJECT_NAME::is_c_id_usable)){
            return r.expect(";")?
                   new_node<Tree>("continue",node::NodeKind::ctrl_continue):nullptr;
        }
        else if(r.expect("switch",PROJECT_NAME::is_c_id_usable)){
            return switch_stmt<Buf,Tree,Str>(r);
        }
        else if(r.expect("select",PROJECT_NAME::is_c_id_usable)){
            return select_stmt<Buf,Tree,Str>(r);
        }
        else if(r.expect("return",PROJECT_NAME::is_c_id_usable)){
            return return_stmt<Buf,Tree,Str>(r);
        }
        else{
            auto t= expr<Buf,Tree,Str>(r);
            if(r.expect(";")){
                return "expr:expected ; but not";
            }
            return t;
        }
    }


    template<class Buf,class Tree,class Str>
    Error<Tree,Str> for_stmt(PROJECT_NAME::Reader<Buf>& r){
        auto ret=new_node<Tree>("for",node::NodeKind::ctrl_for);
        if(!ret){
            return "for:memory is full";
        }
        Error<Tree,Str> hold=ret;
        Error<Tree,Str> err;
        if(r.expect("{")){
            ret->blockbegin=r.readpos();
            if(!block<Buf,Tree,Str>(r,ret->inblock,"}",err)){
                return err;
            }
            return ret;
        }
        Error<Tree,Str> cond=expr<Buf,Tree,Str>(r),init,cont;
        if(!cond){
            return cond;
        }
        if(r.expect("{")){
            ret->blockbegin=r.readpos();
            if(!block<Buf,Tree,Str>(r,ret->inblock,"}",err)){
                return err;
            }
            ret->cond=cond.release();
            return ret;
        }
        if(!r.expect(";")){
            return "for:expected ; but not";
        }
        init=cond;
        if(!(cond=expr<Buf,Tree,Str>(r))){
            return cond;
        }
        if(!r.expect(";")){
            return "for:expected ; but not";
        }
        if(!(cont=expr<Buf,Tree,Str>(r))){
            return cont;
        }
        if(!r.expect("{")){
            return "for:expected { but not";
        }   
        ret->blockbegin=r.readpos();
        if(!block<Buf,Tree,Str>(r,ret->inblock,"}",err)){
            return err;
        }
        ret->init=init.release();
        ret->cond=cond.release();
        ret->cont=cont.release();
        return hold;
    }

    template<class Buf,class Tree,class Str>
    Error<Tree,Str> if_stmt(PROJECT_NAME::Reader<Buf>& r){
        auto ret=new_node<Tree>("if",node::NodeKind::ctrl_if);
        if(!ret){
            return "if:memory is full";
        }
        Error<Tree,Str> err;
        auto cond=expr<Buf,Tree,Str>(r);
        if(!cond){
            delete ret;
            return cond;
        }
        if(!r.expect("{")){
            delete ret;
            return "if:expected { but not";
        }
        ret->blockbegin=r.readpos();
        if(!block<Buf,Tree,Str>(r,ret->inblock,"}",err)){
            delete ret;
            return err;
        }
        ret->cond=cond.release();
        if(r.expect("else",PROJECT_NAME::is_c_id_usable)){
            if(r.expect("if",PROJECT_NAME::is_c_id_usable)){
                auto els=if_stmt<Buf,Tree,Str>(r);
                if(!els){
                    delete ret;
                    return els;
                }
                ret->els=els.release();
            }
            else{
                ret->els=new_node<Tree>("else",node::NodeKind::ctrl_if);
                if(!ret->els){
                    return "if:memory is full";
                }
                if(r.expect("{")){
                    return "if:expected { but not";
                }
                ret->blockbegin=r.readpos();
                if(!block<Buf,Tree,Str>(r,ret->els->inblock,"}",err)){
                    delete ret;
                    return err;
                }
            }
        }
        return ret;
    }

    template<class Buf,class Tree,class Str>
    Error<Tree,Str> switch_stmt(PROJECT_NAME::Reader<Buf>& r){
        auto ret=new_node<Tree>("switch",node::NodeKind::ctrl_switch);
        if(!ret){
            return "switch:memory is full";
        }
        Error<Tree,Str> err=expr<Buf,Tree,Str>(r);
        if(!err){
            return err;
        }
        ret->cond=err.release();
        if(!r.expect("{")){
            return "switch:expected { but not";
        }
        ret->blockbegin=r.readpos();
        if(!case_block<Buf,Tree,Str>(r,ret->inblock,err)){
            delete ret;
            return err;
        }
        return ret;
    }


    template<class Buf,class Tree,class Str>
    Error<Tree,Str> select_stmt(PROJECT_NAME::Reader<Buf>& r){
        auto ret=new_node<Tree>("select",node::NodeKind::ctrl_select);
        if(!ret){
            return "select:memory is full";
        }
        if(!r.expect("{")){
            return "select:expected { but not";
        }
        Error<Tree,Str> err;
        ret->blockbegin=r.readpos();
        if(!case_block<Buf,Tree,Str>(r,ret->inblock,err)){
            delete ret;
            return err;
        }
        return ret;
    }

    template<class Buf,class Tree,class Str>
    Error<Tree,Str> return_stmt(PROJECT_NAME::Reader<Buf>& r){
        auto ret=new_node<Tree>("return",node::NodeKind::ctrl_return);
        if(!ret){
            return "return:memory is full";
        }
        if(r.expect(";"))return ret;
        auto tmp=expr<Buf,Tree,Str>(r);
        if(!tmp){
            delete ret;
            return tmp;
        }
        ret->right=tmp.release();
        return ret;
    }



    template<class Str>
    void to_json(PROJECT_NAME::JSON& j,const Node<Str>& n){
        using namespace PROJECT_NAME;
        using K=NodeKind;
        auto block=[&](){
            auto& ref=j["block"];
            j["blockpos"]=n.blockbegin;
            ref="[]"_json;
            for(auto p=n.inblock->node;p!=nullptr;p=p->next){
               ref.push_back(p);
            };
        };
        j["kind"]=n.kind;
        j["kindnum"]=(int)n.kind;
        j["symbol"]=n.symbol;
        if(n.kind==K::ctrl_for){
            j.not_null_assign("init",n.init);
            j.not_null_assign("cond",n.cond);
            j.not_null_assign("cont",n.cont);
            block();
        }
        else if(n.kind==K::op_cond){
            j.not_null_assign("cond",n.cond);
            j.not_null_assign("than",n.than);
            j.not_null_assign("else",n.els);
        }
        else if(n.kind==K::ctrl_if){
            j["cond"]=n.cond;
            block();
            j.not_null_assign("else",n.els);
        }
        else if(n.kind==K::ctrl_case){
            j["cond"]=n.cond;
            block();
        }
        else if(n.kind==K::ctrl_switch){
            j["cond"]=n.cond;
            block();
        }
        else if(n.kind==K::ctrl_select){
            block();
        }
        else {
            j.not_null_assign("left",n.left);
            j.not_null_assign("right",n.right);
            
        }
    }

    inline void to_json(PROJECT_NAME::JSON& to,const NodeKind& kind){
#define NK_CASE_BLOCK(x,str) case K::x: to=str;break;
        using K=NodeKind;
        switch (kind)
        {
        NK_CASE_BLOCK(obj_bool,"bool")
        NK_CASE_BLOCK(obj_int,"int")
        NK_CASE_BLOCK(obj_float,"float")
        NK_CASE_BLOCK(obj_string,"string")
        NK_CASE_BLOCK(obj_new,"new")
        NK_CASE_BLOCK(obj_closure,"closure")
        NK_CASE_BLOCK(obj_id,"id")
        NK_CASE_BLOCK(ctrl_for,"for")
        NK_CASE_BLOCK(ctrl_if,"if")
        NK_CASE_BLOCK(ctrl_case,"case")
        NK_CASE_BLOCK(ctrl_select,"select")
        NK_CASE_BLOCK(ctrl_switch,"switch")
        default:
            to=node::operator_to_str(kind);
            break;
        }
#undef NK_CASE_BLOCK
    }

    template<class Str>
    void from_json(Node<Str>& to,const PROJECT_NAME::JSON& from){
        int n=0;
        from.at("kindnum").get_to(n);
        to.kind=(NodeKind)n;
        from.at("symbol").get_to(to.symbol);
        if(auto block=from.idx("block")){
            size_t ofs=0;
            for(auto i=block->idx(ofs);i;ofs++,i=block->idx(ofs)){
                to.inblock.push_back(i->get_ptr<Node<Str>>());
            }
            from.at("blockpos").get_to(to.blockbegin);
        }
        auto idxptr=[&](auto& t,const char* i){
            if(auto id=from.idx(i)){
                t=id->get_ptr<Node<Str>>();
                return true;
            }
            return false;
        };
        idxptr(to.cond,"cond");
        idxptr(to.right,"right")||idxptr(to.els,"else")||idxptr(to.cont,"cont");
        idxptr(to.left,"left")||idxptr(to.than,"than")||idxptr(to.init,"init");
    }
}
