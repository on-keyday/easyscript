#pragma once
#include"project_name.h"
#include"reader.h"
#include"basic_helper.h"
#include<memory>
#include<iterator>

//temporary
#include<string>
#include<vector>
#include<map>
#include<assert.h>

namespace PROJECT_NAME{

    template<class C>
    struct ArgVWrapper{
        C** argv=nullptr;
        int argc=0;
        uintptr_t operator[](size_t pos)const{
            if(pos>=argc)return 0;
            return (uintptr_t)argv[pos];
        }
        size_t size()const{
            return (size_t)argc;
        }

        ArgVWrapper(int argc,C** argv):argc(argc),argv(argv){}

        ArgVWrapper(){}

        ArgVWrapper(const ArgVWrapper& in):argv(in.argv),argc(in.argc){  

        }

        ArgVWrapper(ArgVWrapper&& in):argv(in.argv),argc(in.argc){  
            in.argv=nullptr;
            in.argc=0;
        }

        ArgVWrapper& operator=(const ArgVWrapper& in){
            argv=in.argv;
            argc=in.argc;
            return *this;
        }

        ArgVWrapper& operator=(ArgVWrapper&& in)noexcept{
            argv=in.argv;
            argc=in.argc;
            in.argv=nullptr;
            in.argc=0;
            return *this;
        }
    };

    enum class CommandFlag{
        none=0,
        root=0x1,
        set_parentopt=0x2,
        get_parentopt=0x4,
        get_parentarg=0x8,
        invoke_parent=0x10,
        ignore_unknown_option=0x20,
        no_onechar_opts=0x40,
        must_be_flag_if_two_optprefix=0x80,
        ignored_option_is_arg=0x100,
    };

    DEFINE_ENUMOP(CommandFlag)

    enum class OptionFlag{
        none=0,
        hidden_from_child=0x1,
        only_one=0x2,
    };

    DEFINE_ENUMOP(OptionFlag)

    enum class CallbackFlag{
        failed=0,
        thru=1,
        succeed=2,
        parent=3,
    };

    enum class DerivedCbFlag{
        none=0,
        read_inheritance=0x1,
        param_inheritance=0x2,
        option_inheritance=0x4,

        effect_child=0x100,
        protect_from_parent=0x200,
    };

    DEFINE_ENUMOP(DerivedCbFlag)


    enum class ErrorFlag{
        input=0x1,
        user_callback=0x2,
        option=0x4,
        registered_config=0x8,
        system=0x10,
        param=0x20,
    };

    DEFINE_ENUMOP(ErrorFlag)

    enum class TVL{
        truth,
        falsehood,
        error,
    };

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Arg;

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct SetArg;

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Command;

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    using command_callback=int(*)(const Arg<Buf,Str,Vec,Map,MultiMap>&,int,int);

    template<class Buf,class Str>
    using param_read_callback=CallbackFlag(*)(Reader<Buf>&,Str&);

    template<class Str>
    using param_process_callback=CallbackFlag(*)(Str&);

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    using option_parse_callback=CallbackFlag(*)(const Str&,const Command<Buf,Str,Vec,Map,MultiMap>&,SetArg<Buf,Str,Vec,Map,MultiMap>&);

    template<class T>
    using SafePtr=std::shared_ptr<T>;

    template<class Str>
    struct Option{
        Str name;
        size_t argcount=0;
        size_t mustcount=0;
        OptionFlag flag=OptionFlag::none;
    };

    template<class T,class K,class Map>
    T* mapfind(const K& k,Map& in){
        if(auto it=in.find(k);it!=in.end()){
            return &(*it).second;
        }
        return nullptr;
    }

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Parser;

