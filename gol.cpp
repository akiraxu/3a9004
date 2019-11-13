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
		infile = new int[n*haserh];
		outfile = new int[n*haserh];
		
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

	if(id)

	cout << "My id is " << id << " n=" << n << " k=" << k << endl;

	int MPI_Scatter(infile, n*s, MPI_INT, rec, n*s, MPI_INT, 0, MPI_COMM_WORLD);

	for(int i = 0; i < n*s; i++){
		res[i] = rec[i];
	}

	int MPI_Gather(res, n*s, MPI_INT, outfile, n*s, MPI_INT, 0, MPI_COMM_WORLD);

	if(id==0){
		for(int i = 0; i < s; i++){
			for(int j = 0; j < n; j++){
				cout << arr2[n*i+j];
			}
			cout << endl;
		}
	}

	MPI::Finalize();
	return 0;
}
