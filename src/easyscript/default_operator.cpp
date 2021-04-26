#include"default_operator.h"
#include<cfenv>
#pragma STDC FENV_ACCESS ON

auto default_op::err(const std::string& msg)->ValType {return {msg,EvalType::error}; }

bool default_op::is_compareop(const std::string& op){
    return op=="=="||op=="!="||op==">"||op=="<"||op==">="||op=="<=";
}

ValType default_op::interpret_default(const std::string& op,ValType& leftval,ValType& rightval){
    if(leftval.second==EvalType::none){
        leftval={"0",EvalType::integer};
    }
    if(op=="*"&&leftval.second==EvalType::str&&leftval.second==EvalType::integer){
        return str_op(op,leftval,rightval);
    }
    if(op=="&&"||op=="||"){
        auto left=cast_val(EvalType::boolean,leftval);
        if(left.second==EvalType::error)return left;
        auto right=cast_val(EvalType::boolean,rightval);
        if(right.second==EvalType::error)return right;
        return boolean_op(op,left,right);
    }
    if(op=="!"){
        auto right=cast_val(EvalType::boolean,rightval);
        if(right.second==EvalType::error)return right;
        return boolean_op(op,leftval,right);
    }
    auto t=cast_each(leftval,rightval);
    if(t==1)return rightval;
    else if(t==-1)return leftval;
    if(rightval.second==EvalType::boolean){
        return boolean_op(op,leftval,rightval);
    }
    else if(rightval.second==EvalType::integer){
        return integer_op(op,leftval,rightval);
    }
    else if(rightval.second==EvalType::floats){
        return float_op(op,leftval,rightval);
    }
    else if(rightval.second==EvalType::str){
        return str_op(op,leftval,rightval);
    }
    else{
        return err("unexpected evaltype.");
    }
}



ValType default_op::boolean_op(const std::string& op,ValType& leftval,ValType& rightval){
    auto set=[](bool& res,const auto& str){
        if(str=="true"){
            res=true;
        }
        else if(str=="false"){
            res=false;
        }
        else{
            return false;
        }
        return true;
    };
    bool left=false,right=false;
    if(!set(right,rightval.first)){
        return err("not bool:"+rightval.first);
    }
    bool res=false;
    if(op=="!"){
        res=!right;
        return {res?"true":"false",EvalType::boolean};
    }
    if(!set(left,leftval.first)){
        return err("not bool:"+leftval.first);
    }
    if(op=="+"){
        res=left||right;
    }
    else if(op=="*"){
        res=left&&right;
    }
    else if(op=="||"){
        res=left||right;
    }
    else if(op=="&&"){
        res=left&&right;
    }
    else if(is_compareop(op)){
        return compare_op(op,left,right);
    }
    else{
        return err("not supported operator:"+op);
    }
    return {res?"true":"false",EvalType::boolean};
}

ValType default_op::integer_op(const std::string& op,ValType& leftval,ValType& rightval){
    long long left=0;
    long long right=0;
    if(!stoi(leftval.first,left)){
        return err("not integer:"+leftval.first);
    }
    if(!stoi(rightval.first,right)){
        return err("not integer:"+rightval.first);
    }
    long long res=0;
    if(op=="+"){
        res=left+right;
    }
    else if(op=="-"){
        res=left-right;
    }
    else if(op=="*"){
        res=left*right;
    }
    else if(op=="/"||op=="%"){
        if(right==0)return err("division by zero");
        auto dived=div(left,right);
        if(op=="/"){
            res=dived.quot;
        }
        else{
            res=dived.rem;
        }
    }
    else if(op=="++"){
        res=right+1;
    }
    else if(op=="--"){
        res=right-1;
    }
    else if(is_compareop(op)){
        return compare_op(op,left,right);
    }
    else{
        return err("not supported operator:"+op);
    }
    return {std::to_string(res),EvalType::integer};
}

