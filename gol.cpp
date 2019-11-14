# include <cstdlib>
# include "mpi.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <math.h>

using namespace std;

int padding = 3;

//int testp = 0;

void fillZero(int * arr, int size){
	for(int i = 0; i < size; i++){
		arr[i] = 0;
	}
}

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

int calcNextRound(int * arr, int pos, int size_x, int size_y){
	int near = 0;
	for(int i = 0; i < 8; i++){
		int adj = getPos(size_x, pos, i);
		near += (adj < 0 || adj >= size_x*size_y) ? 0 : arr[adj];
	}
	return arr[pos]==0 ? near==3 : near==2 || near==3;
}

void addPadding(int ** arr, int x, int y, int pad){
	int nx = x+2*pad;
	int ny = y+2*pad;
	int * a = new int[nx*ny];
	fillZero(a, nx*ny);
	int ni = pad;
	int nj = pad;
	for(int i = 0; i < y; i++){
		for(int j = 0; j < x; j++){
			a[ni*nx+nj] = (*arr)[x*i+j];
			nj++;
		}
		ni++;
		nj = pad;
	}
	delete [] (*arr);
	(*arr) = a;
}

void removePadding(int ** arr, int x, int y, int pad){
	int nx = x-2*pad;
	int ny = y-2*pad;
	int * a = new int[nx*ny];
	fillZero(a, nx*ny);
	int ni = pad;
	int nj = pad;
	for(int i = 0; i < ny; i++){
		for(int j = 0; j < nx; j++){
			a[i*nx+j] = (*arr)[x*ni+nj];
			nj++;
		}
		ni++;
		nj = pad;
	}
	delete [] (*arr);
	(*arr) = a;
}

void copyArr(int * orig, int * targ, int len){
	for(int i = 0; i < len; i++){
		targ[i] = orig[i];
	}
}

void extractBoundary(int * upper, int * lower, int * arr, int x, int y, int pad){
	for(int i = 0; i < x; i++){
		upper[i] = arr[(x+2*pad)*(pad)+pad+i];
		lower[i] = arr[(x+2*pad)*(pad+y-1)+pad+i];
	}
}

void setBoundary(int * upper, int * lower, int * arr, int x, int y, int pad){
	for(int i = 0; i < x; i++){
		arr[(x+2*pad)*(pad-1)+pad+i] = upper[i];
		arr[(x+2*pad)*(pad+y)+pad+i] = lower[i];
	}
}

void makeAllToAll(int * upper, int * lower, int x, int y, int procs, int my){
	int * sendbuf = new int[2*x];
	int * sendcounts = new int[procs];
	int * sdispls = new int[procs];
	int * recvbuf = new int[2*x];
	int * recvcounts = new int[procs];
	int * rdispls = new int[procs];

	int loc = 0;

	//for(int i = 0; i < procs; i++){cout << sendcounts[i] << "| ";}
	for(int i = 0; i < 2*x; i++){
		sendbuf[i] = recvbuf[i] = 0;
	}

	for(int i = 0; i < procs; i++){
		sendcounts[i] = recvcounts[i] = sdispls[i] = rdispls[i] = 0;
		if(i == my-1){
			sendcounts[i] = x;
			recvcounts[i] = x;
			sdispls[i] = 0;
			rdispls[i] = 0;
			loc += x;
		}else if(i == my+1){
			sendcounts[i] = x;
			recvcounts[i] = x;
			sdispls[i] = loc;
			rdispls[i] = loc;
		}
		
	}

	copyArr(upper, sendbuf, x);
	copyArr(lower, sendbuf+x, x);
	MPI_Alltoallv(sendbuf,sendcounts,sdispls,MPI_INT,recvbuf,recvcounts,rdispls,MPI_INT,MPI_COMM_WORLD);
	copyArr(recvbuf, lower, x);
	copyArr(recvbuf+x, upper, x);
}

void runRound(int ** curr, int ** next, int x, int y){
	//free(*next);
	//(*next) = new int[x*y];
	for(int i = 0; i < x*y; i++){
		(*next)[i] = calcNextRound(*curr, i, x, y);
	}
}

