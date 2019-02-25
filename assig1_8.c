#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_CHILD 8
int sum = 0;
volatile int child_no;
int cid[NUM_CHILD];
volatile int parent_id;
int          msg[2];
float         mean[2]; 
volatile int fun_called[NUM_CHILD];

void get_mean( void* msg ){
	mean[0] = *(float*)msg;
	fun_called[child_no] = 1;
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
	sig_set(&get_mean);

	int i = 0, parent_id = getpid();
	for( i = 0; i < NUM_CHILD; i++ ){
		cid[i] = fork();
		if( cid[i] == 0 ){
			child_no = i; break;
		}
	}
	// parent
	if( getpid() == parent_id ){
		for( i = 0; i < NUM_CHILD; i++ ){
			recv(msg); tot_sum += msg[0];
		}
		mean[0] = ((float)tot_sum / size);
		if( type == 1 ){
			send_multi( parent_id, cid, mean, NUM_CHILD );
			for( i = 0; i < NUM_CHILD; i++ ){
				recv(mean); variance += mean[1];
			}
			variance /= size;
		}
		
	}else{
		int start = (size/NUM_CHILD)*child_no;
		int end   = (child_no == NUM_CHILD - 1)?(size):(size/NUM_CHILD)*(child_no + 1);
		for( i = start; i < end; i++ ){
			msg[0] += arr[i];
		}
		send( getpid(), parent_id, msg );
		if(type == 0) exit();

		if(fun_called[child_no] == 0) sig_pause();
		for( i = start; i < end; i++ ){
			mean[1] += (arr[i] - mean[0])*(arr[i] - mean[0]);
		}
		send( getpid(), parent_id, mean );
		exit();
	}
	for( i = 0; i < NUM_CHILD; i++ )
		wait();
  	//------------------

  	if(type==0){ //unicast sum
		printf(1,"Sum of array for file %s is %d\n", filename,tot_sum);
	}
	else{ //mulicast variance
		printf(1,"Variance of array for file %s is %d\n", filename,(int)variance);
	}
	exit();
}