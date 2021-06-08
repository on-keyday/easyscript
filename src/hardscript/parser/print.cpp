#include"control.h"
#include"../../commonlib/json_util.h"
#include"node.h"

using namespace PROJECT_NAME;
using namespace control;
using namespace node;

namespace control{
    void to_json(JSON& to,const CtrlKind& kind){
        switch (kind)
        {
        case CtrlKind::ctrl:
            to="ctrl";
            break;
        case CtrlKind::decl:
            to="decl";
            break;
        case CtrlKind::expr:
            to="expr";
            break;
        case CtrlKind::func:
            to="func";
            break;
        case CtrlKind::var:
            to="var";
            break;
        default:
            to="unknown";
            break;
        }
    }
    /*
    void to_json(JSON& to,const NodeKind& kind){
        switch (kind)
        {
        case NodeKind::boolean:
            to="bool";
            break;
        case NodeKind::integer:
            to="int";
            break;
        case NodeKind::floats:
            to="float";
            break;
        case NodeKind::string:
            to="string";
            break;
        case NodeKind::create:
            to="new";
            break;
        case NodeKind::closure:
            to="closure";
            break;
        default:
            to="unknown";
            break;
        }
    }*/
}
 

void Tree::to_json(JSON& json)const{
    json["kind"]=kind;
    json["symbol"]=symbol;
    json["left"]=left;
    json["right"]=right;
    for(auto o:arg){
        json["arg"].push_back(o);
    }
}

void Control::to_json(JSON& json)const{
    json["kind"]=kind;
    json["name"]=name;
    json["expr"]=expr;
    json["type"]=type;
    json["inblockpos"]=(long long)inblockpos;
    json["inblock"]=inblock;
    for(auto& o:arg){
        json["param"].push_back(o);
    }
    for(auto& o:argtype){
        json["paramtype"].push_back(o);
    }
}