#include<stdio.h>
#include<stdlib.h>
#include "mpi.h"
#define BOUNDARY 0 
int dr[8]={-1,-1,-1,0,0,1,1,1};
int dc[8]={-1,0,1,-1,1,-1,0,1};
int main(int argc, char * argv[]){
	MPI_Init(&argc,&argv);
	int m,curN=0,N,ghost,np,rank,block,np_root,m_original;
	char* send_arr;
	MPI_Comm cart_comm;
	MPI_Status status;
	char** arr;
	int send_message[4],recv_message[4];
	char* recv_arr;
	char** block_arr,**after_block_arr;
	char* send_finish;
	char* recv_finish;
	int block_arr_size;
	int dims[2]={0,0};
	int periods[2]={0,0};
	int coord[2]={0,0};
	int row,col;
	MPI_Request send_req[8]={0},recv_req[8];
	MPI_Status wait_status;
	MPI_Comm_size(MPI_COMM_WORLD,&np);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);


	MPI_Dims_create(np,2,dims);
	MPI_Cart_create(MPI_COMM_WORLD,2,dims,periods,1,&cart_comm);

	if(rank==0){
		scanf("%d",&m);
		scanf("%d",&N);
		scanf("%d",&ghost);
		for(int i=0;i*i!=np;i++){
			np_root=i;
		}
		m_original=m;
		np_root++;
		ghost++;
		m=m+((m%np_root)>0? np_root-m%np_root:0);
//		printf("<<%d %d %d>>",m,m/np_root,m%np_root);
//	}
//	MPI_Finalize(); return 0;
//	if(rank==0){
		arr=(char**)calloc(m,sizeof(char*));
		for(int i=0;i<m;i++){
			arr[i]=(char*)calloc(m+1,sizeof(char));
			scanf("%s",arr[i]);
		}
		block=m/np_root;
		send_arr=(char*)malloc(sizeof(char)*(block*block));
		if(block==0) block=1;
		send_message[0]=m;
		send_message[1]=N;
		send_message[2]=ghost;
		send_message[3]=block;
		for(int i=0;i<np;i++){
			MPI_Send(&send_message,4,MPI_INT,i,1,MPI_COMM_WORLD);
		}
		for(int i=1;i<np;i++){
			MPI_Cart_coords(cart_comm,i,2,coord);
			row=coord[0];
			col=coord[1];
			for(int j=0;j<block;j++){
				for(int k=0;k<block;k++){
					send_arr[j*block+k]=arr[row*block+j][col*block+k];
					//printf("rank : %d i %d j %d  / %d %d %d %d]\n",rank,i,j,row,col,row*block+j,col*block+k);
				}
			}
			MPI_Send(send_arr,block*block,MPI_CHAR,i,2,MPI_COMM_WORLD);
		}
		MPI_Cart_coords(cart_comm,0,2,coord);
		row=coord[0];
		col=coord[1];
		for(int j=0;j<block;j++){
			for(int k=0;k<block;k++){
				send_arr[j*block+k]=arr[row*block+j][col*block+k];
			}
		}
	}
