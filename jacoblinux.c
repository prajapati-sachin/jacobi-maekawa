#include "types.h"
#include "stat.h"
#include "user.h"


#define N 11
#define E 0.00001
#define T 100.0
#define P 6
#define L 20000

float fabsm(float a){
	if(a<0)
	return -1*a;
return a;
}


int c_ids[P];
volatile int parent_pid;
volatile int this_child_id;
int recv_tokens[P][2];
int ghost_above[N];
int ghost_below[N];
//1 to carry on
//2 to exit
int permission[P];

int main(int argc, char *argv[])
{
//	Taking input from inp file //////////////////////////

	// if(argc< 2){
	// 	printf(1,"Need input filename\n");
	// 	exit();
	// }
	// char *filename;
	// filename=argv[1];
	// int fd = open(filename, 0);
	// char c;
	// read(fd, &c, 1);
	// N = c-'0';
	// read(fd, &c, 1);
	// E = c-'0';
	// read(fd, &c, 1);
	// T = c-'0';
	// read(fd, &c, 1);
	// P = c-'0';
	// read(fd, &c, 1);
	// L = c-'0';
 //  	close(fd);

/////////////////////////////////////////////////////////
//	Initialising Pipes(total 4N-2)		
	int pc_pipe[P][2];
	int cp_pipe[P][2];
	int cc_up[P-1][2];
	int cc_down[P-1][2];


	float diff;
	int i,j;
	float mean;
	float u[N][N];
	float w[N][N];
	int *carryon;
	*carryon=1;
	int *exit;
	*exit=1;
	int count=0;
	mean = 0.0;
	
	//Making pipes
	for(int i=0;i<P;i++){
		if(pipe(pc_pipe[i])<0) printf("Error creating pipe\n");
		if(pipe(cp_pipe[i])<0) printf("Error creating pipe\n");
	}
	for(int i=0;i<P-1;i++){
		if(pipe(cc_up[i])<0) printf("Error creating pipe\n");
		if(pipe(cp_down[i])<0) printf("Error creating pipe\n");
	}

	//Init array values
	for (i = 0; i < N; i++){
		u[i][0] = u[i][N-1] = u[0][i] = T;
		u[N-1][i] = 0.0;
		mean += u[i][0] + u[i][N-1] + u[0][i] + u[N-1][i];
	}
	mean /= (4.0 * N);
	for (i = 1; i < N-1; i++ )
		for ( j= 1; j < N-1; j++) u[i][j] = mean;

	//Setting initial token values
  	for(int i=0;i<P;i++){
		recv_tokens[i]=0;
	}

	parent_pid = getpid();
	

  	//creating n childs
  	for(int i=0;i<P;i++){
  		c_ids[i]=fork();
  		if(c_ids[i]==0){
  			this_child_id = i;
  			break;	
  		} 
  	}

  	//Parent(implements barrier)
  	if(getpid()==parent_pid){
		//closing unnnecary child to child pipe for parent pipes
		for(int i=0;i<P-1;i++){
			close(cc_up[i][0]);
			close(cc_up[i][1]);
			close(cp_down[i][0]);
			close(cp_down[i][1]);
		}

		//closing unnnecary parent to child pipe for parent pipes: closing p->c(read) and c->p(write)
		for(int i=0;i<P-1;i++){
			close(pc_pipe[i][0]);
			close(cp_pipe[i][1]);
		}

		int address[P][2];
		// Sending the address of all neighbours to whom to send ghost values
		for(int i=0;i<P;i++){
			if(i==0){
				address[i][1]=0;
				address[i][2]=c_ids[i+1];
				send(parent_pid, c_ids[i], address[i]);
			}
			else if(i==P){
				address[i][1]=c_ids[i-1];
				address[i][2]=0;
				send(parent_pid, c_ids[i], address[i]);  					
			}
			else{
				address[i][1]=c_ids[i-1];
				address[i][2]=c_ids[i+1];
				send(parent_pid, c_ids[i], address[i]);
			}
		}
  		for(;;){

  			//Recv tokens(max diff) from each process
  			for(int i=0;i<P;i++){
  				recv((void*)recv_tokens[i]);
  				// recv_tokens[i]=0;
  			}
  			count++;

  			//Find the maximum of all received
  			float max_dif = 0;
  			for(int i=0;i<P;i++){
  				if(recv_tokens[i][1]>max_dif) max_dif=recv_tokens[i][1];
  			}

  			//check exiting condition
  			if(max_dif<=E || count>L) {
  				//signal childs for exiting
	  			for(int i=0;i<P;i++){
	  				send(parent_pid, c_ids[i], (void*)exit);
	  			}
  				goto done;
  			}  			 			

  			//else signal each process to carry on
  			for(int i=0;i<P;i++){
  				send(parent_pid, c_ids[i], (void*)carryon);
  			}
  		}
  	}
	//Childs(keep calculating the values)
	else{
		// int upperpid=0;
		// int lowerpid=0;

		//row wise division
		int start_index = ((N-2)/P)*this_child_id+1;
  		int end_index;
  		if(this_child_id==P-1) {end_index= N-2;}
  		else {end_index = start_index + ((N-2)/P);}

  		// Recv the address of negibours, neighbours[0] is the upperpid and negihbours[1] is the lowerpid, 0 in case they are not present 
  		int negibours[2];
  		recv((void*)negibours);
  		// upperpid = negibours[1];
  		// lowerpid = negibours[2];

  		// Send the ghost values to upper and lower process
  		for(int i=0;i<2;i++){
			if(negibours[i]==0) continue;  				
  			for(int k=1;k<N-1;k++){
  				int tosend = u[][]
  				send(getpid(), negibours[i], )
  			}
  		}


  		float diff;
  		for(;;){
  			diff=0;
  			for(i =start_index ; i < end_index; i++){
  				for(j =1 ; j < N-1; j++){
  					w[i][j] = ( u[i-1][j] + u[i+1][j]+
  						    u[i][j-1] + u[i][j+1])/4.0;
  					if( fabsm(w[i][j] - u[i][j]) > diff )
  						diff = fabsm(w[i][j]- u[i][j]);	
  				}
  			}
  			//send diff to parent
  			send(getpid(), parent_pid, (void*)&diff);
  			//recv barrier permission from parent
  			recv((void*)&permission[this_child_id]);
  			//check for exiting condition
  			if(permission[this_child_id]==2) exit();  			
  		}

	}

	// for(;;){
	// 	diff = 0.0;
	// 	for(i =1 ; i < N-1; i++){
	// 		for(j =1 ; j < N-1; j++){
	// 			w[i][j] = ( u[i-1][j] + u[i+1][j]+
	// 				    u[i][j-1] + u[i][j+1])/4.0;
	// 			if( fabsm(w[i][j] - u[i][j]) > diff )
	// 				diff = fabsm(w[i][j]- u[i][j]);	
	// 		}
	// 	}
	//     count++;
	       
	// 	if(diff<= E || count > L){ 
	// 		break;
	// 	}
	
	// 	for (i =1; i< N-1; i++)	
	// 		for (j =1; j< N-1; j++) u[i][j] = w[i][j];
	// }
	// for(i =0; i <N; i++){
	// 	for(j = 0; j<N; j++)
	// 		printf(1,"%d ",((int)u[i][j]));
	// 	printf(1,"\n");
	// }

done:
    for(int i=0;i<P;i++) wait();	
	exit();

}
