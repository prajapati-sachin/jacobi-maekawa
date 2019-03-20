#include "types.h"
#include "stat.h"
#include "user.h"

int readINT(int fd){
	char c;
	int num=0;
	while(1){
		read(fd, &c, 1);
		// printf(1, "%c\n", c );
		if(c=='\n') break;
		num=num*10 + (c-'0');
	}
	return num;
}


float readFLOAT(int fd){
	char c;
	float num=0;
	float deci=10.0;
	int swtch = 0;
	while(1){
		read(fd, &c, 1);
		// temp = c -'0';
		// printf(1, "%c\n", c );		
		if(c=='\n' || c = EOF){ 
			// printf(1, "Exiting now\n"); 
			break;
		}
		else if(c=='.'){
			swtch=1;
		}
		else{
			if(swtch==0){
				num = num*10 + (c-'0');
				// printf(1, "After Num sw=0: %d\n", (int)(num*100000));
			}
			else if(swtch==1){
				float temp = (c-'0');
				// printf(1, "\nSWITCH CALLED\n" );
				// printf(1, "Num : %d\n", (int)num);
				// printf(1, "Temp: %d\n", (int)temp);		
				// printf(1, "Deci: %d\n", (int)deci);		
				num = num + (temp/deci);
				deci = deci*10;
				// printf(1, "After Num: %d\n", (int)(num*100000));
			
			}
		}
		// printf(1, "%d\n", (int)num*100 );

	}
	// printf(1, "Exiting: %d\n", (int)num*1000000 );
	return num;
}

int
main(int argc, char *argv[])
{
	if(argc< 2){
		printf(1,"Need input filename\n");
		exit();
	}

	char *filename;
	filename=argv[1];
	// int type = atoi(argv[1]);
	printf(1,"Filename is %s\n", filename);

	int fd = open(filename, 0);

	int N = readINT(fd); 
	float E = readFLOAT(fd); 
	float T = readFLOAT(fd); 
	int P = readINT(fd); 
	int L = readINT(fd);



    printf(1, "%d\n", N);
    printf(1, "%d\n", (int)(E*1000000));
    printf(1, "%d\n", (int)(T*100));
    printf(1, "%d\n", P);
    printf(1, "%d\n", L);


  	close(fd);

	exit();
}
