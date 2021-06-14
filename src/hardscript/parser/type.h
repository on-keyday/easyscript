#pragma once
#include<reader.h>
#include<basic_helper.h>
#include"node.h"
#include<vector>
#include<map>
#include<assert.h>
namespace type{
    enum class TypeKind{
        alias=0x1,
        primary=0x2,
        pointer=0x10,
        not_null_pointer=0x20,
        temporary=0x40,
        array_n=0x80,
        array_p=0x100,
        has_member=0x1000,
        structure=has_member|0x1,
        unions=has_member|0x2,
        enums=has_member|0x4,
        interface=has_member|0x8,
        generic=0x2000,
        function=0x4000,
    };
    
    enum class TypeAttribute{
        noattr=0x0,
        constant=0x1, 
        thread=0x2,
        atomic=0x4,
        align=0x8,

        global=0x10,
        local=0x20,
    };

    enum class TypePlace{
        arg,
        in_func,
        flat,
        in_struct
    };

    enum class FuncSigMode{
        has_ret=0x1,
        has_type=0x2,
        need_bracket=0x4,
    };

    enum class ScopeKind{
        unset,
        package,
        function,
        block,
        structure,
    };

    enum class ScopeError{
        noerror,
        match_some_name,
        not_found,
        in_the_middle
    };

    template<class T>
    constexpr T enum_or(T l,T r){
        return static_cast<T>(static_cast<uint64_t>(l)|static_cast<uint64_t>(r));
    }

    template<class T>
    constexpr T enum_and(T l,T r){
         return static_cast<T>(static_cast<uint64_t>(l)&static_cast<uint64_t>(r));
    }

    template<class Str>
    struct Object{
        using Type=Type<Str>;
        Type*  type;
        TypeAttribute attr;
        size_t align=0;
        Str name=Str();
        node::Node<Str>* initial=nullptr;
        Object(Object&& in){
            type=in.type;
            initial=in.initial;
            in.initial=nullptr;
            name=std::move(in.name);
        }
        ~Object(){
            delete initial;
        }
    };

    template<class Str>
    struct Attribute{
        TypeAttribute attrflag;
        node::Node<Str>* align_size=nullptr;
        node::Node<Str>* optional_attr=nullptr;
    };

    template<class Str>
    struct Members{
        std::map<Str,Object<Str>> member;
    };

    template<class Str>
    struct Func{
        std::vector<Object<Str>> arg;
    };

    template<class Str>
    struct Derived{
        size_t pointer=0;
        size_t not_null=0;
        size_t temporary=0;
        std::map<size_t,size_t> array_n;
        std::map<node::Node<Str>*,size_t> array_p;
    };

    /*template<class Str>
    struct Generic{
        
    };*/

    template<class Str>
    struct Type{
        Str signature=Str();
        Type* base=nullptr;
        TypeKind kind=TypeKind::primary;
        union{
            Members<Str>* memb=nullptr;
            Func<Str>* func;
            //Generic<Str>* gener;
            node::Node<Str>* size_p;
            size_t size_n;
        };
        Derived<Str> derived;
        ~Type(){
            if(kind==TypeKind::function){
                delete func;
            }
            else if(kind==TypeKind::array_p){
                delete size_p;
            }
            else if(enum_and(kind,TypeKind::has_member)){
                delete memb;
            }
        }
    };

    template<class Str>
    struct Scope{
        ScopeKind kind=ScopeKind::unset;
        Str name=Str();
        std::map<Str,Type<Str>> struct_ty;
        std::map<Str,Type<Str>> interface_ty;
        std::vector<Type<Str>> generic_ty;
        std::map<Str,Type<Str>> alias_ty;
        std::map<Str,Type<Str>> function_ty;
        std::map<Str,Scope> child_scope;
        std::vector<Scope*> using_scope;
        Scope* parent=nullptr;
        Scope* prev=nullptr;
    };

    template<class Str>
    struct TypePool{
        using Type=Type<Str>;
        using Scope=Scope<Str>;
        Scope global_scope;
        Scope* now_scope=&global_scope;

        Type void_ty;
        Type bool_ty;
        std::map<size_t,Type> int_ty;
        std::map<size_t,Type> uint_ty;
        Type f64_ty;
        Type f32_ty;
        std::map<size_t,Type> derived_ty;
        size_t count=1;

