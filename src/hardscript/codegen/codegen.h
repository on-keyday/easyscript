
struct CallStack{
    int depth=0;
    void push();
    void pop(const char* arg);
    void pushf();
    void popf(const char* arg);
};

struct Register{
    
};