int main(int argc,char* argv[]){
	int id;
	int p;
	double wtime;

	if(argc != 4){ 
		//exit(0);
	}
	cout << argc << endl;

	MPI::Init(argc, argv); //  Initialize MPI.
	p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
	id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.

	

	int n, k, testp;

	int *infile;
	int *outfile;

	if(id == 0){
		string fn(argv[1]);
		n = stoi(argv[2]);
		k = stoi(argv[3]);
		testp = stoi(argv[4]);

		cout << fn << n << k << testp << endl;
		
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
			delete [] arr;
			arr = arr2;
			arr2 = new int[n*n];
			for(int i = 0; i < n*n; i++){
				arr2[i] = calcNextRound(arr, i, 100, 100);
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

	bool inityet = false;

	int s = ceil(float(n)/float(p));

	int * rec = new int[n*s];
	int * res = new int[(n+2*padding)*(s+2*padding)];

	MPI_Scatter(infile, n*s, MPI_INT, rec, n*s, MPI_INT, 0, MPI_COMM_WORLD);

	addPadding(&rec, n, s, padding);
	for(int i = 0; i < (n+2*padding)*(s+2*padding); i++){res[i] = rec[i];}

	int * uppad = new int[n];
	int * downpad = new int[n];

	for(int i = 0; i < n; i++){
		uppad[i] = downpad[i] = 0;
	}

	while(k >= 0){
		if(!inityet){
			int * sendup = new int [n*p];
			int * senddown = new int [n*p];

			for(int i = 0; i < n*p; i++){
				sendup[i] = senddown[i] = 0;
			}
			if(id==0){
				for(int i = 1; i < p; i++){
					copyArr(infile + n*(i*s-1), sendup + n*(i), n);
				}
				for(int i = 1; i < p; i++){
					copyArr(infile + n*(i*s), senddown + n*(i-1), n);
				}
			}
			MPI_Scatter(sendup, n, MPI_INT, uppad, n, MPI_INT, 0, MPI_COMM_WORLD);
			MPI_Scatter(senddown, n, MPI_INT, downpad, n, MPI_INT, 0, MPI_COMM_WORLD);

			if(id==testp){
				for(int i = 0; i < n; i++){
					cout << uppad[i];
				}
				cout << endl << endl;
			}

			setBoundary(uppad, downpad, rec, n, s, padding);
			inityet = true;
			delete [] sendup;
			delete [] senddown;
		}else{
			if(id==testp){
				for(int i = 0; i < s+2*padding; i++){
					for(int j = 0; j < n+2*padding; j++){
						cout << rec[(n+2*padding)*i+j];
					}
					cout << endl;
				}
				cout << endl;
			}
			runRound(&rec, &res, n+2*padding, s+2*padding);
			int * temp;
			temp = res;
			res = rec;
			rec = temp;
			extractBoundary(uppad, downpad, rec, n, s, padding);

			if(id==testp){
				cout << "send upper:\n";
				for(int i = 0; i < n; i++){
					cout << uppad[i];
				}
				cout << endl << endl;
				cout << "send lower:\n";
				for(int i = 0; i < n; i++){
					cout << downpad[i];
				}
				cout << endl << endl;
			}

			makeAllToAll(uppad, downpad, n, s, p, id);

			if(id==testp){
				cout << "get upper:\n";
				for(int i = 0; i < n; i++){
					cout << uppad[i];
				}
				cout << endl << endl;
				cout << "get lower:\n";
				for(int i = 0; i < n; i++){
					cout << downpad[i];
				}
				cout << endl << endl;
			}

			setBoundary(uppad, downpad, rec, n, s, padding);

			k--;
		}
	}




	removePadding(&res, n+2*padding, s+2*padding, padding);

	MPI_Gather(res, n*s, MPI_INT, outfile, n*s, MPI_INT, 0, MPI_COMM_WORLD);

	//int MPI_Alltoallv(sendbuf,sendcounts,sdispls,MPI_INT,recvbuf,recvcounts,rdispls,MPI_INT,MPI_COMM_WORLD)

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
