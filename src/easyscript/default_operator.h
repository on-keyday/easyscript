/*license*/
#pragma once
#include"parse.h"

namespace default_op{

    auto err(const std::string& msg)->ValType;

    ValType interpret_default(const std::string& op,ValType& leftval,ValType& rightval);

    ValType boolean_op(const std::string& op,ValType& leftval,ValType& rightval);

    ValType integer_op(const std::string& op,ValType& leftval,ValType& rightval);

    ValType float_op(const std::string& op,ValType& leftval,ValType& rightval);

    ValType str_op(const std::string& op,ValType& leftval,ValType& rightval);

    int cast_each(ValType& leftval,ValType& rightval);

    ValType cast_val(EvalType type,ValType& val);

    template<class T>
    auto stod(T& s,double& res)->bool{
        try{
            res=std::stod(s);
            return true;
        }
        catch(...){
            return false;
        }
    }

    template<class T,class U>
    auto stoi(T& s,U& res)->bool{
        try{
            res=(PROJECT_NAME::remove_cv_ref<decltype(res)>)std::stoll(s);
            return true;
        }
        catch(...){
            return false;
        }
    }

    template<class T,class U>
    auto float_cast(T& res,U& s)->bool{
        double d=0;
        if(!stod(s,d)){
            return false;
        }
        res=(PROJECT_NAME::remove_cv_ref<decltype(res)>)d;
        return true;
    }

    template<class Func,class T>
    ValType to_boolfunc(Func& func,T& left,T&right){
        if(func(left,right)){
            return {"true",EvalType::boolean};
        }
        else{
            return {"false",EvalType::boolean};
        }
    }

    template<class T>
    ValType compare_op(const std::string& op,T& left,T& right){
        if(op==">"){
            auto f=[](auto& a,auto& b){return a>b;};
            return to_boolfunc(f,left,right);
        }
        else if(op=="<"){
            auto f=[](auto& a,auto& b){return a<b;};
            return to_boolfunc(f,left,right);
        }
        else if(op==">="){
            auto f=[](auto& a,auto& b){return a>=b;};
            return to_boolfunc(f,left,right);
        }
        else if(op=="<="){
            auto f=[](auto& a,auto& b){return a<=b;};
            return to_boolfunc(f,left,right);
        }
        else if(op=="=="){
            auto f=[](auto& a,auto& b){return a==b;};
            return to_boolfunc(f,left,right);
        }
        else if(op=="!="){
            auto f=[](auto& a,auto& b){return a!=b;};
            return to_boolfunc(f,left,right);
        }
        return err("not compare operator.");
    }
    bool is_compareop(const std::string& op);
}