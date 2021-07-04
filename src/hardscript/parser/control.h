/*license*/
#pragma once
#include<json_util.h>
#include"operators.h"
#include<list>

namespace control{
    /*enum class TreeKind {
        //symbols
        unknown,
        create,
        boolean,
        integer,
        floats,
        string, 
        closure,
        
        //cotrol
        ctrl_for,
        ctrl_select,
        ctrl_switch,
        ctrl_if,

        def_var,
        def_func,
        dec_func
    };*/

    
    struct Tree{
        Tree* left=nullptr;
        Tree* right=nullptr;
        std::string symbol;
        node::NodeKind kind=node::NodeKind::unknown;
        std::list<Tree*> arg;
        ~Tree(){
            delete left;
            delete right;
            for(auto p:arg){
                delete p;
            }
        }
        void to_json(PROJECT_NAME::JSON& j)const;
    };
    
    
    

    inline Tree* expr(PROJECT_NAME::Reader<std::string>& reader){
        return node::example_expr_parse<Tree>(reader,8);
    }

    /*
    template<class T>
    inline T* make(){
        try{
            return new T();
        }
        catch(...){
            return nullptr;
        }
    }

    template<class T,class... Arg>
    inline T* make(Arg... arg){
        try{
            return new T(arg...);
        }
        catch(...){
            return nullptr;
        }
    }*/


    enum class CtrlKind{
        ctrl,
        func,
        decl,
        var,
        expr
    };

    struct Control{
    private:
        bool moving(Control& from){
            if(expr!=from.expr){
                delete expr;
            }
            expr=from.expr;
            from.expr=nullptr;
            kind=from.kind;
            name=std::move(from.name);
            inblock=std::move(from.inblock);
            type=std::move(from.type);
            arg=std::move(from.arg);
            argtype=std::move(from.argtype);
            inblockpos=from.inblockpos;
            from.inblockpos=0;
            return true;
        }
    public:
        //CtrlKind kind=CtrlKind::unknown;
        CtrlKind kind=CtrlKind::ctrl;
        Tree* expr=nullptr;
        std::string name;
        std::string inblock;
        std::string type;
        std::vector<std::string> arg;
        std::vector<std::string> argtype;
        size_t inblockpos=0;
        Control(const std::string name="",Tree* expr=nullptr,CtrlKind kind=CtrlKind::ctrl):
        name(name),kind(kind),expr(expr){}
        Control(const Control&)=delete;
        Control(Control&& from) noexcept{
            moving(from);
        }

        Control& operator=(Control&& from){
            moving(from);
            return *this;
        }
        ~Control(){
            delete expr;
        }

        void to_json(PROJECT_NAME::JSON& j)const;
    };

    using ReaderT=PROJECT_NAME::Reader<std::string>;
    using NodesT=std::vector<Control>;

    bool parse_all(ReaderT& reader,NodesT& vec);

    bool check_initexpr(ReaderT& reader);

    bool control_parse(ReaderT& reader,NodesT& vec);

    bool read_inblock(ReaderT& reader,std::string& buf,size_t& pos);

    bool parse_inblock(ReaderT& reader,std::string& buf,size_t& pos);

    bool parse_expr(ReaderT& reader,NodesT& vec,bool semicolon);

    bool parse_var(ReaderT& reader,NodesT& vec);

    bool parse_var_detail(ReaderT& reader,NodesT& vec,const char* bracket,bool initeq);

    bool parse_if(ReaderT& reader,NodesT& vec);

    bool parse_func(ReaderT& reader,NodesT& vec,bool decl=false);

    bool parse_for(ReaderT& reader,NodesT& vec);

    bool parse_switch(ReaderT& reader,NodesT& vec);
}
