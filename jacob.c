#include "types.h"
#include "stat.h"
#include "user.h"

int readINT(int fd){
	char c;
	int num=0;
	while(1){
		if(read(fd, &c, 1)<=0) break;
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
		if(c=='\n'){ 
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

float fabsm(float a){
	if(a<0)
	return -1*a;
return a;
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
	// printf(1,"Filename is %s\n", filename);

	int fd = open(filename, 0);

	int N = readINT(fd); 
	float E = readFLOAT(fd); 
	float T = readFLOAT(fd); 
	int P = readINT(fd); 
	int L = readINT(fd);
  	close(fd);


  	//For memory trap
	if(P==8 && N==20) P-=1;

	int c_ids[P];
	int parent_pid;
	int this_child_id=0;


	// Initialising Pipes(total 4N-2)		
	int pc_pipe[P][2];
	int cp_pipe[P][2];
	int cc_up[P-1][2];
	int cc_down[P-1][2];


	int i,j;
	float mean;
	float u[N][N];
	float w[N][N];
	mean = 0.0;
	
	// Making pipes
	for(int i=0;i<P;i++){
		if(pipe(pc_pipe[i])<0) printf(1, "Error creating pipe\n");
		if(pipe(cp_pipe[i])<0) printf(1, "Error creating pipe\n");
	}
	for(int i=0;i<P-1;i++){
		if(pipe(cc_up[i])<0) printf(1, "Error creating pipe\n");
		if(pipe(cc_down[i])<0) printf(1, "Error creating pipe\n");
	}

	//Init array values
	for (i = 0; i < N; i++){
		u[i][0] = u[i][N-1] = u[0][i] = T;
		u[N-1][i] = 0.0;
		mean += u[i][0] + u[i][N-1] + u[0][i] + u[N-1][i];
	}
	mean /= (4.0 * N);
	for (i = 1; i < N-1; i++ ){
		for ( j= 1; j < N-1; j++) u[i][j] = mean;
	}

	if(N<=2){
		for(int i=0;i<N;i++){
			for(int j=0; j<N;j++){
				printf(1, "%d ", (int)u[i][j]);
			}
			printf(1, "\n");
		}
		exit();
	}

	int works[P];
	int start_indices[P];
	int end_indices[P];

	for(int i=0;i<P;i++){
		works[i]=0;
	}

	for(int i=0;i<(N-2);i++){
		works[i%P]++;
	}

	// if((N-2)<0) works[0] = N-2;

	start_indices[0]=1;
	end_indices[0]=works[0];

	int unused=0;

	for(int i=1;i<P;i++){
		start_indices[i] = end_indices[i-1]+1;
		end_indices[i] = start_indices[i] + works[i]-1;
		if(start_indices[i]>end_indices[i]) unused+=1;
	}

	parent_pid = getpid();	

	// for(int i=0;i<P;i++){
	// 	printf("works[%d] = %d\n",i, works[i]);
	// 	printf("Child: %d- Start= %d, End= %d\n", i, start_indices[i], end_indices[i]);
	// 	start_indices[i] = end_indices[i-1]+1;
	// 	end_indices[i] = start_indices[i] + works[i]-1;
	// }	

	// printf("%d\n", unused);

	P -= unused;

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
			close(cc_down[i][0]);
			close(cc_down[i][1]);
		}

		//closing unnnecary parent to child pipe for parent pipes: closing p->c(read) and c->p(write)
		for(int i=0;i<P-1;i++){
			close(pc_pipe[i][0]);
			close(cp_pipe[i][1]);
		}

		// for(int i=0; i<P; i++) printf("Child No: %d= %d\n", i, c_ids[i]);

		float recv_diff[P];
		int count=0;
		int carryon=0;
		int done=1;

  		for(;;){
  			//Recv diff(max diff) from each process  		
  			for(int i=0;i<P;i++){
  				read(cp_pipe[i][0], &recv_diff[i], 4);
  			}
  			count++;
  			// for(int i=0;i<P;i++){
  			// 	printf("%f\n", recv_diff[i]);
  			// }
  			// printf("Stuck herre\n" );
  			//Find the maximum of all received
  			float max_dif = 0;
  			for(int i=0;i<P;i++){
  				if(recv_diff[i]>max_dif) max_dif=recv_diff[i];
  			}

  			// printf("Max diff: %f\n", max_dif);
  			//check exiting condition
  			if(max_dif<=E || count>L){
  				// printf("exit\n");
  				//signal childs for exiting
	  			for(int i=0;i<P;i++){
					write(pc_pipe[i][1], &done, 4);

	  			}
  				// printf(1, "%d\n",count );
				//Receive from P process
				for(int i=0;i<P;i++){
					int start_index = start_indices[i];
					int end_index = end_indices[i];;
					// if(i==P-1) {end_index= N-1;}
					// else {end_index = start_index + ((N-2)/P);}

					for(int j=start_index; j<=end_index; j++){
						// int values[N];
						read(cp_pipe[i][0], u[j], 4*N);
						// for(int k=0;k<N;k++) u[j][k] = values[k];
					}
				}

				for(int i=0;i<N;i++){
					for(int j=0; j<N;j++){
						printf(1, "%d ", (int)u[i][j]);
					}
					printf(1, "\n");
				}

				for(int i=0;i<P-1;i++){
					close(pc_pipe[i][1]);
					close(cp_pipe[i][0]);
				}

			    for(int i=0;i<P;i++) wait();	
				exit();

  			}  			 			

  			//else signal each process to carry on
  			// printf("%d\n", carryon);
  			for(int i=0;i<P;i++){
					write(pc_pipe[i][1], &carryon, 4);
  			}
  		}
		
  		
  	}
	//Childs(keep calculating the values)
	else{
		// closing unnnecary parent to child pipe for child pipes
		for(int i=0;i<P;i++){
			close(pc_pipe[i][1]);
			close(cp_pipe[i][0]);
			if(i==this_child_id) continue;
			close(pc_pipe[i][0]);
			close(cp_pipe[i][1]);
		}

		//closing unnnecary parent to child pipe for parent pipes: closing p->c(read) and c->p(write)
		for(int i=0;i<P-1;i++){
			if(i==this_child_id || i==this_child_id-1) continue;
			close(cc_up[i][0]);
			close(cc_up[i][1]);
			close(cc_down[i][0]);
			close(cc_down[i][1]);
		}

		if(this_child_id-1>=0){
			close(cc_up[this_child_id-1][0]);			
			close(cc_down[this_child_id-1][1]);			
			// write(cc_up[this_child_id-1][1], u[start_index], 4*N);
		}


		if(this_child_id+1<P){
			close(cc_up[this_child_id][1]);			
			close(cc_down[this_child_id][0]);			
			// write(cc_up[this_child_id-1][1], u[start_index], 4*N);
		}	

		//row wise division
		int start_index = start_indices[this_child_id];
  		int end_index = end_indices[this_child_id];
  		// if(this_child_id==P-1) {end_index= N-1;}
  		// else {end_index = start_index + ((N-2)/P);}

  		if(start_index>end_index) exit();


		// int upperpid = this_child_id-1;
		// int lowerpid = this_child_id+1;
		// if(this_child_id==0) upperpid=0;
		// if(this_child_id==P-1) lowerpid=0;

		// Variable to receive ghost values
		// float ghost_above[N];
		// float ghost_below[N];
		float diff;
		int order;


		for(;;){
			diff=0.0;
			
			// printf("Stuck herre\n" );
			// if(this_child_id==4)
			// 	for(int i=0;i<N;i++) printf("%f ", u[start_index][i]); printf("\n");;
			
			//Sending ghost values to upper and lower process
			if(this_child_id-1>=0){
				write(cc_up[this_child_id-1][1], u[start_index], 4*N);
			}
			if(this_child_id+1<P){
				write(cc_down[this_child_id][1], u[end_index], 4*N);	
			}

			// printf("Child Num: %d\n", this_child_id);

			// //Receiving ghost values from upper and lower process
			// if(this_child_id-1>=0){
			// 	read(cc_down[this_child_id-1][0], ghost_above, 4*N);
			// }
			// if(this_child_id+1<P){
			// 	read(cc_up[this_child_id][0], ghost_below, 4*N);
			// }			
			//Receiving ghost values from upper and lower process
			if(this_child_id-1>=0){
				read(cc_down[this_child_id-1][0], u[start_index-1], 4*N);
			}
			if(this_child_id+1<P){
				read(cc_up[this_child_id][0], u[end_index+1], 4*N);
			}

			// if(this_child_id==0)
				// for(int i=0;i<N;i++) printf("Received ghost values: %f\n", ghost_below[i]);

			// //Calculate new values
			// if(this_child_id==0){
			// 	for(int i=start_index;i<=end_index;i++){
			// 		for(int j=1;j<N-1;j++){
			// 			if(start_index==end_index) w[i][j] = (u[i-1][j] + ghost_below[j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 			else{
			// 				if(i==start_index) w[i][j] = (u[i-1][j] + u[i+1][j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 				else if(i==end_index) w[i][j] = (u[i-1][j] + ghost_below[j] + u[i][j-1] + u[i][j+1])/4.0;
			// 				else w[i][j] = (u[i-1][j] + u[i+1][j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 				if(fabsm(w[i][j]-u[i][j])> diff) diff = fabsm(w[i][j]-u[i][j]); 
			// 			}
			// 		}
			// 	}
			// }
			// else if(this_child_id==P-1){
			// 	for(int i=start_index;i<=end_index;i++){
			// 		for(int j=1;j<N-1;j++){
			// 			if(i==start_index) w[i][j] = (ghost_above[j] + u[i+1][j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 			else if(i==end_index) w[i][j] = (u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1])/4.0;
			// 			else w[i][j] = (u[i-1][j] + u[i+1][j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 			if(fabsm(w[i][j]-u[i][j])> diff) diff = fabsm(w[i][j]-u[i][j]); 
			// 		}
			// 	}
			// }
			// else{
			// 	for(int i=start_index;i<=end_index;i++){
			// 		for(int j=1;j<N-1;j++){
			// 			if(start_index==end_index) w[i][j] = (ghost_above[j] + ghost_below[j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 			else{
			// 				if(i==start_index) w[i][j] = (ghost_above[j] + u[i+1][j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 				else if(i==end_index) w[i][j] = (u[i-1][j] + ghost_below[j] + u[i][j-1] + u[i][j+1])/4.0;
			// 				else w[i][j] = (u[i-1][j] + u[i+1][j]+ u[i][j-1] + u[i][j+1])/4.0;
			// 				if(fabsm(w[i][j]-u[i][j])> diff) diff = fabsm(w[i][j]-u[i][j]); 
			// 			}
			// 		}
			// 	}
			// }
			// diff = 0.0;
			for(i =start_index ; i <= end_index; i++){
				for(j =1 ; j < N-1; j++){
					w[i][j] = ( u[i-1][j] + u[i+1][j]+
						    u[i][j-1] + u[i][j+1])/4.0;
					if( fabsm(w[i][j] - u[i][j]) > diff )
						diff = fabsm(w[i][j]- u[i][j]);	
				}
			}


			// printf("Diff from child %d: %f\n", this_child_id, diff);
			//Copy the value to u
			for(int i=start_index;i<=end_index;i++)
				for(int j=1;j<N-1;j++) u[i][j]=w[i][j];




			//Send the value of diff to parent
			write(cp_pipe[this_child_id][1], &diff,  4);

			//Recv order from parent
			read(pc_pipe[this_child_id][0], &order, 4);
			// printf("Order for child %d: %d\n", this_child_id, order);

			
			//Done signal send all values to parent and exit
			if(order==1){
				// if(this_child_id==1){
				// 	for(int i=start_index;i<=end_index;i++){
				// 		for(int j=0;j<N;j++){
				// 			printf("%d ", (int)u[i][j]);
				// 		}
				// 		printf("\n");
				// 		// write(cp_pipe[this_child_id][1], u[i], 4*N);
				// 	}
				// }
				for(int i=start_index;i<=end_index;i++){
					write(cp_pipe[this_child_id][1], u[i], 4*N);
				}
				// Closing opened pipes
				close(pc_pipe[this_child_id][0]);
				// close(pc_pipe[this_child_id][1]);
				// close(cp_pipe[this_child_id][0]);
				close(cp_pipe[this_child_id][1]);

			
				if(this_child_id-1>=0){
					close(cc_up[this_child_id-1][1]);			
					close(cc_down[this_child_id-1][0]);			
					// write(cc_up[this_child_id-1][1], u[start_index], 4*N);
				}


				if(this_child_id+1<P){
					close(cc_up[this_child_id][0]);			
					close(cc_down[this_child_id][1]);			
					// write(cc_up[this_child_id-1][1], u[start_index], 4*N);
				}	

				// close(cc_up[this_child_id][0]);
				// close(cc_up[this_child_id][1]);
				// close(cc_up[this_child_id-1][0]);
				// close(cc_up[this_child_id-1][1]);

				// close(cc_down[this_child_id][0]);
				// close(cc_down[this_child_id][1]);
				// close(cc_down[this_child_id-1][0]);
				// close(cc_down[this_child_id-1][1]);

				exit();
			}			
		}

		exit();
	}

// finalstep:
// 	//Receive from P process
// 	for(int i=0;i<P;i++){
// 		int start_index = start_indices[i];
// 		int end_index = end_indices[i];;
// 		// if(i==P-1) {end_index= N-1;}
// 		// else {end_index = start_index + ((N-2)/P);}

// 		for(int j=start_index; j<=end_index; j++){
// 			// int values[N];
// 			read(cp_pipe[i][0], u[j], 4*N);
// 			// for(int k=0;k<N;k++) u[j][k] = values[k];
// 		}
// 	}

// 	for(int i=0;i<N;i++){
// 		for(int j=0; j<N;j++){
// 			printf("%d ", (int)u[i][j]);
// 		}
// 		printf("\n");
// 	}

// 	for(int i=0;i<P-1;i++){
// 		close(pc_pipe[i][1]);
// 		close(cp_pipe[i][0]);
// 	}

//     for(int i=0;i<P;i++) wait(NULL);	
// 	exit(1);





	exit();
}