        Scope* search_scope_in_scope(PROJECT_NAME::Reader<Str>& r,Scope* sc){
            auto beginpos=r.readpos();
            Str id=Str();
            r.readwhile(id,PROJECT_NAME::until,':');
            if(id.size()==0){
                r.seek(beginpos);
                return nullptr;
            }
            if(!sc->child_scope.count(id)){
                r.seek(beginpos);
                return sc;
            }
            if(r.expect("::")){
                return search_scope_in_scope(r,&sc->child_scope[id]);
            }
            return &sc->child_scope[id];
        }

        Scope* get_require_kind_scope(Scope* search,ScopeKind kind){
            while(search&&search->kind!=kind){
                search=search->parent;
            }
            return search;
        }

        Scope* resolve_namespace(PROJECT_NAME::Reader<Str> r,ScopeError& err,bool finmust){
            auto fincheck=[&]{
                return !finmust||r.ceof();
            };
            
            auto global_search=[&]{
                auto ret=search_scope_in_scope(r,&global_scope);
                if(!ret){
                    err=ScopeError::not_found;
                    return &global_scope;
                }
                if(!fincheck()){
                    err=ScopeError::in_the_middle;
                }
                return ret;
            };

        
            //if name is begun with ::, it's means global scope
            if(r.expect("::")){
                return global_search();
            }

            //next search in current scope and using namespace
            Scope* ret=get_require_kind_scope(now_scope,ScopeKind::package);
            assert(ret);
            
            size_t endpos=0;
            for(auto* s:search->using_scope){
                auto tmp=search_scope_in_scope(r,s);
                if(tmp&&fincheck()){
                    if(ret){
                        err=ScopeError::match_some_name;
                        return nullptr;
                    }
                    endpos=r.readpos();
                    ret=tmp;
                }
                r.seek(0);
            }
            if(!ret){
                return global_search();
            }
            r.seek(endpos);
            if(!fincheck()){
                err=ScopeError::in_the_middle;
            }
            return ret;
        }

        Scope* create_scope_in_scope(PROJECT_NAME::Reader<Str>& r,Scope* sc){
            auto beginpos=r.readpos();
            Str id=Str();
            r.readwhile(id,PROJECT_NAME::until,':');
            if(id.size()==0){
                r.seek(beginpos);
                return nullptr;
            }
            auto ret=&sc->child_scope->operator[](id);
            ret->name=id;
            return r.ceof()?ret:create_scope_in_scope(r,ret);
        }

        Scope* new_scope(PROJECT_NAME::Reader<Str> r,Scope* hint){
            if(r.expect("::")){
                return create_scope_in_scope(r,&global_scope);
            }
            bool err=false;
            auto ret=resolve_scope(r,err,false);
            if(err)return nullptr;
            if(ret){
                return create_scope_in_scope(r,ret);
            }
            if(r.ceof()){
                return nullptr;
            }

        }

        bool enter_namespace(const Str& name){
            assert(now_scope);
            auto& ref=now_scope->child_scope[name];
            if(ref.kind==ScopeKind::unset){
                ref.kind=ScopeKind::package;
            }
            if(ref.kind!=ScopeKind::package){
                return false;
            }
            ref.name=name;
            ref.parent=now_scope;
            now_scope=&ref;
            return true;
        }


        bool enter_function(const Str& name){
            assert(now_scope);
            auto& ref=now_scope->child_scope["func "+name];
            if(ref.kind!=ScopeKind::unset){
                return false;
            }
            ret.name=name;
            ref.kind=ScopeKind::function;
            ref.parent=now_scope;
            now_scope=&ref;
            return true;
        }

        Type* voidt(){
            return &void_ty;
        }

        Type* boolt(){
            return &bool_ty;
        }

        Type* intt(size_t size){
            return &int_ty[size];
        }

        Type* uintt(size_t size){
            return &uint_ty[size];
        }

        Type* floatt(){
            return &f32_ty;
        }

        Type* doublet(){
            return &f64_ty;
        }

        bool is_not_derivable(TypeKind kind){
            return kind==TypeKind::not_null_pointer||kind==TypeKind::temporary;
        }

        void derived_check(Type*& in){
            assert(in);
            while(is_not_derivable(in->kind)){
                assert(in);
                in=in->base;
            }
            assert(in);
        }

        Type* pointer_to(Type* in){
            derived_check(in);
            if(!in->derived.pointer){
                in->derived.pointer=count;
                count++;
                Type& ret=derived_ty[in->derived.pointer];
                ret.base=in;
                ret.kind=TypeKind::pointer;
                return &ret;
            }
            return &derived_ty[in->derived.pointer];
        }

