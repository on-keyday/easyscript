#pragma once
#include"reader.h"
#include<string>
#include<unordered_map>
#include<vector>
#include<cstring>


namespace PROJECT_NAME{
    enum class JSONType{
        unset,
        null,
        boolean,
        integer,
        unsignedi,
        floats,
        string,
        object,
        array
    };

    enum class JSONFormat{
        space=0x1,
        tab=0x2,
        mustline=0x4,
        elmpush=0x8,
        afterspace=0x10,
        mustspace=0x20,
        endline=0x40,
        must=mustline|mustspace,
        defaultf=space|afterspace|endline,
        tabnormal=tab|afterspace|endline,
        shift=space|elmpush|afterspace,
        spacemust=space|mustspace,
        noendline=space|afterspace,
        noendlinetab=tab|afterspace
    };

    
    struct EasyStr{
    private:
        char* str=nullptr;
        size_t len=0;
        void construct(const char* from,size_t fromlen){
            try{
                str=new char[fromlen+1];
                len=fromlen;
            }
            catch(...){
                str=nullptr;
                len=0;
            }
            memmove(str,from,fromlen);
            str[len]=0;
        }
    public:
        EasyStr(){
            str=nullptr;
            len=0;
        }

        EasyStr(const char* from,size_t fromlen){
            construct(from,fromlen);
        }

        EasyStr(const char* from){
            construct(from,strlen(from));
        }

        EasyStr(const EasyStr&)=delete;
        EasyStr(EasyStr&& from){
            str=from.str;
            len=from.len;
            from.str=nullptr;
            from.len=0;
        }
        EasyStr& operator=(EasyStr&& from){
            if(str!=nullptr&&str==from.str)return *this;
            delete[] str;
            str=from.str;
            len=from.len;
            from.str=nullptr;
            from.len=0;
            return *this;
        }

        char& operator[](size_t pos){
            if(pos>len)return str[len];
            return str[pos];
        }

        const char* const_str()const{return str;}
        size_t size()const{return len;}

        ~EasyStr(){
            delete[] str;
            str=nullptr;
            len=0;
        }
    };

    struct JSON{
        using JSONObjectType=std::unordered_map<std::string,JSON>;
        using JSONArrayType=std::vector<JSON>;
    private:
        
        union{
            bool boolean;
            long long numi;
            unsigned long long numu;
            double numf;
            EasyStr value=EasyStr();
            JSONObjectType* obj;
            JSONArrayType* array;
        };
        JSONType type=JSONType::unset;

        template<class T>
        T* init(){
            try{
                return new T();
            }
            catch(...){
                return nullptr;
            }
        }

        bool init_as_obj(JSONObjectType&& json){
            obj=init<JSONObjectType>();
            if(!obj){
                type=JSONType::unset;
                return false;
            }
            type=JSONType::object;
            *obj=std::move(json);
            return true;
        }

        bool init_as_array(JSONArrayType&& json){
            array=init<JSONArrayType>();
            if(!array){
                type=JSONType::unset;
                return false;
            }
            type=JSONType::array;
            *array=std::move(json);
            return true;
        }

        template<class Num>
        bool init_as_num(Num num){
            type=JSONType::integer;
            numi=num;
            return true;
        }

        void destruct();

        JSON& move_assign(JSON&& from);

        JSON& copy_assign(const JSON& from);

        JSON parse_json_detail(Reader<const char*>& reader);

        std::string to_string_detail(JSONFormat format,size_t ofs,size_t indent,size_t base_skip) const;

        std::string escape(const std::string& base);

        template<class Struct>
        auto to_json_detail(JSON& to,const Struct& in)
        ->decltype(to_json(to,in)){
            to_json(to,in);
        }

        template<class Struct>
        auto to_json_detail(JSON& to,const Struct& in) 
        ->decltype(in.to_json(to)) {
            in.to_json(to);
        }


        template<class Vec>
        auto to_json_detail(JSON& to,const Vec& vec)
        ->decltype(to_json_detail(to,vec[0])){
            JSON js;
            for(auto& o:vec){
                js.push_back(JSON(o));
            }
            to=std::move(js);
        }

        template<class Map>
        auto to_json_detail(JSON& to,const Map& m)
        ->decltype(to_json_detail(to,m[std::string()])){
            JSON js;
            for(auto& o:m){
                js[o.first]=JSON(o.second);
            }
            to=std::move(js);
        }


        void obj_check(bool exist,const std::string& in)const{
            if(type!=JSONType::object)throw "object kind miss match.";
            if(exist&&!obj->count(in))throw "invalid range";
        }

        void array_check(bool check,size_t pos)const{
            if(type!=JSONType::array)
                throw "object kind miss match.";
            if(check&&array->size()<=pos)
                throw "invalid index";
        }

        void array_if_unset(){
            if(type==JSONType::unset){
                *this=JSONArrayType{};
            }
        }

