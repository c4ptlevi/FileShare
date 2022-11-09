
#include<vector>
using namespace std;

//enum task_type{LIST , DOWNLOAD , UPLOAD};

struct task
{
    int tt;
    int clientFd;
    char* file;
    int download_offset;
    int upload_offset;
    task(){
        tt=-1;
        upload_offset=0;
        download_offset=0;
        clientFd=-1;
        file=NULL;
    }
};


struct queue{
    int current_size;
    int WIN_SIZE;
    vector<task*>q;
};
