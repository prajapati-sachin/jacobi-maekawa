#include "types.h"
#include "stat.h"
#include "user.h"


int main(void)
{
	printf(1,"%s\n","IPC Test case");
	
	int cid = fork();
	if(cid==0){
		// This is child
		char *msg = (char *)malloc(MSGSIZE);
		int stat=-1;
		int count=0;
		// int a = 0;
		// for(int i=0;i<1e7;i++){
		// 	if(a%5==1) a +=4;
		// 	else a+= 1;
		// }
		while(stat==-1){
			stat = recv(msg);
			count++;
		}
		printf(1,"COOO: %d\n",count );
		printf(1,"A = , 2 CHILD: msg recv is: %s \n", msg );

		exit();

	}else{
		// This is parent
		char *msg_child = (char *)malloc(MSGSIZE);
		msg_child = "Pakaka";
		int a = 0;
		for(int i=0;i<1e7;i++){
			if(a%5==1) a +=4;
			else a+= 1;
		}
		send(getpid(),cid,msg_child);	
		printf(1,"1 PARENT: msg sent is: %s \n", msg_child );
		
		free(msg_child);
	}
	
	exit();
}