//	MPI_Bcast(&broad_message,4,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Recv(&recv_message,4,MPI_INT,0,1,MPI_COMM_WORLD,&status);
	m=recv_message[0];
	N=recv_message[1];
	ghost=recv_message[2];
	block=recv_message[3];
	recv_arr=(char*)calloc(block*block,sizeof(char));
	block_arr_size=block+ghost*2;
	block_arr=(char**)malloc(sizeof(char*)*block_arr_size);
	after_block_arr=(char**)malloc(sizeof(char*)*block_arr_size);

	char* send_ghost12=(char*)malloc(block*ghost*sizeof(char));
	char* recv_ghost12=(char*)malloc(block*ghost*sizeof(char));
	char* send_ghost22=(char*)malloc(block*ghost*sizeof(char));
	char* recv_ghost22=(char*)malloc(block*ghost*sizeof(char));
	char* send_ghost32=(char*)malloc(block*ghost*sizeof(char));
	char* recv_ghost32=(char*)malloc(block*ghost*sizeof(char));
	char* send_ghost42=(char*)malloc(block*ghost*sizeof(char));
	char* recv_ghost42=(char*)malloc(block*ghost*sizeof(char));

	char* send_ghost13=(char*)malloc(ghost*ghost*sizeof(char));
	char* recv_ghost13=(char*)malloc(ghost*ghost*sizeof(char));
	char* send_ghost23=(char*)malloc(ghost*ghost*sizeof(char));
	char* recv_ghost23=(char*)malloc(ghost*ghost*sizeof(char));
	char* send_ghost24=(char*)malloc(ghost*ghost*sizeof(char));
	char* recv_ghost24=(char*)malloc(ghost*ghost*sizeof(char));
	char* send_ghost14=(char*)malloc(ghost*ghost*sizeof(char));
	char* recv_ghost14=(char*)malloc(ghost*ghost*sizeof(char));
	for(int i=0;i<block_arr_size;i++){
		block_arr[i]=(char*)calloc(block_arr_size,sizeof(char));
		after_block_arr[i]=(char*)calloc(block_arr_size,sizeof(char));
	}
	if(rank!=0)MPI_Recv(recv_arr,block*block,MPI_CHAR,0,2,MPI_COMM_WORLD,&status);
	for(int j=0;j<block;j++){
		for(int k=0;k<block;k++){
			block_arr[j+ghost][k+ghost]=(rank==0)? send_arr[j*block+k] : recv_arr[j*block+k];
		}
	}
	int src0,dest0,src1,dest1;
	MPI_Cart_shift(cart_comm,0,1,&src0,&dest0);
	MPI_Cart_shift(cart_comm,1,1,&src1,&dest1);
	for(curN=0;curN<N;){
		//communication		
		if(src1<0){
			for(int i=0;i<block_arr_size;i++)
				for(int j=0;j<ghost;j++)
					block_arr[i][j]=BOUNDARY;
		}else{
			for(int i=0;i<block;i++){
				for(int j=0;j<ghost;j++){
					send_ghost12[i*ghost+j]=block_arr[i+ghost][j+ghost];
				}
			}
			MPI_Isend(send_ghost12,ghost*block,MPI_CHAR,src1,12,MPI_COMM_WORLD,&send_req[0]);	
		}
		if(dest1<0){
			for(int i=0;i<block_arr_size;i++)
				for(int j=block+ghost;j<block_arr_size;j++)
					block_arr[i][j]=BOUNDARY;
		}else{
			for(int i=0;i<block;i++){
				for(int j=0;j<ghost;j++){
					send_ghost22[i*ghost+j]=block_arr[i+ghost][j+block];
				}
			}
			MPI_Isend(send_ghost22,ghost*block,MPI_CHAR,dest1,22,MPI_COMM_WORLD,&send_req[1]);
			
		}
		if(src0<0){
			for(int i=0;i<ghost;i++)
				for(int j=0;j<block_arr_size;j++)
					block_arr[i][j]=BOUNDARY;
		}else{
			for(int i=0;i<ghost;i++){
				for(int j=0;j<block;j++){
					send_ghost32[i*block+j]=block_arr[i+ghost][j+ghost];
				}
			}
			MPI_Isend(send_ghost32,ghost*block,MPI_CHAR,src0,32,MPI_COMM_WORLD,&send_req[2]);
		}
		if(dest0<0){
			for(int i=block+ghost;i<block_arr_size;i++)
				for(int j=0;j<block_arr_size;j++)
					block_arr[i][j]=BOUNDARY;
		}else{
			for(int i=0;i<ghost;i++){
				for(int j=0;j<block;j++){
					send_ghost42[i*block+j]=block_arr[i+block][j+ghost];
				}
			}
			MPI_Isend(send_ghost42,ghost*block,MPI_CHAR,dest0,42,MPI_COMM_WORLD,&send_req[3]);
		}
		if(src0>=0 && dest1>=0){
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					send_ghost23[i*ghost+j]=block_arr[i+ghost][j+block];
				}
			}
			MPI_Isend(send_ghost23,ghost*ghost,MPI_CHAR,dest1-dims[0],23,MPI_COMM_WORLD,&send_req[4]);
		}
		if(src0>=0 && src1>=0){
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					send_ghost13[i*ghost+j]=block_arr[i+ghost][j+ghost];
				}
			}
			MPI_Isend(send_ghost13,ghost*ghost,MPI_CHAR,src1-dims[0],13,MPI_COMM_WORLD,&send_req[5]);
					
		}
	
		if(dest0>=0 && dest1>=0){
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					send_ghost24[i*ghost+j]=block_arr[i+block][j+block];
				}
			}
			MPI_Isend(send_ghost24,ghost*ghost,MPI_CHAR,dest1+dims[0],24,MPI_COMM_WORLD,&send_req[6]);
			
		}
		if(src1>=0 && dest0>=0){
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					send_ghost14[i*ghost+j]=block_arr[i+block][j+ghost];
				}
			}
			MPI_Isend(send_ghost14,ghost*ghost,MPI_CHAR,src1+dims[0],14,MPI_COMM_WORLD,&send_req[7]);
		}
	//receive	
		if(src0>=0){
			MPI_Irecv(recv_ghost42,ghost*block,MPI_CHAR,src0,42,MPI_COMM_WORLD,&recv_req[0]);
			MPI_Wait(&recv_req[0],&wait_status);
			for(int i=0;i<ghost;i++){
				for(int j=0;j<block;j++){
					block_arr[i][j+ghost]=recv_ghost42[i*block+j];
				}
			}
		}
		if(dest0>=0){
			MPI_Irecv(recv_ghost32,ghost*block,MPI_CHAR,dest0,32,MPI_COMM_WORLD,&recv_req[1]);
			MPI_Wait(&recv_req[1],&wait_status);
			for(int i=0;i<ghost;i++){
				for(int j=0;j<block;j++){
					block_arr[i+block+ghost][j+ghost]=recv_ghost32[i*block+j];
				}
			}
		}
		if(src1>=0){
			MPI_Irecv(recv_ghost22,ghost*block,MPI_CHAR,src1,22,MPI_COMM_WORLD,&recv_req[2]);
			MPI_Wait(&recv_req[2],&wait_status);
			for(int i=0;i<block;i++){
				for(int j=0;j<ghost;j++){
					block_arr[i+ghost][j]=recv_ghost22[i*ghost+j];
				}
			}
		}
		if(dest1>=0){
			MPI_Irecv(recv_ghost12,ghost*block,MPI_CHAR,dest1,12,MPI_COMM_WORLD,&recv_req[3]);
			MPI_Wait(&recv_req[3],&wait_status);
			for(int i=0;i<block;i++){
				for(int j=0;j<ghost;j++){
					block_arr[i+ghost][j+block+ghost]=recv_ghost12[i*ghost+j];
				}
			}
		}
		if(src0>=0 && src1>=0){
			MPI_Irecv(recv_ghost24,ghost*ghost,MPI_CHAR,src1-dims[0],24,MPI_COMM_WORLD,&recv_req[4]);
			MPI_Wait(&recv_req[4],&wait_status);
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					block_arr[i][j]=recv_ghost24[i*ghost+j];
				}
			}
		}
		if(dest0>=0 && dest1>=0){
			MPI_Irecv(recv_ghost13,ghost*ghost,MPI_CHAR,dest1+dims[0],13,MPI_COMM_WORLD,&recv_req[5]);
			MPI_Wait(&recv_req[5],&wait_status);
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					block_arr[i+block+ghost][j+block+ghost]=recv_ghost13[i*ghost+j];
				}
			}
		}	
		
		if(src0>=0 && dest1>=0){
			MPI_Irecv(recv_ghost14,ghost*ghost,MPI_CHAR,dest1-dims[0],14,MPI_COMM_WORLD,&recv_req[6]);
			MPI_Wait(&recv_req[6],&wait_status);
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					block_arr[i][j+ghost+block]=recv_ghost14[i*ghost+j];
				}
			}
		}
		if(src1>=0 && dest0>=0){
			MPI_Irecv(recv_ghost23,ghost*ghost,MPI_CHAR,src1+dims[0],23,MPI_COMM_WORLD,&recv_req[7]);
			MPI_Wait(&recv_req[7],&wait_status);
			for(int i=0;i<ghost;i++){
				for(int j=0;j<ghost;j++){
					block_arr[i+ghost+block][j]=recv_ghost23[i*ghost+j];
				}
			}
		}
		
		char flag=0;
		for(int p=0;p<ghost;p++){
			for(int i=p;i<block_arr_size-p;i++){
				for(int j=p;j<block_arr_size-p;j++){
					int sum=0;
					if(flag==0){
						if(block_arr[i][j]==BOUNDARY || i==0 || j==0 || i==block_arr_size-1 || j==block_arr_size-1){ after_block_arr[i][j]=block_arr[i][j]; continue;}
						for(int k=0;k<8;k++){
							sum+= block_arr[i+dr[k]][j+dc[k]]=='#'? 1 : 0;
						}
						if((sum==2 && block_arr[i][j]=='#') || sum==3){
							after_block_arr[i][j]='#';
						}else{
							after_block_arr[i][j]='.';
						}
					}else{
						if(after_block_arr[i][j]==BOUNDARY || i==0 || j==0 || i==block_arr_size-1 || j==block_arr_size-1){ block_arr[i][j]=after_block_arr[i][j]; continue;}
						for(int k=0;k<8;k++){
							sum+= after_block_arr[i+dr[k]][j+dc[k]]=='#'? 1 : 0;
						}
						if((sum==2 && after_block_arr[i][j]=='#') || sum==3){
							block_arr[i][j]='#';
						}else{
							block_arr[i][j]='.';
						}
					}
				}
			}
			flag^=1;
			curN++;
			if(curN>=N) break;
		}
		if(flag){
			for(int i=0;i<block;i++){
				for(int j=0;j<block;j++){
					block_arr[i+ghost][j+ghost]=after_block_arr[i+ghost][j+ghost];
				}
			}

			for(int i=0;i<block_arr_size;i++){
				for(int j=0;j<block_arr_size;j++){
					block_arr[i][j]=after_block_arr[i][j];
				}
			}
		}
	}
	send_finish=(char*)malloc(block*block*sizeof(char));

