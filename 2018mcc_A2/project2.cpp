#include<stdio.h>
#include<stdlib.h>
#include<omp.h>
#include<time.h>
int n;
double* l_inverse(double* a, int n0){	
	double* ret=(double*)calloc(n0*n0,sizeof(double));
	int i,j,k;
	double s;
	for(i=0;i<n0;i++)ret[i*n0+i]=1/a[i*n0+i];
	for(i=1;i<n0;i++){
		for(j=i;j<n0;j++){
			s=0.0;
			for(k=j-i;k<=j;k++){ 
				s+=ret[j*n0+k]*a[k*n0+j-i];
			}
			ret[j*n0+j-i]=-s/a[(j-i)*n0+j-i];
		}
	}
	return ret;
}

double* u_inverse(double *a,int n0){
	double* ret=(double*)calloc(n0*n0,sizeof(double));
	int i,j,k;
	double s;
	for(i=0;i<n0;i++) ret[i*n0+i]=1/a[i*n0+i];
	for(i=1;i<n0;i++){
		for(j=0;j<n0-i;j++){
			s=0.0;
			for(k=j;k<=i+j;k++){
				s+=ret[j*n0+k]*a[k*n0+i+j];
			}

			ret[j*n0+i+j]=-s/a[(i+j)*n0+(i+j)];
		}
	}
	return ret;
}
void multiply(double *L,int r,int c, double *R,double* Dest,int size,int mode){
	for(int i=0;i<size;i++){
		for(int j=0;j<size;j++){
			double sum=0;
			for(int k=0;k<size;k++){
				if(mode==0)sum+=L[i*size+k]*R[(r+k)*n+j+c];
				else sum+=L[(r+i)*n+k+c]*R[k*size+j];
			}
			//		printf("r+i : %d , j+c : %d sum:%lf\n",r+i,j+c,sum);
			Dest[(r+i)*n+j+c]=sum;
		}
	}
	//	printf("\n");
}
void subpart(double* A, double *L,int lr,int lc,double *U,int rr,int rc,int size){
	double sum=0;
	for(int i=0;i<size;i++){
		for(int j=0;j<size;j++){
			sum=0;
			for(int k=0;k<size;k++){
				sum+=L[(i+lr)*n+(k+lc)]*U[(k+rr)*n+(j+rc)];
			}
			A[(i+lr)*n+(j+rc)]-=sum;
		}
	}
}
void LUdecomposition(double *a,double *l,double *u,int r,int size,double** l_inv,double** u_inv){
	for(int k=r;k<r+size;k++){
		u[k*n+k]=a[k*n+k];
		for(int i=k+1;i<r+size;i++){
			l[i*n+k]=a[i*n+k]/u[k*n+k];
			u[k*n+i]=a[k*n+i];
		}
		for(int i=k+1;i<r+size;i++){
			for(int j=k+1;j<r+size;j++){
				a[i*n+j]=a[i*n+j]-l[i*n+k]*u[k*n+j];
			}
		}
	}
	double * copy=(double*)malloc(sizeof(double)*size*size);
	for(int i=r;i<size+r;i++){
		for(int j=r;j<size+r;j++){
			copy[(i-r)*size+(j-r)]=l[i*n+j];
		}
	}
	(*l_inv)=l_inverse(copy,size);
	for(int i=r;i<size+r;i++){
		for(int j=r;j<size+r;j++){
			copy[(i-r)*size+(j-r)]=u[i*n+j];
		}
	}
	(*u_inv)=u_inverse(copy,size);
	if(copy)free(copy);

}
void print_arr(double *a,int n_size){
	int i,j; 
	for(i=0;i<n_size;i++){
		for(j=0;j<n_size;j++){
			printf("%.4e ",a[i*n+j]);
		}
		printf("\n");
	}
}


