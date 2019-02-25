#include "types.h"
#include "stat.h"
#include "user.h"


void handler(void* msg){
	printf(1,"I am handler\n");
	return;
}

// int c= 0;
int main(void)
{
	printf(1,"%s\n","Multi Test case");
	
	int cid = fork();
	if(cid==0){
		printf(1,"%s\n","I am child");
		
		sig_set((signal_handler)&handler);
		// printf(1,"Hanlder is all set\n" );
		// sleep(20);
		sig_pause();
		// printf(1,"Hanlder is all set\n" );

		// printf(1,"Child is exiting now\n" );
		exit();

	}else{
		printf(1,"%s\n","I am parent");

		int arr[1] = {cid}; 
		char *msg_child = (char *)malloc(MSGSIZE);
		msg_child = "Great\n";
		
		sleep(20);
		send_multi(getpid(),arr,msg_child, 1);
		// // c=1;
		// printf(1,"Message Sent to child\n");
		free(msg_child);
		wait();
		
	
	}
	
	exit();
}
