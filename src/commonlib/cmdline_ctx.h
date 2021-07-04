#pragma once
#include"project_name.h"
#include"reader.h"
#include"utfreader.h"
#include"basic_helper.h"
#include<memory>

namespace PROJECT_NAME{

    enum class OptFlag{
        none=0,
        only_one=0x1,
        hidden_from_child=0x2,
    };

#define DEFINE_ENUMOP(TYPE)\
    TYPE operator&(TYPE a,TYPE b){\
        using basety=std::underlying_type_t<TYPE>;\
        return static_cast<TYPE>((basety)a & (basety)b);\
    }\
    TYPE operator|(TYPE a,TYPE b){\
        using basety=std::underlying_type_t<TYPE>;\
        return static_cast<TYPE>((basety)a | (basety)b);\
    }\
    TYPE operator~(TYPE a){\
        using basety=std::underlying_type_t<TYPE>;\
        return static_cast<TYPE>(~(basety)a);\
    }\
    bool any(TYPE a){\
        return a!=TYPE::none;\
    }

    DEFINE_ENUMOP(OptFlag)

    template<class Str>
    struct OptionInfo{
        using string_type=Str;
        string_type name;
        string_type helpstr;
        size_t argcount=0;
        size_t mustcount=0;
        //bool only_one=false;
        OptFlag flag=OptFlag::none;
    };

    template<class Buf,class Str,template<class ...>class Map,template<class ...>class Vec,class User>
    struct CommandCtx;

    template<class Buf,class Str,template<class ...>class Map,template<class ...>class Vec,class User>
    struct CmdlineCtx;

    enum class LogFlag{
        none=0,
        program_name=0x1,
    };

    DEFINE_ENUMOP(LogFlag);

    template<class Buf,class Str,template<class ...>class Map,template<class ...>class Vec,class User>
    struct ArgCtx{
        friend CommandCtx<Buf,Str,Map,Vec,User>;
        friend CmdlineCtx<Buf,Str,Map,Vec,User>;
        using string_type=Str;
        template<class Value>
        using vec_type=Vec<Value>;
        template<class Key,class Value>
        using map_type=Map<Key,Value>;
        using cmdline_type=CmdlineCtx<Buf,Str,Map,Vec,User>;
        using command_type=CommandCtx<Buf,Str,Map,Vec,User>;
        //string_type cmdname;
    private:
        vec_type<string_type> arg;
        CommandCtx<Buf,Str,Map,Vec,User>* basectx;
        map_type<string_type,vec_type<string_type>> options;
        std::unique_ptr<ArgCtx> child=nullptr;
        ArgCtx* parent=nullptr;
        LogFlag logmode=LogFlag::program_name; 
    public:
        const vec_type<string_type>* exists_option(const string_type& optname,bool all=false)const{
            const ArgCtx* c=this; 
            do{
                if(c->options.count(optname)){
                    return std::addressof(c->options.at(optname));
                }
                c=c->parent;
            }while(all&&c);
            return nullptr;
        }

        bool get_optarg(string_type& val,const string_type& optname,size_t index,bool all=false)const{
            if(auto opts=exists_option(optname,all)){
                auto& opt=(*opts);
                if(opt.size()<=index)return false;
                val=opt[index];
                return true;
            }
            return false;
        }

        bool cmpname(const string_type& str)const{
            if(!basectx)return false;
            return basectx->cmdname==str;
        }

        User* get_user(bool root=false)const{
            if(!basectx)return nullptr;
            //return root?basectx->get_root_user():std::addressof(basectx->user);
            return basectx->get_root_user();
        }

        size_t arglen()const{
            return arg.size();
        }

        bool get_arg(size_t index,string_type& out)const{
            if(index>=arg.size())return false;
            out=arg[index];
            return true;
        }

        auto begin()const{
            return arg.begin();
        }

        auto end()const{
            return arg.end();
        }

        const ArgCtx* get_parent()const{
            return parent;
        }

        cmdline_type* get_cmdline()const{
            return basectx?basectx->basectx:nullptr;
        }
        
        command_type* get_this_command()const{
            return basectx;
        }

        void set_logmode(LogFlag flag){
            logmode=flag;
        }

    private:

        template<class Obj>
        void printlog_impl(Obj&)const{}

        template<class Obj,class First,class... Args>
        void printlog_impl(Obj& obj,First first,Args... args)const{
            obj<<first;
            return printlog_impl(obj,args...);
        }

