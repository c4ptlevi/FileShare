
#include<vector>
using namespace std;

enum task_type{LIST , DOWNLOAD , UPLOAD};

struct task
{
    task_type tt;
    int clientFd;
    char* file;
    int download_offset;
    int upload_offset;
    task(){}
};


struct queue{
    int current_size;
    int WIN_SIZE;
    vector<task*>q;
};