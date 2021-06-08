#include<cstdint>
namespace eval{
    template<class Node,class Kind>
    Kind is_const(Node* n){
        switch (n->kind)
        {
        case Kind::integer:
        case Kind::boolean:
        case Kind::string:
        case Kind::floats:
            return n->kind;
        case Kind::unknown:{
            auto& s=n->symbol;
            if(n->left&&n->right){
                
            }
        }
        default:
            break;
        }
        return Kind::unknown;
    }

    template<class Node,class Kind>
    uint64_t eval_int(Node* n,bool& r){
        auto& s=n->symbol;
        
    }
}