    #define for_parents(cmdp) for(auto* c=cmdp;c;c=c->parent)

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Command{
        using string=Str;
        using buffer=Buf;

        template<class T>
        using vector=Vec<T>;
        template<class Key,class Value>
        using map=Map<Key,Value>;
        template<class Key,class Value>
        using multimap=MultiMap<Key,Value>;

        using option=Option<string>;

        using optionptr=SafePtr<option>;

        using param_callback=param_process_callback<string>;
        using read_callback=param_read_callback<buffer,string>;
        using option_callback=option_parse_callback<buffer,string,vector,map,multimap>;
        using command_callback=command_callback<buffer,string,vector,map,multimap>;

        friend Parser<buffer,string,vector,map,multimap>;

        b_char_type<string> optflag='-';

        string name;
        
        command_callback command=nullptr;

        param_callback paramcb=nullptr;
        read_callback readcb=nullptr;
        option_callback optioncb=nullptr;

        DerivedCbFlag cbflag=DerivedCbFlag::none;

        CommandFlag flag=CommandFlag::none;

        Command* parent=nullptr;

        map<string,Command> subcommands;

        map<string,optionptr> options;

        multimap<option*,string> reverse_resolver;

        Command(const string& name,command_callback cb,CommandFlag flag){
            this->name=name;
            this->command=cb;
            this->flag=flag;
        }

        

        void set_cbflag(DerivedCbFlag current){
            DerivedCbFlag parent=this->parent?this->parent->cbflag:DerivedCbFlag::none;
            if(!any(current&DerivedCbFlag::protect_from_parent)&&
                any(parent&DerivedCbFlag::effect_child)){
                    current|=parent;
                    current&=~DerivedCbFlag::protect_from_parent;
            }
            cbflag=current;
        }

        Command* get_subcommand(const string& name)const{
            return mapfind<Command>(name,subcommands);
        }

        Command* make_subcommand(const string& name,command_callback cb,CommandFlag flag,DerivedCbFlag cbflag){
            if(get_subcommand(name)){
                return nullptr;
            }
            auto& ret=subcommands[name];
            ret.parent=this;
            ret.command=cb;
            ret.flag=flag;
            ret.set_cbflag(cbflag);
            return &ret;
        }

        option* get_option(const string& name,const Command** foundat=nullptr,bool all=true)const{
            for_parents(this){
                if(auto ret=mapfind<optionptr>(name,c->options)){
                    auto p=(*ret).get();
                    if(!any(p->flag&OptionFlag::hidden_from_child)){
                        if(foundat){
                            *foundat=c;
                        }
                        return p;    
                    }
                }
                if(all&&any(c->flag&CommandFlag::set_parentopt)){
                    continue;
                }
                break;
            }
            return nullptr;
        }

        option* make_option(const string& name,OptionFlag flag,size_t argnum){
            if(get_option(name))return nullptr;
            auto& opt=options[name];
            opt.reset(new option());
            opt->flag=flag;
            opt->argcount=argnum;
            option* ret=opt.get();
            reverse_resolver.emplace(ret,name);
            return ret;
        }

        bool add_option_alias(option* opt,const string& name){
            string* key=mapfind<string>(opt,reverse_resolver);
            if(!key){
                return false;
            }
            if(get_option(name)){
                return false;
            }
            options[name]=options[*key];
            reverse_resolver.emplace(opt,name);
            return true;
        }
    
        void set_flag(CommandFlag flag){
            this->flag=flag;
        }
        
        void set_callbacks(param_callback param,read_callback read,option_callback option,
                            DerivedCbFlag flag=DerivedCbFlag::none){
            this->paramcb=param;
            this->readcb=read;
            this->optioncb=option;
            set_cbflag(flag);
        }
    };

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Arg{
    private:
        friend SetArg;

        using command=Command<Buf,Str,Vec,Map,MultiMap>;
        
        using string=command::string;

        template<class Key,class Value>
        using map=command::map<Key,Value>;

        template<class T>
        using vector=command::vector<T>;

        vector<map<string,vector<string>>> options;

        map<string,vector<string>>* currentopt=nullptr;

        vector<vector<string>> arg;

        vector<string>* currentarg=nullptr;

        command* base=nullptr;

        Arg(){}
        Arg(const Arg&)=delete;
        Arg(Arg&&)=delete;
    public:
        ~Arg(){}

        command* get_command()const{return base;}

        auto& get_args()const{
            assert(currentarg);
            return *currentarg;
        }

        auto& get_opts()const{
            assert(currentopt);
            return *currentopt;
        }
    private:
        vector<string>* findopt_detail(const string& name,size_t pos,size_t* foundat)const{
            if(vector<string>* found=mapfind<vector<string>>(name,options[pos])){
                if(foundat){
                    *foundat=pos;
                }
                return found;
            }
            return nullptr;
        }

        constexpr static size_t faild=static_cast<size_t>(-1);
    public:
        vector<string>* exists_option_from_child(const string& name,bool all=true,size_t* foundat=nullptr)const{
            assert(options.size());
            size_t pos=options.size()-1;
            for_parents(base){
                assert(pos!=faild);
                if(auto found=findopt_detail(name,pos,foundat)){
                    return found;
                }
                if(all&&any(c->flag&CommandFlag::get_parentopt)){
                    pos--;
                    continue;
                }
                break;
            }
            return nullptr;
        }
    private:
        vector<string>* exists_option_from_parent_detail(command* cmd,bool all,size_t pos,const string& name,size_t* foundat=nullptr)const{
            assert(cmd);
            assert(pos!=faild);
            if(all&&cmd->parent&&any(cmd->flag&CommandFlag::get_parentopt)){
                if(auto found=exists_option_from_parent_detail(cmd->parent,true,pos-1,name,foundat)){
                    return found;
                }
            }
            if(auto found=findopt_detail(name,pos,foundat)){
                return found;
            }
            return nullptr;
        }
    public:
        vector<string>* exists_option_from_parent(const string& name,bool all=false,size_t* foundat=nullptr)const{
            return exists_option_from_parent_detail(base,all,options.size()-1,name,foundat);
        }

        bool get_optarg(string& val,const string& name,size_t pos,bool all=true)const{
            size_t foundat=0;
            if(!exists_option_from_parent(name,all,&foundat)){
                return false;
            }
            size_t s=options.size();
            size_t sum=0;
            for(auto i=foundat;i<s;i++){
                if(auto exists=mapfind<vector<string>>(name,options)){
                    size_t size=exists->size();
                    if(sum+size>pos){
                        val=exists->at(pos-sum);
                        return true;
                    }
                    sum+=size;
                }
            }
            return false;
        }

        vector<string> get_optargs(const string& name,bool all=false)const{
            vector<string> ret;
            size_t foundat=0;
            if(!exists_option_from_parent(name,all,&foundat)){
                return ret;
            }
            size_t s=options.size();
            for(auto i=foundat;i<s;i++){
                if(auto exists=mapfind<vector<string>>(name,options)){
                    if(!exists->size())continue;
                    ret.reserve(ret.size()+exists->size());
                    std::copy(exists->begin(),exists->end(),std::back_inserter(options));
                }
            }
            return ret;
        }
    private:
        bool get_arg_detail(string& val,command* cmd,size_t idx,size_t pos,bool all,size_t& sum)const{
            assert(cmd);
            assert(idx!=faild);
            if(all&&cmd->parent&&any(cmd->flag&CommandFlag::get_parentarg)){
                if(get_arg_detail(val,cmd->parent,idx-1,pos,true,sum)){
                    return true;
                }
            }
            auto& ref=arg[idx];
            size_t s=ref.size();
            if(sum+s>pos){
                val=ref[pos];
                return true;
            }
            sum+=s;
            return false;
        }

        void get_args_detail(vector<string>& ret,size_t idx,command* cmd,bool all)const{
            assert(idx!=faild);
            if(all&&cmd->parent&&any(cmd->flag&CommandFlag::get_parentarg)){
                get_args_detail(ret,idx-1,cmd->parent,true);
            }
            std::copy(arg[idx].begin(),arg[idx].end(),std::back_inserter(ret));
        }
    public:
        bool get_arg(string& val,size_t pos,bool all=true)const{
            size_t sum=0;
            return get_arg_detail(val,base,arg.size()-1,pos,all,sum);
        }

        vector<string> get_args(bool all=false)const{
            vector<string> ret;
            get_args_detail(ret,arg.size()-1,base,all);
            return ret;
        }
    };

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct SetArg{
    private:
        using arg=Arg<Str,Vec,Map,MultiMap>;
        using command=arg::command;

        using string=command::string;

        arg base;
    public:
        SetArg(command* cmd){
            incr_current(cmd);
        }

        template<class Vec>
        void incr_vec(Vec& vec){
            vec.resize(vec.size()+1);
        }

        bool incr_current(command* cmd){
            assert(cmd);
            if(cmd==base.base)return false;
            incr_vec(base.arg);
            incr_vec(base.options);
            base.currentarg=&base.arg[base.arg.size()-1];
            base.currentopt=&base.options[base.arg.size()-1];
            base.base=cmd;
            return true;
        }

        void add_arg(string&& str){
            assert(base.currentarg);
            base.currentarg->push_back(std::move(str));
        }

        auto& add_opt(string&& str){
            assert(base.currentopt);
            return base.currentopt->operator[](std::move(str));
        }

        arg& get_arg(){
            return base;
        }
    };

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Parser{
        using command=Command<Str,Vec,Map,MultiMap>;
        using string=command::string;
        Reader<Buf> r;
        using setarg=SetArg;
        command root;

        Parser(const string& name,command_callback cb):root(name,cb,CommandFlag::root){}

        command* get_root()const{return &root;}

        template<class C>
        static bool get_a_param(string& arg,Reader<ArgVWrapper<C>>& r){
            auto c=(const C*)r.achar();
            if(!c)return false;
            r.increment();
            arg=c;
            return true;
        }

        template<class Buf>
        static bool get_a_param(string& arg,Reader<Buf>& r){
            return get_cmdline(r,1,true,true,arg);
        }

        bool error(ErrorFlag flag,const char* err1,const string& msg=string(),const char* err2="",const char* meta=""){
            return false;
        }

        bool param_get(string& arg,command& cmd){
            for_parents(&cmd){
                if(c->readcb){
                    CallbackFlag res=c->readcb(r,arg);
                    if(res==CallbackFlag::failed){
                        error(ErrorFlag::param|ErrorFlag::user_callback,"user defined param callback failed");
                        return false;
                    }
                    else if(res==CallbackFlag::succeed){
                        return true;
                    }
                    else if(res==CallbackFlag::parent){
                        continue;
                    }
                    break;
                }
                if(c->cbflag&DerivedCbFlag::read_inheritance){
                    continue;
                }
                break;
            }
            if(!get_a_param(arg,r)){
                error(ErrorFlag::param,"param get failed");
                return false;
            }
            return true;
        }

        bool process_param(string& arg,command& cmd){
            for_parents(&cmd){
                if(c->paramcb){
                    CallbackFlag res=c->paramcb(arg);
                    if(res==CallbackFlag::failed){
                        error(ErrorFlag::param|ErrorFlag::user_callback,"user defined process callback failed");
                        return false;
                    }
                    else if(res==CallbackFlag::succeed){
                        return true;
                    }
                    else if(res==CallbackFlag::parent){
                        continue;
                    }
                    break;
                }
                if(c->cbflag&DerivedCbFlag::param_inheritance){
                    continue;
                }
                break;
            }
            return true;
        }

        bool get_and_proc(string& arg,command& cmd){
            return param_get(arg,cmd)&&process_param(arg,cmd);
        }

        TVL call_userdefinedopt(const string& name,Command& cmd,setarg& arg){
            bool called=false;
            for_parents(&cmd){
                if(c->optioncb){
                    CallbackFlag res=c->optioncb(name,cmd,arg);
                    called=true;
                    if(res==CallbackFlag::failed){
                        error(ErrorFlag::option|ErrorFlag::user_callback,"user defined option callback failed");
                        return TVL::error;
                    }
                    else if(res==CallbackFlag::succeed){
                        return TVL::truth;
                    }
                    else if(res==CallbackFlag::parent){
                        if(!c->parent)return TVL::falsehood;
                        continue;
                    }
                    return TVL::falsehood;
                }
                if(c->cbflag&DerivedCbFlag::option_inheritance){
                    continue;
                }
                break;
            }
            return called?TVL::truth:TVL::falsehood;
        }

        TVL find_option(const string& name,Command& cmd,setarg& arg){
            if(auto* opt=cmd.get_option(name)){
                if(any(opt->flag&OptionFlag::only_one)&&arg.get_arg().exists_option_from_child(name)){
                    error(ErrorFlag::option,"option '",name,"' is already set","OptionFlag::only_one");
                    return TVL::error;
                }
                auto& optarg=arg.add_opt(name);
                for(auto i=0;i<opt->argcount;i++){
                    if(r.ceof()||r.achar()==cmd.optflag){
                        if(i<opt->mustcount){
                            error(ErrorFlag::option,"option '",name,"' needs more argument","Option::mustcount");
                            return TVL::error;
                        }
                    }
                    string val;
                    if(!get_and_proc(val,cmd)){
                        return TVL::error;
                    }
                    optarg.push_back(std::move(val));
                }
                return TVL::truth;
            }
            return TVL::falsehood;
        }

        TVL get_a_option(string& name,Command& cmd,setarg& arg){
           TVL res=call_userdefinedopt(name,cmd,arg);
           if(res!=TVL::falsehood)return res;
           res=find_option(name,cmd,arg);
           if(res!=TVL::falsehood)return res;
           auto flag=[&](auto i){return any(cmd.flag&i);};
           auto err=[&](auto& in,bool toarg=false){
                if(flag(CommandFlag::ignore_unknown_option)){
                    if(!toarg&&!flag(CommandFlag::ignored_option_is_arg)){
                        error(ErrorFlag::option,"unknown option '",in,"' (ignored)","CommandFlag::ignore_unknown_option");
                    }
                    return TVL::falsehood;
                }
                else{
                    error(ErrorFlag::option,"unknown option '",in,"'");
                    return TVL::error;
                }
           };
           if(flag(CommandFlag::no_onechar_opts)){
                return err(name,true);
           }
           if(flag(CommandFlag::must_be_flag_if_two_optprefix)&&name[0]==cmd.optflag){
                return err(name,true);
           }
           for(auto i:name){
                string topt(1,i);
                res=call_userdefinedopt(topt,cmd,arg);
                if(res==TVL::error)return res;
                if(res==TVL::truth)continue;
                res=find_option(topt,cmd,arg);
                if(res==TVL::error){
                    return res;
                }
                if(res==TVL::falsehood){
                    res=err(topt);
                    if(res==TVL::error)return res;
                }
           }
           return TVL::truth;
        }

        bool parse_detail(command& cmd,setarg& arg){
            while(!r.eof()){
                string param;
                if(!get_and_proc(param,cmd))return false;
                if(auto sub=cmd.get_subcommand(param)){
                    arg.incr_current(sub);
                    return parse_detail(*sub,arg);
                }
                else if(r.achar()==cmd.optflag){
                    auto ignore_add=[&]{
                        if(any(cmd.flag&CommandFlag::ignored_option_is_arg)){
                            arg.add_arg(std::move(param));
                            return true;
                        }
                        return false;
                    };
                    if(param.size()==1){
                        if(!ignore_add()){
                            error(ErrorFlag::param|ErrorFlag::option,"not a option value");
                            return false;
                        }
                    }
                    else{
                        string copy=param;
                        param.erase(0,1);
                        auto res=get_a_option(param,cmd,arg);
                        if(res==TVL::error)return false;
                        if(res==TVL::falsehood){
                            param=copy;
                            ignore_add();
                        }
                    }
                }
                else{
                    arg.add_arg(std::move(param));
                }
            }
            return true;
        }

        bool parse(setarg& arg){
            if(!r.readable()){
                error(ErrorFlag::input,"no input exists");
                return false;
            }
            return parse_detail(root,arg);
        }
    };

    template<class Buf,class Str,template<class ...>class Vec,template<class ...>class Map,template<class ...>class MultiMap>
    struct Executor{
        using parser=Parser<Buf,Str,Vec,Map,MultiMap>;
        using command=parser::command;
        parser& parse;
        using setarg=parser::setarg;
        int execute(){
            setarg setter(parse.get_root());
            if(!parse.parse(setter))return -1;
            return run(setter);
        }

        int run(setarg& setter){
            command* cmd=setter.get_command();
            int prev=-1;
            int pos=0;
            bool called=false;
            for_parents(cmd){
                if(cmd->command){
                    prev=cmd->command(setter.get_arg(),prev,pos);
                    called=true;
                    pos++;
                    if(any(cmd->flag&CommandFlag::invoke_parent)){
                        continue;
                    }
                    break;
                }
            }
            return prev;
        }
    };
}
