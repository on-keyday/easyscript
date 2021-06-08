#include<cstdint>
#include<operators.h>
namespace eval{
    template<class Node>
    node::NodeKind is_const(Node* n){
        using Kind=node::NodeKind;
        switch (n->kind)
        {
        case Kind::integer:
        case Kind::boolean:
        case Kind::string:
        case Kind::floats:
            return n->kind;
        default:{
            auto& s=n->symbol;
            if(n->left&&n->right){
                
            }
        }
        }
        return Kind::unknown;
    }

    template<class Node,class Kind>
    uint64_t eval_int(Node* n,bool& r){
        auto& s=n->symbol;
        
    }
}