//	printf("[rank %d  : %d %d %d %d]\n",rank,src0,dest0,src1,dest1);
	for(int j=0;j<block;j++){
		for(int k=0;k<block;k++){
			//printf("%c",block_arr[j][k]);
			send_finish[j*block+k]=block_arr[j+ghost][k+ghost];
		}
		//printf("\n");
	}
	if(rank!=0){
		MPI_Send(send_finish,block*block,MPI_CHAR,0,101,MPI_COMM_WORLD);
	}
	if(rank==0){
		recv_finish=(char*)malloc(block*block*sizeof(char));
		MPI_Cart_coords(cart_comm,0,2,coord);
//			printf("{{%d %d %d}}\n",i,coord[0],coord[1]);
		row=coord[0];
		col=coord[1];
		for(int j=0;j<block;j++){
			for(int k=0;k<block;k++){
				arr[row*block+j][col*block+k]=block_arr[j+ghost][k+ghost];
			}
		}
		for(int i=1;i<np;i++){
			MPI_Recv(recv_finish,block*block,MPI_CHAR,i,101,MPI_COMM_WORLD,&status);
			MPI_Cart_coords(cart_comm,i,2,coord);
//			printf("{{%d %d %d}}\n",i,coord[0],coord[1]);
			row=coord[0];
			col=coord[1];
			for(int j=0;j<block;j++){
				for(int k=0;k<block;k++){
					arr[row*block+j][col*block+k]=recv_finish[j*block+k];
				}
			}
		}
		for(int i=0;i<m_original;i++){
			for(int j=0;j<m_original;j++){
				printf("%c",arr[i][j]);
			}
			printf("\n");
		}
	//	printf("[%d %d] Finish\n",m,block);
	}
	MPI_Finalize();
}
