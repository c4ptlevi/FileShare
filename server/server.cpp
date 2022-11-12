#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>	
#include<unistd.h>		//read and write function
#include<string.h>
#include<cstdlib>	
#include<fstream>		//file handling operation
#include<pthread.h>
#include<chrono>

using namespace std;

#include "queue.cpp"

#define PORT 3000
#define WIN_SIZE 20
#define QUANTA  4
#define GET_TIME duration_cast< milliseconds >(system_clock::now().time_since_epoch())


using std::chrono::milliseconds;
using std:: chrono ::duration_cast;
using std::chrono::system_clock;

int64_t get_Time(){
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void log(string d){
    printf("[+]%s\n",d.c_str());
}
void log(char* d){
    printf("[+]%s\n",d);
}
void logError(string d){
    printf("[!!]%s\n",d.c_str());
}

void check(int flag , string onSuccess , string onError){
    if(flag < 0)logError(onError);
    else log(onSuccess);
}
/*
	Server has two sockets.
	1. listening socket.s
	2. client socket.
*/


task_queue Q(WIN_SIZE);
pthread_mutex_t qLock = PTHREAD_MUTEX_INITIALIZER;

void ls(task* t);
void download_c(task* t);
void upload_c(task* t);

void* handleConnections(void* arg){

	while(true){
		for (int i = 0;i<WIN_SIZE;++i){
			pthread_mutex_lock(&qLock);
			if(Q.q[i] == nullptr){
				pthread_mutex_unlock(&qLock);
				continue;
			}
			cout<<Q.q[i]->tt<<"\n";
			switch (Q.q[i]->tt)
			{
			case 0:
				ls(Q.q[i]);
				break;
			case 1:
				upload_c(Q.q[i]);
				break;
			case 2:
				
				download_c(Q.q[i]);

				break;
			default:
				break;
			}


			Q.q[i] = nullptr; //delete this later!!!
			pthread_mutex_unlock(&qLock);

		}
	}

}

void clientAccecptor(int server_fd){

	char buffer[1024] = {0};	
	char msg[] = "Message received!\n";
    
    
    while(true){
        int client_fd;
        struct sockaddr_in address;
        int addrlen = sizeof(address);

        check(client_fd = accept(server_fd, (struct sockaddr*)&address,
                    (socklen_t*)&addrlen) , "new client accepted","accept failure");
        
		memset(buffer,'\0',sizeof(buffer));
    
        pthread_mutex_lock(&qLock);
		
		if(Q.current_size == WIN_SIZE){
            
            string msg = "Server task_queue full :( Try again later.";
            send(client_fd, msg.c_str(), msg.length(), 0);
            close(client_fd);
            pthread_mutex_unlock(&qLock);
            continue;
        }
        task* t = (task*)malloc(sizeof(task));
		t->clientFd = client_fd;
        

        int free_idx = -1;
        for(int i = 0;i<WIN_SIZE;++i){
            if(Q.q[i] == NULL){
                free_idx = i;
				Q.q[free_idx] = t;
                break;
            }
        }

		read(client_fd, buffer, 1024);
		std::cout<<"Client - "<<buffer<<std::endl;

		if(strcmp(buffer,"exit") == 0){
			std::cout<<"exiting connection!\n";
			break;
		}
		else if(strncmp(buffer,"$ls",3) == 0){
			t->tt = 0;
			Q.q[free_idx] = t;
		}
		else if(strncmp(buffer,"$upload",7) == 0){
			std::cout<<"upload";
			std::cout<<"file -"<<&buffer[8]<<std::endl;

			int loc = 7;
			for(int i = 0 ; buffer[i] != -1 ; i++)
				if(buffer[i] == '/')
					loc = i;

			std::cout<<"location : "<<loc<<std::endl;

			t->tt = 1;
			t->file = (&buffer[8]);
			Q.q[free_idx] = t;
		}
		else if(strncmp(buffer,"$download",9) == 0){


			t->tt = 2;
			t->file = (&buffer[10]);
			Q.q[free_idx] = t;

		}
		else
			write(client_fd, msg, strlen(msg));

		

		memset(buffer,'\0',strlen(buffer));

        Q.current_size++;
        log("free idx " + to_string(free_idx));
        pthread_mutex_unlock(&qLock);

		log("after accepting");
        
        
    }
}





int main(){

	pthread_t tid;
    pthread_create(&tid,NULL,&handleConnections,NULL);

	int serv_fd,opt = 1, client_fd;

	char msg[] = "Message received!\n";

	struct sockaddr_in address;
	//structure for handling internet addresses.

	int addr_len = sizeof(address);

	char buffer[1024] = {0};

	serv_fd = socket(AF_INET, SOCK_STREAM, 0);


	if(serv_fd == 0){
		std::cout<<"Error - Socket Creation failed.";
		exit(EXIT_FAILURE);
		//EXIT_FAILURE has value of 8.
	}

	if(setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))

	{

		std::cout<<"Error - problem in setting options for socket.";
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	
	address.sin_port = htons(6001);

	if(bind(serv_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		std::cout<<"Error - problem in binding.";
		exit(EXIT_FAILURE);
	}	

	if(listen(serv_fd, 3) < 0)
	{
		std::cout<<"Error - problem in listening.";
		exit(EXIT_FAILURE);
	}

	//Connection between client and server is established at this point.

	memset(buffer,'\0',sizeof(buffer));

	clientAccecptor(serv_fd);
    pthread_join(tid , NULL);


	return 0;
}


void ls(task* t){

	char buffer[1024] = {0};

	log("inside ls func");
	

	std::ifstream fin;
	fin.open("list.txt");

	while(!fin.eof()){
		fin.getline(buffer, 1024);
		buffer[strlen(buffer)] = '\n';
		std::cout<<buffer<<std::endl;
		write(t->clientFd, buffer, strlen(buffer));
		memset(buffer,'\0',strlen(buffer));
	}

	char terminate[] = "#";

	write(t->clientFd, terminate, strlen(terminate));

	fin.close();
}

void download_c(task* t){
	
	int client_fd=t->clientFd;
	string file=t->file;
	char buffer[1024] = {0};

	int buf = 0, buffer_counter = 0, buf_2;
	std::ifstream fin;

	fin.open(file, std::ios::binary);


	if(!fin){
		buffer[0] = -2;
		write(client_fd, buffer, sizeof(buffer));
		std::cout<<"File not found!"<<std::endl;
	}
	else{

		while((buf = fin.get()) != -1){
			
			if(buffer_counter == 1023){

				buffer[buffer_counter] = 'n';

				write(client_fd, buffer, sizeof(buffer));

				
				read(client_fd, &buf_2, sizeof(buf_2));

				std::cout<<fin.tellg()<<std::endl;				

				buffer[0] = buf;
				buffer_counter = 1;
			}
			else{
				buffer[buffer_counter++] = (char)buf;
			}
		}

		buffer[1023] = 'y';
		
		if(buffer_counter != 1024)
			buffer[buffer_counter++] = 'E';
			
		if(buffer_counter != 1024)
			buffer[buffer_counter++] = 'N';
		
		if(buffer_counter != 1024)
			buffer[buffer_counter++] = 'D';

		std::cout<<buffer[1023]<<std::endl;
		

		write(client_fd, buffer, sizeof(buffer));		

		std::cout<<"transmission ended !"<<std::endl;
	}

	fin.close();
}

void upload_c(task* t){
	int client_fd=t->clientFd;
	string file=t->file;

	std::cout<<"in upload command of client i.e. server downloading"<<std::endl;

	std::cout<<file<<" file"<<std::endl;
	int buffer_counter = 0, buf_2 = 100, end_byte;
	
	char buffer[1024] = {0}, search[] = "END";

	std::ofstream fout;

	read(client_fd, buffer, sizeof(buffer));

	if(buffer[0] == -2)
		std::cout<<"No File exist!"<<std::endl;
	else{
		
		fout.open(file);

		while(buffer[1023] == 'n'){

			fout.put((char)buffer[buffer_counter++]);


			if(buffer_counter == 1023){
				write(client_fd, &buf_2, sizeof(buf_2));
				read(client_fd, buffer, sizeof(buffer));				
				buffer_counter = 0;
			}
		}

		if(buffer[1023] == 'E')
			end_byte = 1022;

		else if(buffer[1023] == 'N')
			end_byte = 1021;

		else if(buffer[1023] == 'D')
			end_byte = 1020;

		else{
			bool cont = true;
			int i;

			while(cont){

				if(buffer[buffer_counter++] == 'E'){
					
					end_byte = buffer_counter - 1;

					for(i = 1 ; i < 3 && search[i] == buffer[buffer_counter++] ; i++);

					if(i == 3)
						cont = false;
				}
			}

			buffer_counter = 0;
		}

		if(end_byte == -1)
			end_byte++;

		while(buffer_counter != end_byte){
			
			fout.put((char)buffer[buffer_counter++]);
		}

		std::cout<<(int)buffer[buffer_counter]<<" this"<<std::endl;

		std::cout<<"file received !"<<std::endl;
		fout.close();
	}



}
