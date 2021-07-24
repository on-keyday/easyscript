#pragma once
#include"project_name.h"
#include<map>
#include<string>
#include<stddef.h>

#include"reader.h"
#include"extutil.h"
#include"fileio.h"
#include"serializer.h"



namespace PROJECT_NAME{

    struct Decomposition{
        std::string command;
        std::u32string to;
    };

    constexpr unsigned char exist_one_bit=0x1;
    constexpr unsigned char exist_two_bit=0x2;
    constexpr unsigned char signbit=0x4;
    constexpr unsigned char large_numbit=0x8;

    struct Numeric{
        int v1=(int)-1;
        int v2=(int)-1;
        //std::string v3;
        unsigned char flag=0;
        union{
            struct{ 
                int _1;
                int _2;
            }v3_S;
            long long v3_L=-1;
        };

        std::string stringify()const{
            std::string ret;
            ret=(flag&signbit?"-":"");
            if(flag&large_numbit){
                ret+=std::to_string(v3_L);
            }
            else{
                ret+=std::to_string(v3_S._1);
                if(flag&exist_two_bit){
                    ret+= "/" + std::to_string(v3_S._2);
                }
            }
            return ret;
        }

        double num()const{
            double ret=0;
            if(!flag)return NAN;
            if(flag&large_numbit){
                ret=(double)v3_L;
            }
            else{
                ret=(double)v3_S._1;
                if(flag&exist_two_bit){
                    ret=ret/v3_S._2;
                }
            }
            if(flag&signbit){
                ret=-ret;
            }
        }
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
        CodeInfo* range=nullptr;
    };

    struct CodeRange{
        CodeInfo* begin=nullptr;
        CodeInfo* end=nullptr;
    };