        template<class Obj>
        void print_cmdname(Obj& obj,command_type* ctx)const{
            if(!ctx)return;
            if(ctx->parent){
                print_cmdname(obj,ctx->parent);
            }
            else{
                if(!ctx->basectx)return;
                ctx->print_program_name(obj);
            }
            obj << ctx->cmdname;
            obj << ":";
            return;
        }
    public:
        template<class Obj,class... Args>
        void log(Obj& obj,Args... args)const{
            if(!basectx)return;
            if(any(logmode&LogFlag::program_name))print_cmdname(obj,basectx);
            printlog_impl(obj,args...,"\n");
        }        
    };

    enum class CmdFlag{
        none=0,
        invoke_parent=0x1,
        use_parentopt=0x2,
    };

    DEFINE_ENUMOP(CmdFlag)

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


    template<class Buf,class Str,template<class ...>class Map,template<class ...>class Vec,class User>
    struct CommandCtx{
    private:
        friend CmdlineCtx<Buf,Str,Map,Vec,User>;
        friend ArgCtx<Buf,Str,Map,Vec,User>;
        using string_type=Str;
        using reader_type=Reader<Buf>;
        using arg_type=ArgCtx<Buf,Str,Map,Vec,User>;
        using cmdline_type=CmdlineCtx<Buf,Str,Map,Vec,User>;
        //using args_type=Args<string_type>;
        using param_callback=void(*)(string_type&);
        using param_self_callback=bool(*)(reader_type&,CommandCtx&,arg_type&);
        using command_callback=int(*)(const arg_type&,int prev,int pos);
        template<class Key,class Value>
        using map_type=Map<Key,Value>;
        
        cmdline_type* basectx=nullptr;

        CommandCtx* parent=nullptr;
        string_type cmdname;
        string_type help_str;
        param_callback param_proc=nullptr;
        param_self_callback self_proc=nullptr;
        command_callback command_proc=nullptr;
        map_type<string_type,std::shared_ptr<OptionInfo<string_type>>> options;
        map_type<string_type,CommandCtx> subcommands;
        //User user=User();
        //bool invoke_parent=false;
        //bool use_parentopt=false;
        CmdFlag flag=CmdFlag::none;

        template<class Obj>
        void print_program_name(Obj& obj)const{
            if(!basectx)return;
            if(basectx->program_name.size()){
                obj << basectx->program_name;
                obj << ":";
            }
        }

        User* get_root_user()const{
            return basectx?std::addressof(basectx->user):nullptr;
        }
    };

    struct Optname{
        const char* longname=nullptr;
        const char* shortname=nullptr;
        bool arg=false;
        OptFlag flag=OptFlag::none;
        const char* help=nullptr;
    };

    template<class Buf,class Str,template<class ...>class Map,template<class ...>class Vec,class User=void*>
    struct CmdlineCtx{
        using string_type=Str;
        template<class Key,class Value>
        using map_type=Map<Key,Value>;
        using reader_type=Reader<Buf>;
        using arg_type=ArgCtx<Buf,Str,Map,Vec,User>;
        using command_type=CommandCtx<Buf,Str,Map,Vec,User>;
        template<class Value>
        using vec_type=Vec<Value>;

        using param_callback=typename command_type::param_callback;
        using input_func=bool(*)(reader_type&);
        using command_callback=typename command_type::command_callback;
        using param_self_callback=typename command_type::param_self_callback;
        
        using error_callback=void(*)(const string_type&,const char*,const string_type&,const char*);

    private:
        friend CommandCtx<Buf,Str,Map,Vec,User>;
        User user;
        param_callback param_proc=nullptr;
        reader_type r;
        
        map_type<string_type,command_type> _command;
        string_type program_name;
        
        error_callback errcb=nullptr;

        vec_type<command_callback> rootcb;

        map_type<string_type,std::shared_ptr<OptionInfo<string_type>>> rootopt;

        void error(const char* err1,const string_type& msg=string_type(),const char* err2=""){
            if(errcb)errcb(program_name,err1,msg,err2);
        }

        template<class C>
        static bool get_a_line(Reader<ArgVWrapper<C>>& r,string_type& arg){
            auto c=(const C*)r.achar();
            if(!c)return false;
            r.increment();
            arg=c;
            return true;
        }

        template<class Bufi>
        static bool get_a_line(Reader<Bufi>& r,string_type& arg){
            return get_cmdline(r,1,true,arg);
        }

        bool get_a_param(string_type& arg,command_type* ctx=nullptr){
            if(!get_a_line(r,arg))return false;
            for(command_type* c=ctx;c;c=c->parent){
                if(c->param_proc){
                    c->param_proc(arg);
                    return true;
                }
            }
            if(param_proc){
                param_proc(arg);
            }
            return true;
        }