        void obj_if_unset(){
            if(type==JSONType::unset){
                *this=JSONObjectType{};
            }
        }

    public:

        JSON& operator=(const JSON& from){
            if(this==&from)return *this;
            destruct();
            return copy_assign(from);
        }

        JSON& operator=(JSON&& from) noexcept {
            if(this==&from)return *this;
            destruct();
            return move_assign(std::move(from));
        }

        JSON(){
            type=JSONType::unset;
        }

        JSON(const JSON& from){
            copy_assign(from);
        }

        JSON(JSON&& from) noexcept{
             move_assign(std::move(from));
        }

        JSON(const std::string& in,JSONType type);

        JSON(std::nullptr_t){
            type=JSONType::null;
        }

        JSON(bool b){
            type=JSONType::boolean;
            boolean=b;
        }

        JSON(char num){init_as_num(num);}
        JSON(unsigned char num){init_as_num(num);}
        JSON(short num){init_as_num(num);}
        JSON(unsigned short num){init_as_num(num);}
        JSON(int num){init_as_num(num);}
        JSON(unsigned int num){init_as_num(num);}
        JSON(long num){init_as_num(num);}
        JSON(unsigned long num){init_as_num(num);}
        JSON(long long num){init_as_num(num);}
        JSON(unsigned long long num){
            type=JSONType::unsignedi;
            numu=num;
        }

        template<class Struct>
        JSON(const Struct& in){
            try{
                to_json_detail(*this,in);
            }
            catch(...){
                destruct();
                type=JSONType::unset;
            }
        }


        template<class Struct>
        JSON(Struct* in){
            if(!in){
                type=JSONType::null;
            }
            else{
                try{
                    to_json_detail(*this,*in);
                }
                catch(...){
                    destruct();
                    type=JSONType::unset;
                } 
            }
        }

        
        JSON(double num){
            type=JSONType::floats;
            numf=num;
        }

        JSON(const std::string& in){
            type=JSONType::string;
            auto str=escape(in);
            value=EasyStr(str.c_str(),str.size());
        }

        JSON(const char* in){
            if(!in){
                type=JSONType::null;
            }
            else{
                type=JSONType::string;
                auto str=escape(in);
                value=EasyStr(str.c_str());
            }
        }

        JSON(JSONObjectType&& json){
            init_as_obj(std::move(json));   
        }

        JSON(JSONArrayType&& json){
            init_as_array(std::move(json));
        }

        JSON& operator[](size_t pos){
            array_if_unset();
            array_check(true,pos);
            return array->operator[](pos);
        }
        
        JSON& operator[](const std::string& key){
            obj_if_unset();
            obj_check(false,key);
            return obj->operator[](key);
        }


        JSON& at(const std::string& key)const{
            obj_check(true,key);
            return obj->at(key);
        }

        JSON& at(size_t pos)const{
            array_check(true,pos);
            return array->at(pos);
        }

        JSON* idx(size_t pos){
            array_if_unset();
            try{
                array_check(true,pos);
            }
            catch(...){
                return nullptr;
            }
            return &array->operator[](pos);
        }

        JSON* idx(const std::string& key){
            obj_if_unset();
            try{
                obj_check(true,key);
            }
            catch(...){
                return nullptr;
            }
            return &obj->operator[](key);
        }

        bool push_back(JSON&& json){
            array_if_unset();
            array_check(false,0);
            array->push_back(std::move(json));
            return true;
        }

        size_t size(){
            if(type!=JSONType::array)return 0;
            return array->size();
        }

        JSONType gettype()const{return type;}

        ~JSON(){
            destruct();
        }
        bool parse_assign(const std::string& in);

        std::string to_string(size_t indent=0,JSONFormat format=JSONFormat::defaultf) const;

        bool operator==(const JSON& right){
            if(this->type==right.type){
                if(this->type==JSONType::unset||this->type==JSONType::null){
                    return true;
                }
                else if(this->type==JSONType::string){
                    return this->value.size()==right.value.size()&&
                    strcmp(this->value.const_str(),right.value.const_str())==0;
                }
                else if(this->type==JSONType::integer){
                    return this->numi==right.numi;
                }
                else if(this->type==JSONType::floats){
                    return this->numf==right.numf;
                }
                else if(this->type==JSONType::unsignedi){
                    return this->numu==right.numu;
                }
                else if(this->type==JSONType::boolean){
                    return this->boolean==right.boolean;
                }
            }
            return this->to_string()==right.to_string();
        }

        JSON* path(const std::string& p);
    };

    JSON operator""_json(const char* in,size_t size);
    
    using JSONObject=JSON::JSONObjectType;
    using JSONArray=JSON::JSONArrayType;

    template<class Vec>
    JSON atoj(Vec& in){
        JSON ret="[]"_json;
        for(auto& i:in){
            ret.push_back(i);
        }
        return ret;
    }
}