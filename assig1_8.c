#include "types.h"
#include "stat.h"
#include "user.h"

#define CHILDS 8

int c_ids[CHILDS];
volatile int parent_pid;
volatile int this_child_id;
float mean_sent;
int recv_psums[CHILDS];
float recv_pvar[CHILDS];
float mean_received;
volatile int sreceived[CHILDS];
int partial_sum;
float partial_variance;


void message_handler(void* msg){
	//Message received is 8 bytes
	// memmove((void*)&mean_received, msg, 8);
	mean_received = *(float*)msg;
  sreceived[this_child_id] = 1;
	return;
}

int
main(int argc, char *argv[])
{
	if(argc< 2){
		printf(1,"Need type and input filename\n");
		exit();
	}
	char *filename;
	filename=argv[2];
	int type = atoi(argv[1]);
	printf(1,"Type is %d and filename is %s\n",type, filename);

	int tot_sum = 0;	
	float variance = 0.0;

	int size=1000;
	short arr[size];
	char c;
	int fd = open(filename, 0);
	for(int i=0; i<size; i++){
		read(fd, &c, 1);
		arr[i]=c-'0';
		read(fd, &c, 1);
	}	
  	close(fd);
  	// this is to supress warning
  	printf(1,"first elem %d\n", arr[0]);
  
  	//----FILL THE CODE HERE for unicast sum and multicast variance
  	sig_set((signal_handler)&message_handler);
  	for(int i=0;i<CHILDS;i++) c_ids[i] = -1;
  	parent_pid = getpid();
  	partial_sum=0;
    partial_variance=0;
    
  	// arr_sum=0;  
    for(int i=0;i<CHILDS;i++) sreceived[i]=0;
  	
  	//creating n childs
  	for(int i=0;i<CHILDS;i++){
  		c_ids[i]=fork();
  		if(c_ids[i]==0){
  			this_child_id = i;
  			break;	
  		} 
  	}
  	//Parent
  	if(getpid()==parent_pid){
  		//Receive partial sums from childs
  		for(int i=0;i<CHILDS;i++){
  			recv((void*)&recv_psums[i]);
  		  tot_sum+=recv_psums[i];
      }

      printf(1, "tot Sent: %d\n", tot_sum);
  		mean_sent = ((float)tot_sum/size);
      // printf(1, "Mean Sent: %d\n", (int)mean_sent);
      // printf(1, "l Sent: %d\n", (int)l);
      // printf(1, "size Sent: %d\n", size);

  		//send the mean to all child proccess
  		send_multi(parent_pid, c_ids, (void*)&mean_sent, CHILDS);
  		
  		//Receive partial variance from childs
  		for(int i=0;i<CHILDS;i++){
  			recv((void*)&recv_pvar[i]);
        variance+=recv_pvar[i];
  		}

  		variance = variance/size;
  	}
  	//Childs
  	else{
  	 	// int *partial_sum;
  		// float *partial_variance;
  		// *partial_sum = 0;
  		int start_index = (size/CHILDS)*this_child_id;
  		int end_index;
  		if(this_child_id==CHILDS-1) {end_index= size;}
  		else {end_index = start_index + (size/CHILDS);}
  		
      for(int i=start_index;i<end_index;i++){
  			(partial_sum)+=arr[i];
  		}
  		send(getpid(), parent_pid, (void*)&partial_sum);

  		//wait for the mean
  		if(sreceived[this_child_id]==0) sig_pause();
			
      // printf(1, "%d\n", (int)mean_received);
      for(int i=start_index;i<end_index;i++){
				(partial_variance)+=(arr[i]-mean_received)*(arr[i]-mean_received);
			}
		  send(getpid(), parent_pid, (void*)&partial_variance);
      exit();
    }

    for(int i=0;i<CHILDS;i++) wait();

 	//------------------

	if(type==0){ //unicast sum
	printf(1,"Sum of array for file %s is %d\n", filename,tot_sum);
	}
	else{ //mulicast variance
		printf(1,"Variance of array for file %s is %d\n", filename,(int)variance);
	}
	exit();
}
