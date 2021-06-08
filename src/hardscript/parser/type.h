#pragma once
#include<reader.h>
#include<basic_helper.h>
#include<vector>
#include<map>
#include<assert.h>
namespace type{
    enum class TypeKind{
        alias,
        primary,
        pointer,
        not_null_pointer,
        temporary,
        array,
        structure,
        unions,
        enums,
        interface,
        generic,
        function,
    };
    
    enum class TypeAttribute{
        noattr=0x0,
        constant=0x1, 
        thread=0x2,
        atomic=0x4,
        allign=0x8,

        global=0x10,
        local=0x20,
    };

    enum class ScopeKind{
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

    

    template<class Str>
    struct Member{
        std::map<Str,Type<Str>*> member;
    };

    template<class Str>
    struct Func{
        std::vector<Type<Str>*> arg;
    };

    template<class Str>
    struct Derived{
        size_t pointer=0;
        size_t not_null=0;
        size_t temporary=0;
        std::map<size_t,size_t> array;
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
            Member<Str>* memb=nullptr;
            Func<Str>* func;
            //Generic<Str>* gener;
            size_t size;
        };
        Derived<Str> derived;
    };

    template<class Str>
    struct Scope{
        ScopeKind kind=ScopeKind::package;
        Str name=Str();
        std::map<Str,Type<Str>> struct_ty;
        std::map<Str,Type<Str>> interface_ty;
        std::map<Str,Type<Str>> generic_ty;
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
        Scope* namespace_scope=&global_scope;
        Scope* function_scope=nullptr;
        Scope* block_scope=nullptr;

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
            Scope* ret=nullptr;
            ret=search_scope_in_scope(r,namespace_scope);
            if(!fincheck()){
                ret=nullptr;
            }
            r.seek(0);
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
            PROJECT_NAME::Reader<Str> r(name);
            bool err=false;
            auto scope=resolve_namespace(r,err,true);
            if(err==ScopeError::in_the_middle||err==ScopeError::not_found){
                if(!scope)return false;
                scope=new_scope(r,scope);
                if(!scope)return false;   
            }
            scope->prev=namespace_scope;
            namespace_scope=scope;
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

        Type* array_of(Type* in,size_t size){
            if(is_not_derivable(in->kind)){
                return nullptr;
            }
            auto& ref=in->derived.array[size];
            if(!ref){
                ref=count;
                count++;
                Type& ret=derived_ty[ref];
                ret.base=in;
                ret.kind=TypeKind::array;
                ret.size=size;
                return &ret;
            }
            return &derived_ty[ref];
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
            r.readwhile(sig,PROJECT_NAME::is_c_id_usable);
            if(prev==sig.size()){
                return false;
            }
        }while(scope&&r.expect("::"));
        return true;
    }

    template<class Buf,class Str>
    bool read_signature(Str& sig,PROJECT_NAME::Reader<Buf>& r){
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
        if(r.expect("*")){
            return sigreturn("*");
        }
        else if(r.expect("&&")){
            return sigreturn("&&");
        }
        else if(r.expect("&")){
            return sigreturn("&");
        }
        else if(r.expect("[")){
            return sandwhich("[","]");
        }
        else if(r.expect("(")){
            return read_func_signature(sig,r);
        }
        else if(r.expect("struct",PROJECT_NAME::is_c_id_usable)){

        }
        else if(r.expect("interface",PROJECT_NAME::is_c_id_usable)){
            
        }
        else if(r.expect("union",PROJECT_NAME::is_c_id_usable)){

        }
        else if(r.expect("enum",PROJECT_NAME::is_c_id_usable)){

        }
        else if(r.expect("const",PROJECT_NAME::is_c_id_usable)){
            return sigreturn("const ");
        }
        else if(r.expect("allign",PROJECT_NAME::is_c_id_usable)){
            sig+="allign";
            if(!r.expect("("))return false;
            return sandwhich("(",")");     
        }
        else if(r.expect("thread_local",PROJECT_NAME::is_c_id_usable)){
            return sigreturn("thread_local ");
        }
        else if(r.expect("atomic",PROJECT_NAME::is_c_id_usable)){
            return sigreturn("atomic ");
        }
        else if(r.expect("local",PROJECT_NAME::is_c_id_usable)){
            return sigreturn("local ");
        }
        else if(r.expect("global",PROJECT_NAME::is_c_id_usable)){
            return sigreturn("global ");
        }
        else if(r.expect("@")){
            sig+="@";
            if(!r.expect("("))return false;
            return sigreturn("(",")");
        }
        else if(PROJECT_NAME::is_c_id_top_usable(r.achar())||r.achar()==':'){
            return read_id(true,sig,r);
        }
        else{
            return true;
        }
        return false;
    }   

    template<class Buf,class Str>
    bool read_func_signature(Str& sig,PROJECT_NAME::Reader<Buf>& r){
        while(true){
            if(r.expect(")")){
                sig+=")";
                break;
            }
            bool need=true;
            if(PROJECT_NAME::is_c_id_top_usable(r.achar())){
                if(read_id(false,sig,r)){
                    sig+=" ";
                    if(r.achar()==','||r.achar()==')'){
                        need=false;
                    }
                }
            }
            if(need){
                if(!read_signature(sig,r)){
                    return false;
                }
            }
            if(!r.expect(",")&&!r.ahead(")")){
                return false;
            }
        }
        if(r.expect("->")){
            sig+="->";
            return read_signature(sig,r);
        }
        return true;
    }

    template<class Buf,class Str>
    bool read_struct_sig(Str& sig,PROJECT_NAME::Reader<Buf>& r){
        if(!r.expect("{"))return false;
        sig+="{";
        while(true){
            if(r.expect("}")){
                return false;
            }
        }
    }
}