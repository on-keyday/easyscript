#pragma once
#include"project_name.h"
#include"extutil.h"
#include"serializer.h"
#include"fileio.h"
#include<map>

namespace PROJECT_NAME{
    struct Decomposition{
        std::string command;
        std::u32string to;
    };

    struct Numeric{
        char32_t v1=(char32_t)-1;
        char32_t v2=(char32_t)-1;
        std::string v3;
    };

    struct CaseMap{
        char32_t upper=(char32_t)-1;
        char32_t lower=(char32_t)-1;
        char32_t title=(char32_t)-1;
    };

    struct CodeInfo{
        char32_t codepoint;
        std::string name;
        std::string category;
        unsigned int ccc=0; //Canonical_Combining_Class
        std::string bidiclass;
        Decomposition decomposition;
        Numeric numeric;
        bool mirrored=false;
        CaseMap casemap;
    };

    struct UnicodeData{
        std::map<char32_t,CodeInfo> codes;
        std::multimap<std::string,CodeInfo*> names;
        std::multimap<std::string,CodeInfo*> categorys;
    };

    inline void parse_case(std::vector<std::string>& d,CaseMap& ca){
        if(d[12]!=""){
            unsigned int c=(unsigned int)-1;
            Reader("0x"+d[12])>> c;
            ca.upper=c;
        }
        if(d[13]!=""){
            unsigned int c=(unsigned int)-1;
            Reader("0x"+d[13])>>c;
            ca.lower=c;
        }
        if(d.size()==15){
            unsigned int c=(unsigned int)-1;
            Reader("0x"+d[14])>>c;
            ca.title=c;
        }
    }

    inline void parse_decomposition(std::string& s,Decomposition& res){
        if(s=="")return;
        auto c=split(s," ");
        size_t pos=0;
        if(c[0][0]=='<'){
            res.command=c[0];
            pos=1;
        }
        for(auto i=pos;i<c.size();i++){
            unsigned int ch=0;
            Reader(c[i])>> ch;
            res.to.push_back((char32_t)ch);
        }
    }

    inline void parse_codepoint(std::vector<std::string>& d,CodeInfo& info){
        unsigned int codepoint=(unsigned int)-1;
        Reader("0x"+d[0])>>codepoint;
        info.codepoint=(char32_t)codepoint;
        info.name=d[1];
        info.category=d[2];
        Reader("0x"+d[3])>>info.ccc;
        info.bidiclass=d[4];
        parse_decomposition(d[5],info.decomposition);
        if(d[6]!=""){
            codepoint=(unsigned int)-1;
            Reader(d[6])>>codepoint;
            info.numeric.v1=codepoint;
        }
        if(d[7]!=""){
            codepoint=(unsigned int)-1;
            Reader(d[7])>>codepoint;
            info.numeric.v2=codepoint;
        }
        info.numeric.v3=d[8];
        info.mirrored=d[9]=="Y"?true:d[9]=="N"?false:throw "Why";
        parse_case(d,info.casemap);
    }

    inline UnicodeData parse_unicodedata(std::vector<std::vector<std::string>>& data){
        UnicodeData ret;
        for(auto& d:data){
            CodeInfo info;
            parse_codepoint(d,info);
            auto& point=ret.codes[info.codepoint];
            ret.names.emplace(info.name,&point);
            ret.categorys.emplace(info.category,&point);
            point=std::move(info);
        }
        return ret;
    }



    template<class C>
    std::vector<std::vector<std::string>> load_unicodedata_text(C* name){
        Reader<FileMap> r=FileMap(name);

        auto each=lines(r,false);
        
        std::vector<std::vector<std::string>> ret;

        for(auto& i:each){
            ret.push_back(split(i,";"));
        }

        return ret;
    }

