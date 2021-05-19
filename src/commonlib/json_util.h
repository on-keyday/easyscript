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
        floats,
        string,
        object,
        array
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
            auto n=std::to_string(num);
            value=EasyStr(n.c_str(),n.size());
            return true;
        }

        JSON parse_json_detail(Reader<const char*>& reader);

        void destruct(){
            if(type==JSONType::object){
                delete obj;
                obj=nullptr;
            }
            else if(type==JSONType::array){
                delete array;
                array=nullptr;
            }
            else if(type==JSONType::boolean||type==JSONType::null||type==JSONType::unset){

            }
            else{
                value.~EasyStr();
            }
        }

        JSON& move_assign(JSON&& from){
            this->type=from.type;
            if(type==JSONType::object){
                obj=from.obj;
                from.obj=nullptr;
            }
            else if(type==JSONType::array){
                array=from.array;
                from.array=nullptr;
            }
            else if(type==JSONType::boolean){
                boolean=from.boolean;
            }
            else if(type==JSONType::null||type==JSONType::unset){

            }
            else{
                value=std::move(from.value);
            }
            from.type=JSONType::unset;
            return *this;
        }

        JSON& copy_assign(const JSON& from){
            this->type=from.type;
            if(type==JSONType::object){
                *this=JSON("",type);
                auto& m=*from.obj;
                for(auto& i:m){
                    obj->operator[](i.first)=i.second;
                }
            }
            else if(type==JSONType::array){
                *this=JSON("",type);
                auto& m=*from.array;
                for(auto& i:m){
                    array->push_back(i);
                }
            }
            else if(type==JSONType::boolean){
                boolean=from.boolean;
            }
            else if(type==JSONType::null||type==JSONType::unset){

            }
            else{
                value=EasyStr(from.value.const_str(),from.value.size());
            }
            return *this;
        }

        std::string to_string_detail(bool format,size_t ofs,size_t indent,size_t base_skip,bool useskip) const;

        std::string escape(const std::string& base);

        template<class Struct>
        auto to_json(const Struct& in) 
        -> decltype(in.to_json()){
            return in.to_json();
        }

        template<class Struct>
        auto to_json(Struct&& in) 
        -> decltype(in.to_json()){
            return in.to_json();
        }

        template<class Struct>
        auto to_json(Struct* in) 
        -> decltype(in->to_json()){
            return in->to_json();
        }

        std::string to_json(JSON&& in){
            return in.to_string();
        }

        template<class Vec>
        auto to_json(const Vec& vec)
        ->decltype(to_json(vec[0])){
            JSON js=JSONArrayType{};
            for(auto& o:vec){
                auto tmp=to_json(o);
                Reader<const char*> r(tmp.c_str());
                js.append(parse_json_detail(r));
            }
            return js.to_string();
        }

        template<class Map>
        auto to_json(const Map& m)
        ->decltype(to_json(m[std::string()])){
            JSON js=JSONObjectType{};
            for(auto& o:m){
                auto tmp=to_json(o.second);
                Reader<const char*> r(tmp.c_str());
                js[o.first]=parse_json_detail(r);
            }
            return js.to_string();
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

        JSON(const std::string& in,JSONType type){
            if(type==JSONType::object){
                init_as_obj({});
            }
            else if(type==JSONType::array){
                init_as_array({});
            }
            else if(type==JSONType::boolean){
                if(in=="true"){
                    boolean=true;
                }
                else if(in=="false"){
                    boolean=false;
                }
                else{
                    type=JSONType::unset;
                    return;
                }
            }
            else if(type==JSONType::null){

            }
            else{
                value=EasyStr(in.c_str(),in.size());
            }
            this->type=type;
        }


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

        template<class Struct>
        JSON(const Struct& in){
            try{
                parse_assign(to_json(in));
            }
            catch(...){
                type=JSONType::unset;
            }
        }

        template<class Struct>
        JSON(Struct&& in){
            try{
                parse_assign(to_json(in));
            }
            catch(...){
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
                    parse_assign(to_json(in));
                }
                catch(...){
                    type=JSONType::unset;
                } 
            }
        }

        JSON(double num){
            type=JSONType::floats;
            auto n=std::to_string(num);
            value=EasyStr(n.c_str(),n.size());
        }

        JSON(const std::string& in){
            type=JSONType::string;
            auto str=escape(in);
            value=EasyStr(str.c_str(),str.size());
        }

        JSON(const char* in){
            type=JSONType::string;
            auto str=escape(in);
            value=EasyStr(str.c_str());
        }

        JSON(JSONObjectType&& json){
            init_as_obj(std::move(json));   
        }

        JSON(JSONArrayType&& json){
            init_as_array(std::move(json));
        }

        JSON& operator[](size_t pos){
            if(type!=JSONType::array)throw "object kind miss match.";
            if(array->size()<=pos)throw "invalid index";
            return array->operator[](pos);
        }

        bool append(JSON&& json){
            if(type!=JSONType::array)return false;
            array->push_back(std::move(json));
            return true;
        }
        
        JSON& operator[](const std::string& key){
            if(type!=JSONType::object)throw "object kind miss match.";
            return obj->operator[](key);
        }

        JSON* idx(size_t pos){
            if(type!=JSONType::array)return nullptr;
            if(array->size()<=pos)return nullptr;
            return &array->operator[](pos);
        }

        JSON* idx(const std::string& key){
            if(type!=JSONType::object)return nullptr;
            if(!obj->count(key))return nullptr;
            return &obj->operator[](key);
        }

        JSONType gettype()const{return type;}

        ~JSON(){
            destruct();
        }
        bool parse_assign(const std::string& in);

        std::string to_string(bool format=false,size_t indent=2,bool useskip=false) const;
    };
    
    using JSONObject=JSON::JSONObjectType;
    using JSONArray=JSON::JSONArrayType;
}