ValType default_op::float_op(const std::string& op,ValType& leftval,ValType& rightval){
    double left=0;
    double right=0;
    if(!float_cast(left,leftval.first)){
        return err("not float:"+leftval.first);
    }
    if(float_cast(right,rightval.first)){
        return err("not float:"+rightval.first);
    }
    feclearexcept(FE_ALL_EXCEPT);
    auto res=0.0;
    if(op=="+"){
        res=left+right;
    }
    else if(op=="-"){
        res=left-right;
    }
    else if(op=="*"){
        res=left-right;
    }
    else if(op=="/"){
        res=left/right;
    }
    else if(is_compareop(op)){
        return compare_op(op,left,right);
    }
    else{
        return err("not supported operator:"+op);
    }
    if(fetestexcept(FE_ALL_EXCEPT)){
        return err("floating point arithmetic exception.");
    }
    return {std::to_string(res),EvalType::floats};
}

ValType default_op::str_op(const std::string& op,ValType& leftval,ValType& rightval){
    auto ok=[](auto c){return c=='"'||c=='\'';};
    auto left=leftval.first;
    auto right=rightval.first;
    if(!ok(left.front())||!ok(left.back())){
            return err("not str:"+left);
    }
    left.pop_back();
    left.erase(0,1);
    std::string res;
    if(op=="*"&&rightval.second==EvalType::integer){
        long long count=0;
        if(!stoi(right,count)){
            return err("not int:"+right);
        }
        if(count<0){
            return err("unexpected number:"+std::to_string(count));
        }
        res+="\'";
        for(auto i=0ll;i<count;i++){
            res+=right;
        }
        res+="'";
        return {res,EvalType::str};
    }
    if(!ok(right.front())||!ok(right.back())){
        return err("not str:"+right);
    }
    right.pop_back();
    right.erase(0,1);
    if(op=="+"){
        res="'"+left+right+"'";
    }
    else if(is_compareop(op)){
        return compare_op(op,left,right);
    }
    else{
        return err("not supported operator:"+op);
    }
    return {res,EvalType::str};
}

int default_op::cast_each(ValType& leftval,ValType& rightval){
    if(leftval.second>rightval.second){
        rightval=cast_val(leftval.second,rightval);
        if(rightval.second==EvalType::error)return -1;
    }
    else if(leftval.second<rightval.second){
        leftval=cast_val(rightval.second,leftval);
        if(leftval.second==EvalType::error)return 1;
    }
    return 0;
}

ValType default_op::cast_val(EvalType type,ValType& val){
    if(val.second==type){
        ValType ret;
        ret=val;
        return ret;
    }
    if(type==EvalType::boolean){
        bool res=false;
        if(val.second==EvalType::str){
            res=!(val.first=="\"\""||val.first=="''");
        }
        else if(val.second==EvalType::integer){
            if(!stoi(val.first,res)){
                return err("not number");
            }
        }
        else if(val.second==EvalType::floats){
            if(!float_cast(res,val.first)){
                return err("not float number");
            }
        }
        else{
            return err("unexpected type.");
        }
        return {res?"true":"false",EvalType::boolean};
    }
    else if(type==EvalType::integer){
        long long res=0;
        if(val.second==EvalType::boolean){
            if(val.first=="true")res=1;
            else if(val.first=="false")res=0;
            else{
                return err("unexpected value:"+val.first);
            }
        }
        else if(val.second==EvalType::floats){
            if(!float_cast(res,val.first)){
                return err("not float number");
            }
        }
        else if(val.second==EvalType::str){
            auto tmp=val.first;
            tmp.pop_back();
            tmp.erase(0,1);
            if(!stoi(tmp,res)){
                return err("not cast str to int:"+tmp);
            }
        }
        else{
            return err("unexpected type.");
        }
        return {std::to_string(res),EvalType::integer};
    }
    else if(type==EvalType::floats){
        double res=0;
        if(val.second==EvalType::boolean){
            if(val.first=="true")res=1;
            else if(val.first=="false")res=0;
            else{
                return err("unexpected value:"+val.first);
            }
        }
        else if(val.second==EvalType::integer){
            if(!stoi(val.first,res)){
                return err("not int:"+val.first);
            }
        }
        else if(val.second==EvalType::str){
            auto tmp=val.first;
            tmp.pop_back();
            tmp.erase(0,1);
            if(!float_cast(res,tmp)){
                return err("not cast str to float:"+tmp);
            }
        }
        else{
            return err("unexpected type.");
        }
        return {std::to_string(res),EvalType::floats};
    }
    else if(type==EvalType::str){
        return {"'"+val.first+"'",EvalType::str};
    }
    else{
        return err("unexpected type");
    }
}