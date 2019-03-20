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
int recv_tokens[P];
int ghost_above[N];
int ghost_below[N];
//1 to carry on
//2 to exit
int permission[P];

int main(int argc, char *argv[])
{
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
	//Init
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
  		for(;;){
  			//Recv tokens(max diff) from each process
  			for(int i=0;i<P;i++){
  				recv((void*)&recv_tokens[i]);
  				// recv_tokens[i]=0;
  			}
  			count++;
  			//Find the maximum of all received
  			float max_dif = 0;
  			for(int i=0;i<P;i++){
  				if(recv_tokens[i]>max_dif) max_dif=recv_tokens[i];
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
		//row wise division
		int start_index = ((N-2)/P)*this_child_id+1;
  		int end_index;
  		if(this_child_id==CHILDS-1) {end_index= N-1;}
  		else {end_index = start_index + ((N-2)/P);}
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
