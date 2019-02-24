#include "types.h"
#include "stat.h"
#include "user.h"


int main(void)
{
	printf(1,"%s\n","MUl Test case");
	int sp = 7;
	int rpd[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	char *msg_child1 = (char *)malloc(MSGSIZE);
	msg_child1 = "Pakap";
	int length = 8;
	send_multi(sp,  rpd, msg_child1, length);
	// add(3,4);
	printf(1, "%s\n","done" );	
	free(msg_child1);
	exit();
}
