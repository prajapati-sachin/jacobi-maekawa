#include "types.h"
#include "stat.h"
#include "user.h"


int main(void)
{
	printf(1,"%s\n","IPC Test case");
	
	int cid = fork();
	if(cid==0){
		// This is child
		char *msg1 = (char *)malloc(MSGSIZE);
		// char *msg2 = (char *)malloc(MSGSIZE);
		int stat=-1;
		// int count=0;
		// int a = 0;
		// for(int i=0;i<1e7;i++){
		// 	if(a%5==1) a +=4;
		// 	else a+= 1;
		// }
		while(stat==-1){
			stat = recv(msg1);
			// count++;
		}
		// stat=-1;
		// while(stat==-1){
		// 	stat = recv(msg2);
		// 	// count++;
		// }
		// printf(1,"COOO: %d\n",count );
		printf(1,"2 CHILD: msg recv is: %s \n", msg1 );
		// printf(1,"2 CHILD: msg2 recv is: %s \n", msg2 );
		// printf(1,"A: %d \n", a );

		exit();

	}else{
		// This is parent
		char *msg_child1 = (char *)malloc(MSGSIZE);
		msg_child1 = "P";
		// char *msg_child2 = (char *)malloc(MSGSIZE);
		// msg_child2 = "masg2";
		// int a = 0;
		// for(int i=0;i<1e7;i++){
		// 	if(a%5==1) a +=4;
		// 	else a+= 1;
		// }
		send(getpid(),cid,msg_child1);	
		// send(getpid(),cid,msg_child2);	
		printf(1,"1 PARENT: msg sent is: %s \n", msg_child1);
		// printf(1,"1 PARENT: msg2 sent is: %s \n", msg_child2 );
		// printf(1,"A: %d \n", a );
		
		free(msg_child1);
		// free(msg_child2);
	}
	
	exit();
}
