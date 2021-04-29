#include"reader.h"
#include"reader_helper.h"
#include"basic_tree.h"

namespace PROJECT_NAME{
    template<class Tree,class Kind,class Buf>
    struct ExampleTree{
        Tree* result=nullptr;

        template<class Str>
        Tree* make_tree(Str symbol,Tree*left=nullptr,Tree* right=nullptr,Kind kind=Kind::unknown){
            Tree* ret=nullptr;
            try{
                ret=new Tree();
            }
            catch(...){
                return nullptr;
            }
            ret->symbol=symbol;
            ret->left=left;
            ret->right=right;
            ret->kind=kind;
            return ret;
        }

        void delete_tree(Tree* tree){
            delete tree;
        }

        bool op_expect(Reader<Buf>*,const char*&){return false;}

        template<class First,class... Other>
        bool op_expect(Reader<Buf>* self,const char*& expected,First first,Other... other){
            return self->expect(first,expected)||op_expect(self,expected,other...);
        }

        int depthmax=7;
    
        bool expect(Reader<Buf>* self,int depth,const char* expected){
            auto check=[&](auto... args){return op_expect(self,expected,args...);};
            switch (depth){
            case 7:
                return check("=");
            case 6:
                return check("||");
            case 5:
                return check("&&");
            case 4:
                return check("==","!=");
            case 3:
                return check("<=",">=","<",">");
            case 2:
                return check("*","/","%");
            case 1:
                return check("+","-");
            case 0:
                return check("++","--","+","-","!");
            default:
                return false;
            }
        }

        Tree* primary_tree(Reader<Buf>* self){
            Tree* ret=nullptr;
            if(self->expect("(")){
                ret=expression<Tree>(self,*this);
                if(!ret)return nullptr;
                if(!self->expect(")")){
                    delete ret;
                    return nullptr;
                }
            }
            else if(self->ahead("\"")||self->ahead("'")){
                Buf str;
                if(!reader.string(str))return nullptr;
                ret=make_tree(str,nullptr,nullptr,Kind::string);
                if(!ret)return nullptr;
            }
            else if(is_digit(self->achar())){
                Buf num;
                NumberContext<char> ctx;
                self->readwhile(num,number,&ctx);
                if(!ctx.succeed)return nullptr;
                if(ctx.floatf){
                    ret=make_tree(num,nullptr,nullptr,Kind::floats);
                }
                else{
                    ret=make_tree(num,nullptr,nullptr,Kind::integer);
                }
                if(!ret)return nullptr;
            }
            else if(reader.expect("new",is_c_id_usable)){
                auto tmp=expression(self,*this);
                if(!tmp)return nullptr;
                ret=make<Tree>("new",nullptr,tmp,EvalType::create);
                if(!ret){
                    delete tmp;
                    return nullptr;
                }
            }  
            else if(is_c_id_top_usable(reader.achar())){
                Buf id;
                reader.readwhile(id,untilincondition,is_c_id_usable<char>);
                if(id.size()==0)return nullptr;
                if(id=="true"||id=="false"){
                    ret=make_tree(id,nullptr,nullptr,Kind::boolean);
                }
                else{
                    ret=make_tree(id);
                }
                if(!ret)return nullptr;
            }
            return ret;
        }

        Tree* after_tree(Tree* ret,Reader<Buf>* self){
            const char* expected=nullptr;
            while(true){
                if(self->expect("(")){
                    auto hold=make_tree("()",nullptr,ret);
                    if(!hold){
                        delete ret;
                        return nullptr;
                    }
                    ret=hold;
                    while(true){
                        if(self->expect(")"))break;
                        auto tmp=expression<Tree>(self,*this);
                        if(!tmp||(!self->expect(",")&&!self->ahead(")"))){
                            delete tmp;
                            delete ret;
                            return nullptr;
                        }
                        ret->arg.push_back(tmp);
                    }
                    continue;
                }
                else if(self->expect("[")){
                    auto hold=make_tree("[]",nullptr,ret);
                    if(!hold){
                        delete ret;
                        return nullptr;
                    }
                    ret=hold;
                    auto tmp=expression<Tree>(self,*this);
                    if(!tmp||!self->ahead("]")){
                        delete tmp;
                        delete ret;
                        return nullptr;
                    }
                    ret->left=tmp;
                    continue;
                }
                else if(reader.expect(".")){
                    if(!is_c_id_top_usable(reader.achar())){
                        delete ret;
                        return false;
                    }
                    Buf id;
                    reader.readwhile(id,untilincondition,is_c_id_usable<char>);
                    if(id.size()==0)return false;
                    auto tmp1=make_tree(id,nullptr,nullptr);
                    if(!tmp1){
                        delete ret;
                        return false;
                    }
                    auto tmp2=make_tree(".",ret,tmp1);
                    if(!tmp2){
                        delete ret;
                        delete tmp1;
                        return false;
                    }
                    ret=tmp2;
                    continue;
                }
                else if(reader.expect("++",expected)||reader.expect("--",expected)){
                    auto tmp=make_tree(expected,nullptr,ret);
                    if(!tmp){
                        delete ret;
                        return false;
                    }
                    ret=tmp;
                    continue;
                }
                break;
            }
            return ret;
        }
    };
}