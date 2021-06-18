/*
    Copyright (c) 2021 on-keyday
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include"basic_helper.h"
#include<string>
#include<algorithm>

namespace PROJECT_NAME{
    template<class Map,class Val>
    struct HTTPHeaderContext{
        Map buf;
        bool succeed=false;
        bool synerr=false;
        auto emplace(Val& key,Val& value)-> 
        decltype(buf.emplace(key,value)){
            std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){ return tolower(c);});
            return buf.emplace(key,value);
        }
    };

    template<class Map,class Val>
    struct HTTPRequest{
        HTTPHeaderContext<Map,Val> header;
        bool succeed=false;
        int version=0;
        Val method;
        Val path;
    };

    template<class Map,class Val>
    struct HTTPResponse{
        HTTPHeaderContext<Map,Val> header;
        bool succeed=false;
        int version=0;
        Val reason;
        int statuscode=0;
    };

    template<class Buf>
    struct HTTP2Frame{
        Buf buf;
        bool succeed=false;
        int id=0;
        int len=0;
        unsigned char type=0;
        unsigned char flag=0;
        int maxlen=16384; 
        bool continues=false;
    };

    template<class Buf>
    struct URLContext{
        bool needend=false;
        Buf scheme;
        Buf username;
        Buf password;
        Buf host;
        Buf port;
        Buf path;
        Buf query;
        Buf tag;
        bool succeed=false;
    };

    struct Base64Context{
        char c62='+';
        char c63='/';
        bool strict=false;
        bool succeed=false;
    };

    template<class Buf>
    struct URLEncodingContext{
        bool path=false;
        bool space_to_plus=false;
        bool query=false;
        bool big=false;
        bool failed=false;
        Buf no_escape;
        bool special(char c,bool decode){
            if(space_to_plus){
                if(decode){
                    return c==' ';
                }
                else{
                    return c=='+';
                }
            }
            return false;
        }
        bool special_add(Buf& ret,char c,bool decode){
            if(decode){
                if(c=='+'){
                    ret.push_back(' ');
                }
                else{
                    ret.push_back(c);
                }
            }
            else{
                if(c==' '){
                    ret.push_back('+');
                }
                else{
                    ret.push_back(c);
                }
            }
            return true;
        }
        bool noescape(char c){
            if(is_alpha_or_num(c))return true;
            if(path){
                if(c=='/'||c=='.'||c=='-')return true;
            }
            if(query){
                if(c=='?'||c=='&'||c=='=')return true;
            }
            for(auto i:no_escape){
                if(i==c)return true;
            }
            return false;
        }
    };

    template<class Buf,class Header,class Recver>
    struct HTTPBodyContext{
        size_t strtosz(Buf& str,bool& res,int base=10){
            size_t p=0;
            res=parse_int(str.data(),&str.data()[str.size()],p,base);
            return p;
        }
        Recver& recv;
        Header& header;
        bool has_header(const char* key,const char* value){
            auto res=header.count(key);
            if(res&&value){
                auto it=header.equal_range(key);
                res=(*it.first).second.find(value)!=~0;
            }
            return res;
        }

        auto header_ref(const char* in)-> 
        decltype((*header.equal_range(in).first).second)&{
            return (*header.equal_range(in).first).second;
        }

        HTTPBodyContext(Header& head,Recver& recver):recv(recver),header(head){}
        bool succeed=false;
    };

    template<class Val,class Map,class Buf>
    bool httpheader(Reader<Buf>* self,HTTPHeaderContext<Map,Val>& ctx,bool begin){
        using Char=typename Reader<Buf>::char_type;
        static_assert(sizeof(Char)==1);
        if(begin)return true;
        if(!self)return true;
        while(!self->ceof()){
            Val key,value;
            if(!self->readwhile(key,until,':')){
                return true;
            }
            if(!self->expect(":")){
                ctx.synerr=true;
                return true;
            }
            self->expect(" ");
            if(!self->readwhile(value,untilincondition,[](char c){return c!='\r'&&c!='\n';})){
                return true;
            }
            if(!self->expect("\r\n")&&!self->expect("\n")){
                if(!self->ceof(1))ctx.synerr=true;
                return true;
            }
            ctx.emplace(key,value);
            if(self->expect("\r\n")||self->expect("\n")){
                ctx.succeed=true;
                break;
            }
        }
        return true;
    }

    template<class Ctx,class Buf>
    bool httprequest(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Char=typename Reader<Buf>::char_type;
        static_assert(sizeof(Char)==1);
        if(begin)return true;
        if(!self)return true;
        if(!self->readwhile(ctx.method,until,' '))return true;
        if(!self->expect(" "))return true;
        if(!self->readwhile(ctx.path,until,' '))return true;
        if(!self->expect(" "))return true;
        if(!self->expect("HTTP/0.9")&&!self->expect("HTTP/1.0")&&!self->expect("HTTP/1.1")){
           return true;
        }
        ctx.version=((self->offset(-3)-'0')*10)+(self->offset(-1)-'0');
        if(!self->expect("\r\n")&&!self->expect("\n"))return true;
        if(!self->readwhile(httpheader,ctx.header,true))return true;
        ctx.succeed=ctx.header.succeed;
        return true;
    }

    template<class Ctx,class Buf>
    bool httpresponse(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Char=typename Reader<Buf>::char_type;
        static_assert(sizeof(Char)==1);
        if(begin)return true;
        if(!self)return true;
        char code[3]={0};
        if(!self->expect("HTTP/1.0")&&!self->expect("HTTP/1.1")){
            return true;
        }
        ctx.version=((self->offset(-3)-'0')*10)+(self->offset(-1)-'0');
        if(!self->expect(" "))return true;
        self->read_byte(code,3);
        if(!self->expect(" "))return true;
        ctx.statuscode=(code[0]-'0')*100+(code[1]-'0')*10+code[2]-'0';
        if(!self->readwhile(ctx.reason,untilincondition,[](char c){return c!='\r'&&c!='\n';}))return true;
        if(!self->expect("\r\n")&&!self->expect("\n"))return true;
        if(!self->readwhile(httpheader,ctx.header,true))return true;
        ctx.succeed=ctx.header.succeed;
        return true;
    }

    template<class Ctx,class Buf>
    bool http2frame(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Char=typename Reader<Buf>::char_type;
        static_assert(sizeof(Char)==1);
        if(begin){
            ctx.succeed=false;
            ctx.continues=false;
            return true;
        }
        if(!self)return true;
        if(self->readable()<9){
            ctx.continues=true;
            return true;
        }
        if(self->read_byte(&ctx.len,3,translate_byte_net_and_host,true)<3)return true;
        ctx.len>>=8;
        if(ctx.len>=0x00800000||ctx.len<0)return true;
        if(self->read_byte(&ctx.type,1,translate_byte_net_and_host,true)<1)return true;
        if(self->read_byte(&ctx.flag,1,translate_byte_net_and_host,true)<1)return true;
        if(self->read_byte(&ctx.id,4,translate_byte_net_and_host,true)<4)return true;
        if(ctx.id<0)return true;
        if(self->readable()<ctx.len){
            ctx.continues=true;
        }
        else if(ctx.len>ctx.maxlen){
            return true;
        }
        ctx.buf.resize(ctx.len);
        memmove(ctx.buf.data(),&self->ref().data()[self->readpos()],ctx.len);
        self->seek(self->readpos()+ctx.len);
        ctx.succeed=true;
        return true;
    }

    template<class Ctx,class Buf>
    bool http2frame_reader(Reader<Buf>* self,Ctx& ctx,bool begin){
        if(!self)return true;
        if(!begin)return true;
        auto beginpos=self->readpos();
        self->readwhile(http2frame,ctx);
        if(ctx.succeed){
            self->ref().erase(beginpos,ctx.len+9);
            self->seek(beginpos);
        }
        if(ctx.continues){
            self->seek(beginpos);
        }
        return false;
    }


    template<class Ctx,class Buf>
    bool parse_url(Reader<Buf>* self,Ctx& ctx,bool begin){
        using Char=typename Reader<Buf>::char_type;
        static_assert(sizeof(Char)==1);
        if(begin)return true;
        if(!self)return true;
        auto beginpos=self->readpos();
        bool has_scheme=false,has_userandpass=false;
        if(self->readwhile(untilnobuf,(Char)'/')){
            if(self->offset(-1)==(Char)':'){
                has_scheme=true;
            }
        }
        self->seek(beginpos);
        if(self->readwhile(untilnobuf,(Char)'@')){
            has_userandpass=true;
        }
        self->seek(beginpos);
        if(has_scheme){
            self->readwhile(ctx.scheme,untilincondition,is_url_scheme_usable<Char>);
            if(!self->expect(":"))return true;
        }
        self->expect("//");
        if(has_userandpass){
            if(!self->readwhile(ctx.username,untilincondition,
            +[](Char c){return c!=':'&&c!='@';}))return true;
            if(self->achar()==':'){
                if(!self->expect(":"))return true;
                if(!self->readwhile(ctx.password,until,(Char)'@'))return true;
            }
            if(!self->expect("@"))return true;
        }
        if(self->expect("[")){
            ctx.host.push_back('[');
            if(!self->readwhile(ctx.host,untilincondition,[](Char c){
                return c==(Char)':'||is_hex(c);
            },true))return false;
            if(ctx.host.size()<=1)return false;
            if(!self->expect("]"))return false;
            ctx.host.push_back(']');
        }
        else{
            if(!self->readwhile(ctx.host,untilincondition,is_url_host_usable<Char>,true))return true;
        }
        if(self->expect(":")){
            if(!self->readwhile(ctx.port,untilincondition,+[](Char c){return is_digit(c);},true))return true;
            //if(!self->ahead("/"))return true;
        }
        if(self->ahead("/")){
            if(!self->readwhile(ctx.path,untilincondition,+[](Char c){return c!='?'&&c!='#';},true))return true;
            if(self->ahead("?")){
                if(!self->readwhile(ctx.query,untilincondition,+[](Char c){return c!='#';},true))return true;
            }
            if(self->ahead("#")){
                if(!self->readwhile(ctx.tag,untilincondition,+[](Char c){return c!='\0';},true))return true;
            }
        }
        if(ctx.needend&&!self->ceof())return true;
        ctx.succeed=true;
        return true;
    }

    
    template<class Ret,class Ctx,class Buf>
    bool base64_encode(Reader<Buf>* self,Ret& ret,Ctx& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        if(!ctx)return true;
        if(begin)return true;
        if(!self){
            return true;
        }
        auto translate=[&ctx](unsigned char c)->char{
            if(c>=64)return 0;
            if(c<26){
                return 'A'+c;
            }
            else if(c>=26&&c<52){
                return 'a'+(c-26);
            }
            else if(c>=52&&c<62){
                return '0'+(c-52);
            }
            else if(c==62){
                return ctx->c62;
            }
            else if(c==63){
                return ctx->c63;
            }
            return 0;
        };
        while(!self->ceof()){
            int num=0;
            auto read_size=self->read_byte(&num,3,translate_byte_net_and_host);
            if(!read_size)break;
            num>>=8;
            char buf[]={(char)((num>>18)&0x3f),(char)((num>>12)&0x3f),(char)((num>>6)&0x3f),(char)(num&0x3f)};
            for(auto i=0;i<read_size+1;i++){
                ret.push_back(translate(buf[i]));
            }
        }
        while(ret.size()%4){
            ret.push_back('=');
        }
        return true;
    }

    template<class Ret,class Ctx,class Buf>
    bool base64_decode(Reader<Buf>* self,Ret& ret,Ctx& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        if(!ctx)return true;
        if(begin)return true;
        if(!self){
            return true;
        }
        auto firstsize=self->readable();
        if(ctx->strict==true&&firstsize%4)return true;
        auto translate=[&ctx](char c)->unsigned char{
            if(c>='A'&&c<='Z'){
                return c-'A';
            }
            else if(c>='a'&&c<='z'){
                return c -'a'+ 26;
            }
            else if(c>='0'&&c<='9'){
                return c+-'0'+52;
            }
            else if(c==ctx->c62){
                return 62;
            }
            else if(c==ctx->c63){
                return 63;
            }
            return ~0;
        };
        Reader<Buf> tmp;
        self->readwhile(tmp.ref(),addifmatch,[&ctx](char c){
            return is_alpha_or_num(c)||ctx->c62==c||ctx->c63==c||c=='=';});
        if(ctx->strict){
            if(tmp.readable()!=firstsize)return true;
        }
        while(!tmp.ceof()){
            int shift=0;
            char bytes[4]={0},*shift_p=(char*)&shift;
            auto readsize=tmp.read_byte(bytes,4);
            auto no_ignore=0;
            for(auto i=0;i<readsize;i++){
                auto bit=translate(bytes[i]);
                if(bit==0xff)
                    break;
                no_ignore++;
                shift+=((bit<<6)*(3-i));
            }
            shift=translate_byte_net_and_host<int>(shift_p);
            shift>>=8;
            for(auto i=0;i<no_ignore-1;i++){
                ret.push_back(shift_p[i]);
            }
        }
        ctx->succeed=true;
        return true;
    }

    template<class Ret,class Ctx,class Buf>
    bool url_encode(Reader<Buf>* self,Ret& ret,Ctx& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        if(begin)return true;
        if(!self)return true;
        if(!ctx->noescape(self->achar())){
            if(ctx->special(self->achar(),false)){
                ctx->special_add(ret,self->achar(),false);
            }
            else{
                auto c=self->achar();
                auto msb=(c&0xf0)>>4;
                auto lsb=c&0x0f;
                auto translate=[&ctx](unsigned char c)->unsigned char{
                    if(c>15)return 0;
                    if(c<10){
                        return c+'0';
                    }
                    else{
                        if(ctx->big){
                            return c-10+'A';
                        }
                        else{
                            return c-10+'a';
                        }
                    }
                    return 0;
                };
                ret.push_back('%');
                ret.push_back(translate(msb));
                ret.push_back(translate(lsb));
            }
        }
        else{
            ret.push_back(self->achar());
        }
        return false;
    }

     template<class Ret,class Ctx,class Buf>
    bool url_decode(Reader<Buf>* self,Ret& ret,Ctx& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        if(begin)return true;
        if(!self){
            return true;
        }
        if(ctx->special(self->achar(),true)){
            ctx->special_add(ret,self->achar(),true);
        }
        else if(self->achar()=='%'){
            auto first=0;
            auto second=0;
            self->increment();
            first=self->achar();
            second=self->achar();
            auto translate=[](unsigned char c)->unsigned char{
                if(c>='0'&&c<='9'){
                    return c-'0';
                }
                else if(c>='A'&&c<='F'){
                    
                    return c-'A'+10;
                }
                else if(c>='a'&&c<='f'){
                    return c-'a'+10;
                }
                return ~0;
            };
            first=translate(first);
            second=translate(second);
            if(first==0xff||second==0xff){
                ctx->failed=true;
                return true;
            }
            ret.push_back((first<<4)+second);
        }
        else{
            ret.push_back(self->achar());
        }
        return false;
    }

    template<class Ret,class Ctx,class Buf>
    bool httpbody(Reader<Buf>* self,Ret& ret,Ctx& ctx,bool begin){
        static_assert(sizeof(typename Reader<Buf>::char_type)==1);
        if(!ctx)return false;
        if(!begin)return true;
        if(!self)return true;
        //auto& header=ctx->header;
        bool res=true;
        if(ctx->has_header("transfer-encoding","chunked")){
            size_t sum=0;
            while(true){
                while(!is_hex(self->achar())){
                    if(!ctx->recv(self->ref()))return false;
                }
                Buf num;
                self->readwhile(num,until,'\r');
                while(!self->expect("\r\n")){
                    if(!ctx->recv(self->ref()))return false;
                }
                auto size=ctx->strtosz(num,res,16);
                if(!res)return false;
                if(size==0)break;
                while(self->readable()<size){
                    if(!ctx->recv(self->ref()))return false;
                }
                auto prevsum=sum;
                sum+=size;
                ret.resize(sum);
                memmove(&ret.data()[prevsum],&self->ref().data()[self->readpos()],size);
                self->seek(self->readpos()+size);
                while(!self->expect("\r\n")){
                    if(!ctx->recv(self->ref()))return false;
                }
            }
            ctx->succeed=true;
        }
        else if(ctx->has_header("content-length",nullptr)){
            size_t size=ctx->strtosz(ctx->header_ref("content-length"),res);
            if(!res)return false;
            while(self->readable()<size){
                if(!ctx->recv(self->ref()))return false;
            }
            ret.resize(size);
            memmove(ret.data(),&self->ref().data()[self->readpos()],size);
            ctx->succeed=true;
        }
        return false;
    }

}