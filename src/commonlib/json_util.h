#include"reader.h"
#include<string>
#include<map>
#include<vector>
#include<exception>

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
    private:
        using JSONMap=std::map<std::string,JSON>;
        using JSONArray=std::vector<JSON>;
        union{
            bool boolean;
            EasyStr value=EasyStr();
            JSONMap* obj;
            JSONArray* array;
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

        bool init_as_obj(std::initializer_list<std::pair<const std::string,JSON>>&& json){
            obj=init<JSONMap>();
            if(!obj){
                type=JSONType::unset;
                return false;
            }
            type=JSONType::object;
            for(auto& o:json){
                obj->try_emplace(o.first,std::move(o.second));
            }
            
            return true;
        }

        bool init_as_array(std::initializer_list<JSON>&& json){
            array=init<JSONArray>();
            if(!array){
                type=JSONType::unset;
                return false;
            }
            type=JSONType::array;
            for(auto& o:json){
                
            }
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

        std::string to_string_detail(bool format,size_t ofs,size_t indent,size_t base_skip,bool useskip);
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

        JSON(long long num){
            type=JSONType::integer;
            auto n=std::to_string(num);
            value=EasyStr(n.c_str(),n.size());
        }

        JSON(double num){
            type=JSONType::floats;
            auto n=std::to_string(num);
            value=EasyStr(n.c_str(),n.size());
        }

        JSON(const std::string& str){
            type=JSONType::string;
            value=EasyStr(str.c_str(),str.size());
        }

        JSON(std::initializer_list<std::pair<const std::string,JSON>>&& json){
            init_as_obj(std::move(json));   
        }

        JSON(std::initializer_list<JSON>&& json){
            init_as_array(std::move(json));
        }

        JSON& operator[](size_t pos){
            if(type!=JSONType::array)throw std::exception("object kind miss match.");
            return array->operator[](pos);
        }

        bool append(JSON&& json){
            if(type!=JSONType::array)return false;
            array->push_back(std::move(json));
            return true;
        }
        
        JSON& operator[](std::string& key){
            if(type!=JSONType::object)throw std::exception("object kind miss match.");
            return obj->operator[](key);
        }

        ~JSON(){
            destruct();
        }
        bool parse_assign(const std::string& in);

        std::string to_string(bool format=false,size_t indent=2,bool useskip=false);
    };
    
}