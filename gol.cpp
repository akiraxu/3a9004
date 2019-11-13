# include <cstdlib>
# include "mpi.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <math.h>

using namespace std;

int getPos(int size_x, int x, int dx){
	int rc = -1;
	switch(dx){
		case 0:
			rc = x - size_x - 1;
			break;
		case 1:
			rc = x - size_x;
			break;
		case 2:
			rc = x - size_x + 1;
			break;
		case 3:
			rc = x - 1;
			break;
		case 4:
			rc = x + 1;
			break;
		case 5:
			rc = x + size_x - 1;
			break;
		case 6:
			rc = x + size_x;
			break;
		case 7:
			rc = x + size_x + 1;
			break;
		default:
			rc = -1;
			break;
	}
	return rc;
}

int calcNextRound(int * arr, int pos, int size_x){
	int near = 0;
	for(int i = 0; i < 8; i++){
		int adj = getPos(size_x, pos, i);
		near += adj < 0 ? 0 : arr[adj];
	}
	return arr[pos]==0 ? near==3 : near==2 || near==3;
}

int addPadding(int ** arr, int x, int y, int pad){
	int nx = x+2*pad;
	int ny = y+2*pad;
	int * a = new int[nx*ny];
	int ni = 1;
	int nj = 1;
	for(int i = 0; i < y; i++){
		for(int j = 0; j < x; j++){
			a[ni*nx+nj] = arr[x*i+j];
			nj++;
		}
		ni++;
		nj = 1;
		cout << endl;
	}
}

int removePadding(int ** arr, int x, int y, int pad){
	int nx = x-2*pad;
	int ny = y-2*pad;
	int * a = new int[nx*ny];
	int ni = 0;
	int nj = 0;
	for(int i = pad; i < y-pad; i++){
		for(int j = pad; j < x-pad; j++){
			a[(ni-pad)*nx+nj] = arr[x*i+j];
			nj++;
		}
		ni++;
		nj = 0;
		cout << endl;
	}
}

int main(int argc,char* argv[]){
	int id;
	int p;
	double wtime;

	if(argc != 4){ 
		exit(0);
	}
	cout << argc << endl;

	MPI::Init(argc, argv); //  Initialize MPI.
	p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
	id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.

	

	int n, k;

	int *infile;
	int *outfile;

	if(id == 0){
		string fn(argv[1]);
		n = stoi(argv[2]);
		k = stoi(argv[3]);

		cout << fn << n << k << endl;
		
		int *arr = new int[n*n];
		int *arr2 = new int[n*n];
		int haserh = ceil(float(n)/float(p));
		infile = new int[n*haserh*p];
		outfile = new int[n*haserh*p];
		
		ifstream in(fn);
		ofstream out("" + fn + ".out");
		
		string line;
		
		int i = 0;
		while(getline(in,line)){
			if(i>=n){
				break;
			}
			for(int j = 0; j < n; j++){
				arr[n*i+j] = stoi(line.substr(j,1));
			}
			i++;
		}

		for(int i = 0; i < n*n; i++){
			arr2[i] = arr[i];
			infile[i] = arr[i];
		}
		for(int t = 0; t < k; t++){
			free(arr);
			arr = arr2;
			arr2 = new int[n*n];
			for(int i = 0; i < n*n; i++){
				arr2[i] = calcNextRound(arr, i, 100);
			}
		}
/*
		for(int i = 0; i < n; i++){
			for(int j = 0; j < n; j++){
				cout << arr2[n*i+j];
			}
			cout << endl;
		}
*/
	}
	MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&k,1,MPI_INT,0,MPI_COMM_WORLD);

	int s = ceil(float(n)/float(p));

	int * rec = new int[n*s];
	int * res = new int[n*s];

	cout << "My id is " << id << " n=" << n << " k=" << k << endl;

	MPI_Scatter(infile, n*s, MPI_INT, rec, n*s, MPI_INT, 0, MPI_COMM_WORLD);

	cout << "ID:" << id << " After Scatter" << endl;

	for(int i = 0; i < n*s; i++){
		res[i] = rec[i];
	}

	addPadding(&res, n, s, 10);
	removePadding(&res, n, s, 10);

	MPI_Gather(res, n*s, MPI_INT, outfile, n*s, MPI_INT, 0, MPI_COMM_WORLD);

	int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)

	cout << "ID:" << id << " After Gather" << endl;

	if(id==0){
		cout << "final" << endl;
		for(int i = 0; i < s * p; i++){
			for(int j = 0; j < n; j++){
				cout << outfile[n*i+j];
			}
			cout << endl;
		}
	}

	MPI::Finalize();
	return 0;
}