        Type* reference_of(Type* in){
            if(is_not_derivable(in)){
                return nullptr;
            }
            if(!in->derived.not_null){
                in->derived.not_null=count;
                count++;
                Type& ret=derived_ty[in->derived.not_null];
                ret.base=in;
                ret.kind=TypeKind::not_null_pointer;
                return &ret;
            }
            return &derived_ty[in->derived.not_null];
        }

        Type* array_of(Type* in,node::Node<Str>* size){
            assert(in);
            if(is_not_derivable(in->kind)){
                return nullptr;
            }
            assert(size);
            size_t pos=0;
            if(size->kind==node::NodeKind::obj_int){
                size_t n=0;
                try{
                   n=std::stoull(size->symbol);
                }
                catch(...){
                    delete size;
                    return nullptr;
                }
                if(n==0){
                    return nullptr;
                }
                auto& ref=in->derived.array_n[n];
                if(!ref){
                    ref=count;
                    count++;
                    Type& ret=derived_ty[ref];
                    ret.base=in;
                    ret.kind=TypeKind::array_n;
                    ret.size_n=n;
                    return &ret;
                }
                pos=ref;
            }
            else{
                auto& ref=in->derived.array_p[size];
                if(!ref){
                    ref=count;
                    count++;
                    Type& ret=derived_ty[ref];
                    ret.base=in;
                    ret.kind=TypeKind::array_p;
                    ret.size_p=size;
                    return &ret;
                }
                pos=ref;
            }
            return &derived_ty[pos];
        }

        Type* temporary_of(Type* in){
            derived_check(in);
            if(!in->derived.temporary){
                in->derived.temporary=count;
                count++;
                Type& ret=derived_ty[in->derived.temporary];
                ret.base=in;
                ret.kind=TypeKind::temporary;
                return &ret;
            }
            return &derived_ty[in->derived.temporary];
        }   

        Type* struct_(const Str& name){
            PROJECT_NAME::Reader<Str> r(name);
            r.expect("::")
        }

        Type* generic(){
            assert(now_scope);
            Type ty;
            ty.kind=TypeKind::generic;
            now_scope->generic_ty.push_back(std::move(ty));
            return &now_scope->generic_ty.back();
        }
    };

    template<class Buf,class Str>
    bool read_id(bool scope,Str& sig,PROJECT_NAME::Reader<Buf>& r){
        if(scope){
            if(r.expect("::")){
                sig+="::";
            }
        }
        bool first=true;
        do{ 
            if(!PROJECT_NAME::is_c_id_top_usable(r.achar())){
                return false;
            }
            if(!first){
                sig+="::";
            }
            first=false;
            auto prev=sig.size();
            r.readwhile(sig,PROJECT_NAME::is_c_id_usable<char>);
            if(prev==sig.size()){
                return false;
            }
        }while(scope&&r.expect("::"));
        return true;
    }

    template<class Tree,class Buf,class Str>
    bool read_attr_signature(Attribute<Str>& attr,node::Error<Tree,Str>& err,PROJECT_NAME::Reader<Buf>& r,TypePlace mode){
        auto sigreturn=[&](auto in){
            attr.attrflag=enum_or(attr.attrflag,in);
            return read_attr_signature(attr,r,mode);
        };
        auto bad=[&](auto f){
            return enum_and(attr.attrflag,f);
        };
        if(r.expect("const",PROJECT_NAME::is_c_id_usable)){
            if(bad(TypeAttribute::constant)){
                err="type:const attribute is allready added";
                return false;
            }
            return sigreturn(TypeAttribute::constant);
        }
        else if(r.expect("atomic",PROJECT_NAME::is_c_id_usable)){
            if(bad(TypeAttribute::atomic)){
                err="type:atomic attribute is allready added";
                return false;
            }
            return sigreturn(TypeAttribute::atomic);
        }
        else if(r.expect("align",PROJECT_NAME::is_c_id_usable)){
            if(mode!=TypePlace::in_struct){
                err="type:align attribute is usable in struct or union but not";
                return false;
            }
            if(bad(TypeAttribute::align)){
                err="type:align attribute is allready added";
                return false;
            }
            if(!r.expect("(")){
                err="type:align:expected ( but not";
                return false;
            }
            err=node::expr<Buf,Tree,Str>(r);
            if(!err)return false;
            if(!r.expect(")")){
                delete err.release();
                err="type:align:expected ) but not";
                return false;
            }
            attr.align_size=err.release();
            return sigreturn(TypeAttribute::align);     
        }
        else if(r.expect("thread_local",PROJECT_NAME::is_c_id_usable)){
            if(mode!=TypePlace::in_func){
                err="type:thread_local attribute is usable in function but not";
                return false;
            }
            if(bad(TypeAttribute::thread)){
                err="type:thread_local attribute is allready added";
                return false;
            }
            return sigreturn(TypeAttribute::thread);
        }
        else if(r.expect("local",PROJECT_NAME::is_c_id_usable)){
            if(mode==TypePlace::arg){
                err="type:local attribute is not usable on argument";
                return false;
            }
            if(bad(TypeAttribute::local)){
                err="type:local attribute is allready added";
                return false;
            }
            return sigreturn(TypeAttribute::local);
        }
        else if(r.expect("global",PROJECT_NAME::is_c_id_usable)){
            if(mode==TypePlace::arg){
                err="type:global attribute is not usable on argument";
                return false;
            }
            if(bad(TypeAttribute::global)){
                err="type:global attribute is allready added";
                return false;
            }
            return sigreturn(TypeAttribute::global);
        }
        else if(r.expect("@")){
            if(attr.optional_attr){
                err="type:optional attribute is already added";
                return false;
            }
            if(!r.ahead("(")){
                err="type:optional_attr:expected ( but not";
                return false;
            }
            err=expr<Buf,Tree,Str>(r);
            if(err)return false;
            if(err.ret.kind!=node::NodeKind::op_call||err.ret.right){
                delete err.release();
                err="type:invalid optional attribute signature";
                return false;
            }
            return read_attr_signature(attr,err,r,mode);
        }
        return true;
    }