        bool parse_a_option(const string_type& str,command_type& ctx,arg_type& out){
            std::shared_ptr<OptionInfo<string_type>> opt;
            bool ok=false;
            bool on_parent=false;
            for(command_type* c=&ctx;c;c=c->parent){
                if(c->options.count(str)){
                    decltype(opt)& ref=c->options[str];
                    if(!on_parent||any(ref->flag&~OptFlag::hidden_from_child)){
                        opt=ref;
                        ok=true;
                        break;
                    }
                    
                }
                if(any(c->flag&CmdFlag::use_parentopt)){
                    break;
                }
                on_parent=true;
            }
            if(!ok){
                if(rootopt.count(str)){
                    opt=rootopt[str];
                }
                else{
                    error("unknown option '",str,"'");
                    return false;
                }
            }
            if(any(opt->flag&OptFlag::only_one)){
                if(out.options.count(opt->name)){
                    error("option ",str," is already set");
                    return false;
                }
            }
            vec_type<string_type> optarg;
            size_t count=0;
            while(count<opt->argcount){
                auto prevpos=r.readpos();
                string_type arg;
                if(!get_a_param(arg,&ctx)||arg[0]=='-'){
                    if(count<opt->mustcount){
                        error("option ",str," needs more argument");
                        return false;
                    }
                    r.seek(prevpos);
                    break;
                }
                optarg.push_back(std::move(arg));
            }
            out.options[opt->name]=std::move(optarg);
            return true;
        }

        bool option_parse(const string_type& str,command_type& ctx,arg_type& out){
            if(str[0]=='-')return parse_a_option(str,ctx,out);
            for(auto& c:str){
                if(!parse_a_option(string_type(1,c),ctx,out))return false;
            }
            return true;
        }

        bool parse_default(command_type& ctx,arg_type& base,arg_type*& fin){
            while(!r.eof()){
                string_type str;
                if(!get_a_param(str,&ctx)){
                    error("param get failed");
                    return false;
                }
                if(str[0]=='-'){
                    str.erase(0,1);
                    if(!option_parse(str,ctx,base))return false;
                }
                else if(ctx.subcommands.count(str)){
                    command_type& sub=ctx.subcommands[str];
                    base.child.reset(new arg_type());
                    base.child->parent=&base;
                    base.child->logmode=base.logmode;
                    return parse_by_context(sub,*base.child,fin);
                }
                else{
                    base.arg.push_back(str);
                }
            }
            fin=&base;
            return true;
        }

        bool parse_by_context(command_type& ctx,arg_type& base,arg_type*& fin){
            base.basectx=&ctx;
            if(ctx.self_proc){
                fin=&base;
                return ctx.self_proc(r,ctx,base);
            }
            else{
                return parse_default(ctx,base,fin);
            }
        }


        bool parse(arg_type& ctx,arg_type*& fin){
            Str str;
            if(!get_a_param(str))return false;
            if(param_proc)param_proc(str);
            return parse_with_name(str,ctx,fin);
        }

        bool parse_with_name(const string_type& name,arg_type& ctx,arg_type*& fin){
            if(!_command.count(name))return false;
            return parse_by_context(_command[name],ctx,fin);
        }

        bool input(){
            //r=reader_type(string_type(in),ignore_space);
            return (bool)r.readable();
        }

        int run(const arg_type& arg){
            if(!arg.basectx){
                error("library is broken");
                return -1;
            }
            int res=-1;
            int pos=0;
            bool called=false;
            for(command_type* c=arg.basectx;c;c=c->parent){
                if(c->command_proc){
                    res=c->command_proc(arg,res,pos);
                    called=true;
                    pos++;
                    if(any(c->flag&CmdFlag::invoke_parent)){
                        continue;
                    }
                    break;
                }
            }
            if(!called){
                error("any action is not registered");
                return -1;
            }
            for(auto cb:rootcb){
                cb(arg,res,pos);
                pos++;
            }
            return res;
        }

        command_type* register_command_impl(command_type& cmd,const string_type& name,
        command_callback proc,const string_type& help=string_type(),
        CmdFlag flag=CmdFlag::none){
            cmd.cmdname=name;
            cmd.command_proc=proc;
            cmd.help_str=help;
            cmd.basectx=this;
            //cmd.use_parentopt=use_parentopt;
            //cmd.invoke_parent=invoke_parent;
            cmd.flag=flag;
            return &cmd;
        }

        template<class Input>
        bool input_common(Input in){
            //if(!in)return false;
            if(!in(r)){
                error("input failed");
                return false;
            }
            if(!input()){
                error("input failed");
                return false;
            }
            return true;
        }

    public:
        void register_error(error_callback cb){
            errcb=cb;
        }

