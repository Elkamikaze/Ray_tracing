#include "common/cpu_anim.h"
#include "common/cpu_bitmap.h"
#include <iostream>
#include <unistd.h>

#include <cmath>
#include <math.h>
#include <mpi.h>

using namespace std;

#define rnd(x) ((x * rand()) / RAND_MAX)

int display=1;
int nbr_spheres = 100;

int IMAGE_WIDTH  = 512;
int IMAGE_HEIGHT = 512;
const int IMAGE_Z = 500;
int size = 0;
#define INF 1e10f

/**
 * structure to store Sphere information
 */
struct Sphere {
	// red, green, blue
	float r, g, b;
	// coordinates and radius
	float x, y, z, radius;

	/**
	 * determine if a ray (r_x, r_y) intersects a sphere
	 * and return distance
	 */
	float intersects(float r_x, float r_y, float *scale) {
		float	dx = r_x - x;
		float   dy = r_y - y;
		
		float q =  (radius * radius) - (dx * dx + dy * dy);
		if (q >= 0.0) {
			float dz = sqrtf( q );
			*scale = dz / sqrtf(radius * radius);
			return dz + z;
		}
		return -INF;
	}
	
};

Sphere *tab_spheres_h;



void allocate_cpu_resources() {
	tab_spheres_h = new Sphere[ nbr_spheres ];
	for (int i=0; i<nbr_spheres; ++i) {
		Sphere *s = &tab_spheres_h[i];
		s->r = rnd( 1.0f );
		s->g = rnd( 1.0f );
		s->b = rnd( 1.0f );
		s->x = rnd(static_cast<float>(IMAGE_WIDTH)) - IMAGE_WIDTH/2;
		s->y = rnd(static_cast<float>(IMAGE_HEIGHT)) - IMAGE_HEIGHT/2;
		s->z = rnd(1000.0f) + IMAGE_Z;
		s->radius = rnd(50.0f) + 10.0;

		/*
		cout << "x=" << s->x << ", y=" << s->y << ", z=" << s->z 
			<< ", radius=" << s->radius << endl;		
		*/	
	}
}

/**
 * main function for computation
 * @param ptr : pointer to bitmap
 * for each pixel of the image, determine which sphere is the closest
 * to the observer and setup color consequently
 */
void kernel(unsigned char *ptr ) {
	int offset = 0;

	
	/******** all process *********/
	MPI_Scatter(tab_spheres_h,size,MPI_FLOAT,tab_spheres_h,size,MPI_FLOAT,0,MPI_COMM_WORLD);	
	

	// for each pixel of the image
	for (int y = 0; y < IMAGE_HEIGHT; ++y) {
		for (int x = 0; x < IMAGE_WIDTH; ++x) {
			float ox = static_cast<float>(x - IMAGE_WIDTH/2);
			float oy = static_cast<float>(y - IMAGE_HEIGHT/2);
			
			//cout << "ox = " << ox << ", oy = " << oy << endl;
			
			float r,g,b;
			r = g = b = 0.0;
			float max_z = -INF;
			float distance, z;
			
			for (int i= 0; i < nbr_spheres; ++i) {
				z = tab_spheres_h[i].intersects(ox, oy, &distance);
				if (z > max_z) {
					r = tab_spheres_h[i].r * distance;
					g = tab_spheres_h[i].g * distance;
					b = tab_spheres_h[i].b * distance;
					max_z = z;
				}
			}
			
			ptr[offset + 0] = static_cast<unsigned char>(r * 255);
			ptr[offset + 1] = static_cast<unsigned char>(g * 255);
			ptr[offset + 2] = static_cast<unsigned char>(b * 255);
			ptr[offset + 3] = 255;
			offset += 4;
		}
	}

	MPI_Gather(ptr,size,MPI_CHAR,ptr,size,MPI_CHAR,0,MPI_COMM_WORLD);

}	

/**
 * call program with arguments : ns h w d
 * - ns : number of spheres (default 100)
 * - h image height (default 512)
 * - w image width (default 512)
 * - d display  image (0 = no, else yes)
 */
int main(int argc, char *argv[]) {

	srand(getpid());

	int myrank, numProcs;
	
	/* declare variables */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

	size = nbr_spheres/numProcs;
/*
	if (argc > 1) {
		nbr_spheres = atoi(argv[1]);
		if (nbr_spheres <= 0) nbr_spheres = 1;
	}
	
	if (argc > 2) {
		IMAGE_WIDTH = atoi(argv[2]);
		IMAGE_HEIGHT = IMAGE_WIDTH;
	}
	if (argc > 3) {
		IMAGE_HEIGHT = atoi(argv[3]);
	}
	if (argc > 4) {
		display = atoi(argv[4]);
	}*/
		
	CPUBitmap bitmap( IMAGE_WIDTH, IMAGE_HEIGHT );
	unsigned char *bitmap_d = bitmap.get_ptr();

	if(myrank == 0){
		// check that numProcs divides NELTS else abort
		if (size*numProcs!=N) {
			printf("Number of processes (%d) must divide evenly into problem size (%d).\n",numProcs,N);
			fflush(stdout);
			MPI_Abort(MPI_COMM_WORLD,1);
		}

		allocate_cpu_resources();
	}
	
	cout << "bitmap size = " << bitmap.image_size() << endl;

	clock_t start, stop;
	start = clock();
	
	kernel(bitmap_d);

	stop = clock();
	float elapsed_time = (stop-start)/1000000.0;
	cout << "time : " << elapsed_time << " seconds" << endl;

	//if (display) bitmap.display_and_exit();

	delete [] tab_spheres_h;
	
    MPI_Finalize();
}