    template<class Buf,class Tree,class Str>
    Type<Str>* read_type_signature(node::Error<Tree,Str>& err,TypePlace mode,PROJECT_NAME::Reader<Buf>& r,TypePool<Str>& pool){
        auto sigreturn=[&](auto in){
            sig+=in;
            return read_signature(sig,r);
        };
        auto sandwhich=[&](auto begin,auto end){
            sig+=begin;
            r.readwhile(sig,PROJECT_NAME::until,end[0]);
            if(!r.expect(end)){
                return false;
            }
            sig+=end;
            return read_signature(sig,r);
        };
        Type<Str>* ret=nullptr;
        if(r.expect("*")){
            ret=read_type_signature(err,mode,r,pool);
            return ret?pool.pointer_to(ret):nullptr;
        }
        else if(r.expect("&&")){
            ret=read_type_signature(err,mode,r,pool);
            return ret?pool.temporary_of(ret):nullptr;
        }
        else if(r.expect("&")){
            ret=read_type_signature(err,mode,r,pool);
            return ret?pool.reference_of(ret):nullptr;   
        }
        else if(r.expect("[")){
            err=node::expr<Buf,Tree,Str>(r);
            if(!err)return nullptr;
            if(!r.expect("]")){
                err="type:array:expected ] but not";
                return nullptr;
            }
            ret=read_type_signature(err,mode,r,pool);
            return ret?pool.array_of(ret,err.release()):nullptr;
        }
        else if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
            return read_func_sig(sig,r,false);
        }
        else if(r.expect("struct",PROJECT_NAME::is_c_id_usable)){
            return read_struct_sig(sig,r);
        }
        else if(r.expect("interface",PROJECT_NAME::is_c_id_usable)){
            return read_interface_sig(sig,r);
        }
        else if(r.expect("union",PROJECT_NAME::is_c_id_usable)){
            return read_struct_sig(sig,r);
        }
        else if(r.expect("enum",PROJECT_NAME::is_c_id_usable)){
            return read_enum_sig(sig,r);
        }
        else if(read_type_keyword(ret,r,pool)){
            return ret;
        }   
        else if(PROJECT_NAME::is_c_id_top_usable(r.achar())||r.achar()==':'){
            
        }
        else if(mode==TypePlace::arg){
            return pool.generic();
        }
    }

    template<class Buf,class Tree,class Str>
    bool read_type_keyword(Type<Str>*& ret,PROJECT_NAME::Reader<Buf>& r,TypePool<Str>& pool){
        auto e=[&](auto in,int biufv,size_t s){
            if(r.expect(in,PROJECT_NAME::is_c_id_usable)){
            switch(biufv){
            case 1:ret=pool.boolt();break;
            case 2:ret=pool.intt(s);break;
            case 3:ret=pool.uintt(s);break;
            case 4:ret=pool.floatt();break;
            case 5:ret=pool.doublet();break;
            case 6:ret=pool.voidt();break;
            default:return false;
            }
            return true;
            }
            return false;
        };
        return e("int",2,32)||e("uint",3,32)||e("bool",1,0)||e("float",4)||e("double",5)||e("void",6,0)||
               e("int8",2,8)||e("int16",2,16)||e("int32",2,32)||e("int64",2,64)||
               e("uint8",3,8)||e("uint16",3,16)||e("uint32",3,32)||e("uint64",3,64)
    }

    template<class Buf,class Tree,class Str>
    bool read_func_sig(node::Error<Tree,Str>& err,PROJECT_NAME::Reader<Buf>& r,FuncSigMode mode,TypePool<Str>& pool){
        auto flag=[&](auto in){return (bool)enum_and(mode,in)};
        if(r.expect("(")){
            while(true){
                if(r.expect(")")){
                    break;
                }
                bool need=true;
                Object<Str> obj;
                if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                    size_t beginpos=r.readpos();
                    if(read_id(false,obj.name,r)){
                        if(r.achar()==','||r.achar()==')'){
                            if(flag(FuncSigMode::has_type)){
                               r.seek(beginpos); 
                            }
                        }
                    }
                }
                auto ret=read_type_signature(err,mode,r,pool);
                if(!ret)return nullptr;
                
                if(!r.expect(",")&&!r.ahead(")")){
                    return false;
                }
            }
        }
        else{
            assert(!flag(FuncSigMode::need_bracket));
        }
        if(flag(FuncSigMode::has_ret)&&r.expect("->")){
            //sig+="->";
            if(r.ahead("(")){
                return read_func_sig(err,r,enum_or(FuncSigMode::need_bracket,FuncSigMode::has_type));
            }
            else{
                return read_type_signature(err,TypePlace::arg,r,pool);
            }
        }
        return true;
    }

    template<class Buf,class Str>
    bool read_maybe_named_func_sig(Str& sig,PROJECT_NAME::Reader<Buf>& r,bool must){
        if(must||PROJECT_NAME::is_c_id_top_usable(r.achar())){
            if(!read_id(false,sig,r)){
                return false;
            }
        }
        return read_func_sig(sig,r,true,false);
    }

    template<class Buf,class Str>
    bool read_struct_sig(Str& sig,PROJECT_NAME::Reader<Buf>& r){
        if(!r.expect("{"))return false;
        sig+="{";
        while(!r.eof()){
            if(r.expect("}")){
                sig+="}";
                return true;
            }
            if(r.expect("func",PROJECT_NAME::is_c_id_usable)){
                if(!read_maybe_named_func_sig(sig,r,true))return false;
                if(r.ahead("{")){
                    sig+="{";
                    if(!r.readwhile(sig,PROJECT_NAME::depthblock_withbuf,PROJECT_NAME::BasicBlock(true,false,false))){
                        return false;
                    }
                    sig+="} ";
                }
                else if(r.expect(";")){
                    sig+="; ";
                }
                else{
                    return false;
                }
            }
            else{
                if(!read_id(false,sig,r))return false;
                sig+=" ";
                if(!read_signature(sig,r))return false;
                if(!r.expect(";"))return false;
                sig+="; ";
            }
        }
        return false;
    }

    template<class Buf,class Str>
    bool read_interface_sig(Str& sig,PROJECT_NAME::Reader<Buf>& r){
        if(!r.expect("{"))return false;
        sig+="{";
        while(!r.eof()){
            if(r.expect("}")){
                sig+="}";
                return true;
            }
            if(!read_id(false,sig,r))return false;
            if(!r.expect("("))return false;
            if(!read_func_sig(sig,r,true))return false;
            if(!r.expect(";"))return false;
            sig+="; ";
        }
        return false;
    }

    template<class Buf,class Str>
    bool read_enum_sig(Str& sig,PROJECT_NAME::Reader<Buf>& r){
        if(!r.expect("{"))return false;
        sig+="{";
        if(!r.eof()){
            if(r.expect("}")){
                sig+="}";
                return true;
            }
            if(!read_id(false,sig,r))return false;
            if(r.expect("=")){
                sig+="=";
                r.read_while(sig,PROJECT_NAME::untilincondition,[](char c){
                    return c!=','&&c!='}';
                });
            }
            if(r.ahead("}")){
                continue;
            }
            if(!r.expect(","))return false;
            if(r.ahead('}')){
                continue;
            }
            sig+=", ";
        }
        return false;
    }
}