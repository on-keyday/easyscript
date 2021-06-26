1+1;

true||false;
inet;
1+2+4-5&5+~bl;
if i:=0;i>-1{
    u:=55;
}
else;


for i:=r{

}

for i:=0;i<10;++i {

}

func ast(a,b){
    a+b;
}

type Int int32;

namespace Class{
    type Class struct{
        _ *Class
        A **Class
    };

    decl f(a *Class);
}

type Test *Class::Class;

compiler:=import("compiler");

namespace std{
    func invoke(callable,args []){
        type T typeof(callable);
        if !vaildexpr(callable(args)) {
            compiler.abort("'callable'(%t) is not callable",typeof(callable));    
        }
        return callable(args);
    }
}

std::invoke(Test::f,null);