    struct UnicodeData{
        std::map<char32_t,CodeInfo> codes;
        std::multimap<std::string,CodeInfo*> names;
        std::multimap<std::string,CodeInfo*> categorys;
        std::vector<CodeRange> ranges;
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
            Reader("0x"+c[i])>> ch;
            res.to.push_back((char32_t)ch);
        }
    }

    inline void parse_real(std::string& str,Numeric& numeric){
        if(str=="")return;
        Reader<Refer<std::string>> r(str);
        if(r.expect("-")){
            numeric.flag|=signbit;
        }
        if(!r.ceof()){
            size_t pos=r.readpos();
            r>>numeric.v3_S._1;
            if(numeric.v3_S._1==-1){
                r.seek(pos);
                r>> numeric.v3_L;
                numeric.flag|=large_numbit;
            }
            else{
                numeric.flag|=exist_one_bit;
                if(r.expect("/")){
                    r>>numeric.v3_S._2;
                    numeric.flag|=exist_two_bit;
                }
            }
        }
    }

    inline void parse_numeric(std::vector<std::string>& d,CodeInfo& info){
        if(d[6]!=""){
            //codepoint=(unsigned int)-1;
            Reader(d[6])>>info.numeric.v1;
            //info.numeric.v1=codepoint;
        }
        if(d[7]!=""){
            //codepoint=(unsigned int)-1;
            Reader(d[7])>>info.numeric.v2;
            //info.numeric.v2=codepoint;
        }
        //info.numeric.v3=d[8];
        parse_real(d[8],info.numeric);
    }

    inline bool parse_codepoint(std::vector<std::string>& d,CodeInfo& info){
        if(d.size()<14)return false;
        unsigned int codepoint=(unsigned int)-1;
        Reader("0x"+d[0])>>codepoint;
        info.codepoint=(char32_t)codepoint;
        info.name=d[1];
        info.category=d[2];
        Reader("0x"+d[3])>>info.ccc;
        info.bidiclass=d[4];
        parse_decomposition(d[5],info.decomposition);
        parse_numeric(d,info);
        if(d[9]!="Y"&&d[9]!="N")return false;
        info.mirrored=d[9]=="Y"?true:false;
        parse_case(d,info.casemap);
        return true;
    }

    inline void set_codepoint_info(CodeInfo& info,UnicodeData& ret,CodeInfo*& prev){
        auto& point=ret.codes[info.codepoint];
        ret.names.emplace(info.name,&point);
        ret.categorys.emplace(info.category,&point);
        if(prev){
            if(info.codepoint!=prev->codepoint+1&&info.name.back()=='>'&&
               prev->name.back()=='>'&&!prev->range){
                ret.ranges.push_back(CodeRange{prev,&point});
                info.range=prev;
                prev->range=&point;
            }
        }
        point=std::move(info);
        prev=&point;
    }

    inline bool parse_unicodedata(std::vector<std::vector<std::string>>& data,UnicodeData& ret){
        CodeInfo* prev=nullptr;
        for(auto& d:data){
            CodeInfo info;
            if(!parse_codepoint(d,info)){
                return false;
            }
            set_codepoint_info(info,ret,prev);
        }
        return true;
    }



    template<class C>
    std::vector<std::vector<std::string>> load_unicodedata_text(C* name){
        Reader<FileReader> r(name);

        if(!r.ref().is_open()){
            return std::vector<std::vector<std::string>>();
        }

        auto each=lines(r,false);
        
        std::vector<std::vector<std::string>> ret;

        for(auto& i:each){
            ret.push_back(split(i,";"));
        }

        return ret;
    }

    template<class Buf>
    void serialize_codeinfo(Serializer<Buf> w,CodeInfo& info,int version=1){
        w.write_hton(info.codepoint);
        w.template write_as<char>(info.name.size());
        w.write_byte(info.name);
        w.template write_as<char>(info.category.size());
        w.write_byte(info.category);
        w.template write_as<char>(info.ccc);
        w.template write_as<char>(info.bidiclass.size());
        w.write_byte(info.bidiclass);
        w.template write_as<char>(info.decomposition.command.size());
        w.write_byte(info.decomposition.command);
        size_t size=info.decomposition.to.size();
        w.template write_as<char>(size*sizeof(char32_t));
        w.write_hton(info.decomposition.to.data(),size);
        w.template write_as<char>(info.numeric.v1);
        w.template write_as<char>(info.numeric.v2);
        if(version==0){
            std::string str=info.numeric.stringify();
            w.template write_as<char>(str.size());
            w.write_byte(str);
        }
        else if(version==1){
            w.write(info.numeric.flag);
            if(info.numeric.flag&large_numbit){
                w.write_hton(info.numeric.v3_L);
            }
            else{
                if(info.numeric.flag&exist_one_bit){
                    w.write_hton(info.numeric.v3_S._1);
                }
                if(info.numeric.flag&exist_two_bit){
                    w.write_hton(info.numeric.v3_S._2);
                }
            }
        }
        w.write(info.mirrored);
        w.write_hton(info.casemap.upper);
        w.write_hton(info.casemap.lower);
        w.write_hton(info.casemap.title);
    }

    template<class Buf>
    bool deserialize_codeinfo(Deserializer<Buf>& r,CodeInfo& info,int version=1){
        if(!r.read_reverse(info.codepoint))return false;
        size_t size=0;
        if(!r.template read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.name,size))return false;
        if(!r.template read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.category,size))return false;
        if(!r.template read_as<unsigned char>(info.ccc))return false;
        if(!r.template read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.bidiclass,size))return false;
        if(!r.template read_as<unsigned char>(size))return false;
        if(!r.read_byte(info.decomposition.command,size))return false;
        if(!r.template read_as<unsigned char>(size))return false;
        if(!r.read_byte_ntoh(info.decomposition.to,size/sizeof(char32_t)))return false;
        if(!r.template read_as<unsigned char>(info.numeric.v1))return false;
        if(info.numeric.v1==0xff)info.numeric.v1=-1;
        if(!r.template read_as<unsigned char>(info.numeric.v2))return false;
        if(info.numeric.v2==0xff)info.numeric.v2=-1;
        if(!r.template read_as<unsigned char>(info.numeric.flag))return false;
        if(version==0){
            size=info.numeric.flag;
            info.numeric.flag=0;
            std::string str;
            if(!r.read_byte(str,size))return false;
            parse_real(str,info.numeric);
        }
        else if(version==1){
            if(info.numeric.flag&large_numbit){
                if(!r.read_ntoh(info.numeric.v3_L)){
                    return false;
                }
            }
            else{
                if(info.numeric.flag&exist_one_bit){
                    if(!r.read_ntoh(info.numeric.v3_S._1)){
                        return false;
                    }
                }
                if(info.numeric.flag&exist_two_bit){
                    if(!r.read_ntoh(info.numeric.v3_S._2)){
                        return false;
                    }
                }
            }
        }
        //if(!r.read_byte(info.numeric.v3,size))return false;
        if(!r.read(info.mirrored))return false;
        if(!r.read_ntoh(info.casemap.upper))return false;
        if(!r.read_ntoh(info.casemap.lower))return false;
        if(!r.read_ntoh(info.casemap.title))return false;
        return true;
    }

    template<class Buf>
    void serialize_unicodedata(Serializer<Buf>& ret,UnicodeData& data,int version=1){
        if(version==1){
            ret.write_byte("UDv1",4);
        }
        for(auto& d:data.codes){
            serialize_codeinfo(ret,d.second,version);
        }   
    }

    template<class Buf>
    bool deserialize_unicodedata(Deserializer<Buf>& r,UnicodeData& ret){
        int version=0;
        if(r.base_reader().expect("UDv1")){
            version=1;
        }
        CodeInfo* prev=nullptr;
        while(!r.eof()){
            CodeInfo info;
            if(!deserialize_codeinfo(r,info,version)){
                return false;
            }
            set_codepoint_info(info,ret,prev);
        }
        return true;
    }


}