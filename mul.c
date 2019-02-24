#include "types.h"
#include "stat.h"
#include "user.h"


void handler(void* msg){
	printf(1,"I am handler\n");
}

int main(void)
{
	printf(1,"%s\n","IPC Test case");
	
	int cid = fork();
	if(cid==0){
		sig_set((signal_handler)&handler);
		printf(1,"Hanlder is all set\n" );
		
		// This is child

		sleep(20);
		printf(1,"Child is exiting now" );
		exit();

	}else{
		// This is parent
		// int a = 0;
		// for(int i=0;i<1e7;i++){
		// 	if(a%5==1) a +=4;
		// 	else a+= 1;
		// }
		// printf(1,"Waiting A: %d \n", a);
		
		sig_send(cid, 1);
		printf(1,"Signal Sent to child");
		
	}
	
	exit();
}