        [[nodiscard]]
        command_type* register_command(const string_type& name,command_callback proc,
        const string_type& help=string_type()){
            if(!proc||_command.count(name))return nullptr;
            command_type& cmd=_command[name];
            return register_command_impl(cmd,name,proc,help,CmdFlag::none);
        }

        [[nodiscard]]
        command_type* register_subcommand(command_type* cmd,const string_type& name,
        command_callback proc,const string_type& help=string_type(),
        CmdFlag flag=CmdFlag::none){
            if(!cmd)return nullptr;
            if(cmd->subcommands.count(name))return nullptr;
            auto& sub=cmd->subcommands[name];
            return register_command_impl(sub,name,proc,help,flag);
        }

        [[nodiscard]]
        command_type* get_command(const string_type& name){
            return _command.count(name)?&_command[name]:nullptr;
        }

        [[nodiscard]]
        command_type* get_subcommand(command_type* cmd,const string_type& name){
            return cmd&&cmd->subcommands.count(name)?&cmd->subcommands[name]:nullptr;
        }

        bool remove_command(const string_type& name){
            return (bool)_command.erase(name);
        }

        bool remove_subcommand(command_type* cmd,const string_type& name){
            return cmd?(bool)cmd->subcommands.erase(name):false;
        }


        bool register_root_callback(command_callback proc){
            if(!proc)return false;
            rootcb.push_back(proc);
            return true;
        }

        bool register_callbacks(command_type* cmd,param_callback param,param_self_callback self=nullptr){
            if(!cmd)return false;
            cmd->param_proc=param;
            cmd->self_proc=self;
            return true;
        }

        bool register_paramproc(param_callback param){
            param_proc=param;
            return true;
        }

        std::shared_ptr<OptionInfo<string_type>> register_option(command_type* cmd,const string_type& name,
                size_t arg=0,size_t must=0,OptFlag flag=OptFlag::none,const string_type& help=string_type()){
            auto set_info=[&](auto& info){
                info.reset(new OptionInfo<string_type>);
                info->name=name;
                info->argcount=arg;
                info->mustcount=must;
                //info->only_one=only_one;
                info->flag=flag;
                info->helpstr=help;
                return info;
            };
            if(!cmd){
                if(rootopt.count(name))return nullptr;
                return set_info(rootopt[name]);
            }
            else{
                if(cmd->options.count(name))return nullptr;
                return set_info(cmd->options[name]);
            }
        }

        bool register_option_alias(command_type* cmd,std::shared_ptr<OptionInfo<string_type>>& info,const string_type& name){
            if(!cmd){
                if(rootopt.count(name)){
                    return false;
                }
                rootopt[name]=info;
            }
            else{
                if(cmd->options.count(name))return false;
                cmd->options[name]=info;
            }
            return true;
        }

        bool register_by_optname(command_type* cmd,vec_type<Optname> info){
            for(Optname& i:info){
                int arg=i.arg?1:0;
                if(auto opt=register_option(cmd,i.longname,arg,arg,i.flag,i.help?i.help:"");
                   !opt||(i.shortname&&!register_option_alias(cmd,opt,i.shortname))){
                    return false;
                }
            }
            return true;
        }

        
        void register_program_name(const string_type& name){
            program_name=name;
        }
        
        template<class input_func=CmdlineCtx::input_func>
        int execute(input_func in,LogFlag logmode=LogFlag::program_name){
            if(!input_common(in))return -1;
            arg_type arg,*fin=nullptr;
            arg.set_logmode(logmode);
            if(!parse(arg,fin))return -1;
            return run(*fin);
        }

        template<class input_func=CmdlineCtx::input_func>
        int execute_with_name(const string_type& name,input_func in,LogFlag logmode=LogFlag::program_name){
            if(!input_common(in))return -1;
            arg_type arg,*fin=nullptr;
            arg.set_logmode(logmode);
            if(!parse_with_name(name,arg,fin))return -1;
            return run(*fin);
        }

        template<class input_func=CmdlineCtx::input_func>
        int execute_with_context(command_type* ctx,input_func in,LogFlag logmode=LogFlag::program_name){
            if(!ctx)return -1;
            if(!input_common(in))return -1;
            arg_type arg,*fin=nullptr;
            arg.set_logmode(logmode);
            if(!parse_by_context(*ctx,arg,fin))return -1;
            return run(*fin);
        }

        User& get_user(){
            return user;
        }
    };

    template<class C,class Str,template<class ...>class Map,template<class ...>class Vec,class User=void*>
    using ArgVCtx=CmdlineCtx<ArgVWrapper<C>,Str,Map,Vec,User>;
}