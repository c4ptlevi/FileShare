
#include<bits/stdc++.h>
using namespace std;

//enum task_type{LIST , DOWNLOAD , UPLOAD};

struct task
{
    int tt;
    int clientFd;
    string file;
    int download_offset;
    int upload_offset;
    task(){
        tt=-1;
        upload_offset=0;
        download_offset=0;
        clientFd=-1;
        file="";
    }
};


struct task_queue{
    int current_size;
    int WIN_SIZE;
    vector<task*>q;
    task_queue(int wsize){
        WIN_SIZE = wsize;
        current_size = 0;
        q.assign(WIN_SIZE , nullptr);
    }
};
