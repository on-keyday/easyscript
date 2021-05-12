#include"http2.h"
using namespace PROJECT_NAME;

bool HTTP2AppContext::send_frame(H2FType type,H2Flag flag,int id,char* data,int size){
    if(no_change)return false;
    auto streams=self->get_streams(this);
    if(!streams)return false;
    if(type>WINDOW_UPDATE)return false;
    return streams->send_frame(type,flag,id,data,size);
}