int main(int argc,char * argv[]){
	int no_thread,bound;
	int r_seed=1;
	int print_flag=0,n_init=0;
	double*a_ori,*a,*l,*u,*l_inv,*u_inv;
	int part;
	if(1){
		if(argc!=5){
			printf("Usage : project2 <size_of_a_matrix> <random_seed_number> <numbuer_of_thread> <print_flag>\n");
			exit(EXIT_FAILURE);	
		}
		n_init=atoi(argv[1]);
		r_seed=atoi(argv[2]);
		no_thread=atoi(argv[3]);
		print_flag=atoi(argv[4]);
	}
	part=n_init/32+1;
	//part=312;
	n=(n_init/part+(n_init%part==0?0:1))*part;
	bound=n*n;
	omp_set_num_threads(no_thread);
	a_ori=(double*)malloc(sizeof(double)*bound);
	a=(double*)malloc(sizeof(double)*bound);
	l=(double*)calloc(bound,sizeof(double));
	u=(double*)calloc(bound,sizeof(double));
//printf("[%d %d]\n",part,n);
	for(int i=0;i<n;i++) l[i*n+i]=1;
	srand(r_seed);
	//	srand(time(NULL));
	for(int i=0;i<n_init;i++){
		int j=0;
		for(;j<n_init;j++){
			a[i*n+j]=rand();
		}
		for(;j<n;j++) a[i*n+j]=0;
	}
	for(int i=n_init;i<n;i++){
		for(int j=0;j<n;j++){
			a[i*n+j]=0;
			if(i==j){
				a[i*n+j]=1;
			}
		}
	}
	//	printf("A\n");print_arr(a,n);
	//	printf("U\n");print_arr(u,n);
	//	printf("L\n");print_arr(l,n);

	for(int i=0;i<n*n;i++) a_ori[i]=a[i];
//	double starttime=omp_get_wtime();

	int bs=n/part;
	for(int PA=0;PA<part;PA++){
		int sp=PA*bs;
		//printf("[%d]\n",PA);
		LUdecomposition(a,l,u,sp,bs,&l_inv,&u_inv);
#pragma omp parallel
		{
#pragma omp for nowait 
			for(int p=PA+1;p<part;p++){
				multiply(l_inv,sp,p*bs,a,u,bs,0);//UUUUUU
			}
#pragma omp for
			for(int p=PA+1;p<part;p++){
				multiply(a,p*bs,sp,u_inv,l,bs,1);//LLLLLL
			}
		}
#pragma omp parallel for 
		for(int p=PA+1;p<part;p++){
			for(int q=PA+1;q<part;q++){
				subpart(a,l,p*bs,sp,u,sp,q*bs,bs);
			}
		}
		//	if(l_inv)free(l_inv);
		//	if(u_inv)free(u_inv);
		/*		if(print_flag){
				printf("\n\nPA:%d sp : %d\n",PA,sp);
				printf("Linv:\n");print_arr(l_inv,n/part);
				printf("Uinv:\n");print_arr(u_inv,n/part);

				printf("L:\n");print_arr(l,n);
				printf("U:\n");print_arr(u,n);
				printf("A:\n");print_arr(a_ori,n);
				}*/
	}
//	printf("[%lf]\n",omp_get_wtime()-starttime);
	if(print_flag){
		printf("L:\n");print_arr(l,n_init);
		printf("U:\n");print_arr(u,n_init);
		printf("A:\n");print_arr(a_ori,n_init);
	}
	//verify!
/*	printf("CHECK!\n");
#pragma omp parallel for
	for(int i=0;i<n_init;i++){
		if(i%100==0)printf("[%d progressed]\n",i*100/n_init);
		double tmp;
		for(int j=0;j<n_init;j++){
			double sum=0;
			for(int k=0;k<n_init;k++){
				sum+=l[i*n+k]*u[k*n+j];
			}
			//printf("[%.4lf,%.4lf]",sum,a_ori[pi[i]][j]);
			tmp=a_ori[i*n+j]-sum;
			tmp=tmp>0?tmp:-tmp;
			if( tmp>1)printf("%lf %lf %.8lfWRONG!\n",sum,a_ori[i*n+j],tmp);

		}
	}
	printf("CHECKEND!\n");*/
}

