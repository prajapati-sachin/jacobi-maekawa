#include "types.h"
#include "stat.h"
#include "user.h"
#define NP 6
#define MSGSIZE 8
volatile int fun_called;
// int f_called[NP] = {1,1}
int cid[NP];
int child_no, i;

void fun(void *a){
	int *b = (int *)a;
	fun_called = 1;
	printf(1, "Child %dth : %d, %d\n",i,b[0],b[1]);
}

void s_handler(void* msg){
	// arrived [1] = 1;
	char *msg_recv = (char *)malloc(MSGSIZE);
	memmove(msg_recv,msg,MSGSIZE);
	// uint aa = (uint)&sender_pid;
	// printf(1,"2 CHILD: msg recv by1: %d \n", sender_pid) ;
	fun_called = 1;
	
	printf(1,"2 CHILD: msg recv by: %d is: %s \n",getpid() ,msg_recv );
}

int main(void)
{	
	sig_set(&s_handler);

	for(i=0;i<NP;i++)
		cid[i] = -1;

	for(i=0;i<NP;i++){
		cid[i] = fork();
		if(cid[i] == 0){break;}
	}


	if(cid[NP-1] <= 0){
		if(fun_called == 0){ 
			// printf(1,"%dth fun\n",i);
			sig_pause();
			// printf(1,"%dth fun\n",i);
		}
		printf(1,"%dth child ended\n",i);
		exit();
	}else{
		// This is parent

		// int a[NP][2];
		// for(i=0;i<NP;i++)
		// 	a[i][0] = i+1, a[i][1] = -(i+1);
		char *msg_child = (char *)malloc(MSGSIZE);
		// *(as+1) = 22220;
		msg_child = "PPP";
		// send_multi(getpid(),cid,msg_child,4)
		// for(int k=0;k<NP;k++)
		// 	signal_send(cid[k],2,msg_child);

		send_multi(getpid(),cid,msg_child,6);
		printf(1,"1 PARENT2: msg sent by:%d is: %s \n",getpid(), msg_child );

		// for(i=0;i<NP;i++){


		// 	signal_send(cid[i], 0, (void *)msg_child);
			
		// 	sleep(20);
		// }
		free(msg_child);
		for(i=0;i<NP;i++)
			wait();
	}
	
	exit();
}