    template<class Buf>
    void serialize_codeinfo(Buf& ret,CodeInfo& info){
        Serializer<Buf&> w(ret);
        w.write_hton(info.codepoint);
        w.write_as<char>(info.name.size());
        w.write_byte(info.name);
        w.write_as<char>(info.category.size());
        w.write_byte(info.category);
        w.write_as<char>(info.ccc);
        w.write_as<char>(info.bidiclass.size());
        w.write_byte(info.bidiclass);
        w.write_as<char>(info.decomposition.command.size());
        w.write_byte(info.decomposition.command);
        size_t size=info.decomposition.to.size();
        w.write_as<char>(size*sizeof(char32_t));
        w.write_hton(info.decomposition.to.data(),size);
        w.write_as<char>(info.numeric.v1);
        w.write_as<char>(info.numeric.v2);
        w.write_as<char>(info.numeric.v3.size());
        w.write_byte(info.numeric.v3);
        w.write(info.mirrored);
        w.write_hton(info.casemap.upper);
        w.write_hton(info.casemap.lower);
        w.write_hton(info.casemap.title);
    /*
        auto change_endian=[](char32_t& in){
            return translate_byte_net_and_host<char32_t>((char*)&in);
        };
        auto codepoint=change_endian(info.codepoint);
        char* c=(char*)&codepoint;
        ret+=std::string(c,4);
        ret+=(char)info.name.size();
        ret+=info.name;
        ret+=(char)info.category.size();
        ret+=info.category;
        ret+=(char)info.ccc;
        ret+=(char)info.bidiclass.size();
        ret+=info.bidiclass;
        ret+=(char)info.decomposition.command.size();
        ret+=info.decomposition.command;
        size_t size=(info.decomposition.to.size()*sizeof(char32_t));
        ret+=(char)size;
        for(auto& p:info.decomposition.to){
            codepoint=change_endian(p);
            ret+=std::string((char*)&codepoint,4);
        }
        ret+=(char)info.numeric.v1;
        ret+=(char)info.numeric.v2;
        ret+=(char)info.numeric.v3.size();
        ret+=info.numeric.v3;
        ret+=(char)info.mirrored;
        codepoint=change_endian(info.casemap.upper);
        ret+=std::string((char*)&codepoint,4);
        codepoint=change_endian(info.casemap.lower);
        ret+=std::string((char*)&codepoint,4);
        codepoint=change_endian(info.casemap.title);
        ret+=std::string((char*)&codepoint,4);
    */
    }

    template<class Buf>
    bool deserialize_codeinfo(Deserializer<Buf>& r,CodeInfo& info){
        if(!r.read_reverse(info.codepoint))return false;
        size_t size=0;
        if(!r.read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.name,size))return false;
        if(!r.read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.category,size))return false;
        if(!r.read_as<unsigned char>(info.ccc))return false;
        if(!r.read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.bidiclass,size))return false;
        if(!r.read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.decomposition.command,size))return false;
        if(!r.read_as<unsigned char>(size))return false;
        if(!r.read_byte_ntoh(info.decomposition.to,size/sizeof(char32_t)))return false;
        if(!r.read_as<unsigned char>(info.numeric.v1))return false;
        if(info.numeric.v1==0xff)info.numeric.v1=(char32_t)-1;
        if(!r.read_as<unsigned char>(info.numeric.v2))return false;
        if(info.numeric.v2==0xff)info.numeric.v2=(char32_t)-1;
        if(!r.read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.numeric.v3,size))return false;
        if(!r.read(info.mirrored))return false;
        if(!r.read_ntoh(info.casemap.upper))return false;
        if(!r.read_ntoh(info.casemap.lower))return false;
        if(!r.read_ntoh(info.casemap.title))return false;
        return true;
    }

    template<class Buf>
    void serialize_unicodedata(Buf& ret,UnicodeData& data){
        for(auto& d:data.codes){
            serialize_codeinfo(ret,d.second);
        }   
    }

    template<class Buf>
    bool deserialize_unicodedata(Deserializer<Buf>& r,UnicodeData& ret){
        while(!r.eof()){
            CodeInfo info;
            if(!deserialize_codeinfo(r,info)){
                return false;
            }
            auto& point=ret.codes[info.codepoint];
            ret.names.emplace(info.name,&point);
            ret.categorys.emplace(info.category,&point);
            point=std::move(info);
        }
        return true;
    }


}