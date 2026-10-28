/*
 * Pipeline : transformations / - z-buffer / - lighting /
 *            |-> rotations /
 *            |-> depth /
 *
 * axis :         +------+ x
 *                |\
 *                | \
 *                |  + z
 *                +
 *                y   
 */

/** Imports **/

#include<stdio.h>
#include<string.h>
#include<math.h>
#include<unistd.h>

/** Constants **/

#define pi 3.1415926

// commented values are default values for a terminal size 90x25
#define DEPTH_COEFF 60 // 60 - scaling factor for the depth effect
#define DISTANCE 100 // 100 - represents the distance to the camera
#define LIGHT_DECAY 0.5 // 0.5 - how fast the light decays with distance 


#define WIDTH 50 // 50
#define HEIGHT 25 // 25

/** Main program **/

struct vector3{
	double x;
	double y;
	double z;
};

void rotateVector(struct vector3 *v, double theta, double phi, double gamma){
	double x = v->x;
	double y = v->y;
	double z = v->z;
	
	// expands rotation transformations on each axis
	// rotation x
	double r_y = y*cos(theta)-z*sin(theta);
	z = y*sin(theta)+z*cos(theta);
	y = r_y;

	//rotation y
	double r_x = x*cos(phi)+z*sin(phi);
	z = -x*sin(phi)+z*cos(phi);
	x = r_x;

	//rotation z
	r_x = x*cos(gamma)-y*sin(gamma);
	y = x*sin(gamma)+y*cos(gamma);
	x = r_x;
	
	// updates vector position
	v->x = x;
	v->y = y;
	v->z = z;
}

void displayVector(struct vector3 v){
	// test function
	printf("%f, %f, %f \n", v.x, v.y, v.z);
}

void updateAxis(struct vector3 *x, struct vector3 *y, struct vector3 *z, double theta, double phi, double gamma){
	// rotates any three given vectors, meant to rotate the unit axis
	rotateVector(x, theta, phi, gamma);
	rotateVector(y, theta, phi, gamma);
	rotateVector(z, theta, phi, gamma);
}

struct vector3 alignVectorToAxis(struct vector3 v, struct vector3 x, struct vector3 y, struct vector3 z){
	// aligns vector's component on given unit vectors
	// meant to be applied after rotating said unit vectors
	struct vector3 u_v;
	u_v.x = v.x * x.x + v.y * y.x + v.z * z.x;
	u_v.y = v.x * x.y + v.y * y.y + v.z * z.y;
	u_v.z = v.x * x.z + v.y * y.z + v.z * z.z;
	
	return u_v;
}

void applyDepth(struct vector3 *v){
	// applies depth effect with further away points getting closer
	// to the center of the screen, resulting in a perspective effect.
	if (v->z > 0){ // doesn't display points with neg z, i.e behind the camera
		double depthFactor = DEPTH_COEFF/v->z; // scaled with a coefficient
		v->x *= depthFactor;
		v->y *= depthFactor;
	}
}

void displayZBuffer(double z_buffer[HEIGHT][WIDTH]){
	// test function to display the raw z-buffer contents
	for (int i = 0; i < HEIGHT; i++){
		for (int j = 0; j < WIDTH; j++){
			printf("%f ", z_buffer[i][j]);
		}
		printf("\n");
	}
}

void insertVertexToZBuffer(double z_buffer[HEIGHT][WIDTH], struct vector3 v){
	// inserting given vertex into the z-buffer, overwriting points that are
	// further away if necessary
	if (v.z > 0){
		int x = (int)round(v.x)+WIDTH/2; // centers the vertex to the middle of the screen
		int y = (int)round(v.y)+HEIGHT/2;
		if (0 <= x && x < WIDTH && 0 <= y && y < HEIGHT && (z_buffer[y][x] > v.z || z_buffer[y][x] == 0)){
			z_buffer[y][x] = v.z;
		}
	}
}

double updateZBuffer(double z_buffer[HEIGHT][WIDTH], struct vector3 verticesArray[], int size, struct vector3 x, struct vector3 y, struct vector3 z, struct vector3 center){
	// writes all points in the z-buffer after rotation and returns the minimum
	// z value found among the points cloud
	int minZ = DISTANCE;
	for (int i = 0; i < size; i++){
		struct vector3 v = alignVectorToAxis(verticesArray[i], x, y, z);
	        // the vertex is rotated away from the camera as otherwise the camera
		// would be at the center of the object
		v.x += center.x;
                v.y += center.y;
                v.z += center.z;
                applyDepth(&v);
        	insertVertexToZBuffer(z_buffer, v);
		if (minZ > v.z)
			minZ = v.z;
	}
	return minZ;
}

void createCubeVerticesArray(struct vector3 verticesArray[]){
	// defines the vertices of a cube and inserts them into
	// a given array. Since the size is not computed, the array
	// must be large enough to compile everything, otherwise it
	// displays a black screen or crashes.
	int counter = 0;
	for (double i = -1; i <= 1.1; i+=0.1){
		for (double j = -1; j <= 1.1; j+=0.1){
			for (double k = -1; k <= 1.1; k+=0.1){
				if (i*i >= 1 || j*j >= 1 || k*k >= 1){
					struct vector3 tempV = {i*10, j*10, k*10};
					verticesArray[counter++] = tempV;
				}
			}
		}
	}
	printf("%d\n", counter);
}

void createTorusVerticesArray(struct vector3 verticesArray[]){
	// defines a torus through parametrization. The step, large radius
	// and small radius are hardcoded.
	int counter = 0;
	double step = 2*pi / 60;
	for (double theta = 0; theta < 2*pi; theta += step){
		for (double phi = 0; phi < 2*pi; phi += step){
			struct vector3 tempV = {(10+5*cos(phi))*cos(theta), (10+5*cos(phi))*sin(theta), 5*sin(phi)};
			verticesArray[counter++] = tempV;
		}
	}

	printf("%d\n", counter);
}

void drawFrame(double z_buffer[HEIGHT][WIDTH], double minZ){
	char palette[] = " @%#*+=-:."; // ascii chars palette sorted from brightest to darkest
	char display[WIDTH*HEIGHT*2+HEIGHT+1] = "";
	printf("\e[1;1H\e[2J"); // clears the console
	int k = 0;
	for (int i = 0; i < HEIGHT; i++){
		for (int j = 0; j < WIDTH; j++){
			// scales the z-value of each point based on the distance to the camera
			// and the LIGHT_DECAY value. The z-value itself defines how bright a vertex
			// is, as the closer it is to the camera the brighter it gets.
			int lightIndex = (int)((z_buffer[i][j]-minZ)*LIGHT_DECAY+1);
			char lightChar = ' ';
			// if the index is within range of the palette it displays the associated character
			// otherwise it displays the last char, i.e the darkest one
			if (lightIndex < strlen(palette)){
				lightChar = palette[lightIndex];
			}
			else{
				lightChar = '.';
			}
			if (z_buffer[i][j] == 0)
				// z-value=0 means default value, i.e background
				lightChar = ' ';
			display[k++] = lightChar;
			display[k++] = lightChar;
		}
		display[k++] = '\n';
	}
	printf("%s\n", display);
}

void resetZBuffer(double z_buffer[HEIGHT][WIDTH]){
	// writes a 0 in every cell of the z-buffer
	for (int i = 0; i < HEIGHT; i++){
		for (int j = 0; j < WIDTH; j++){
			z_buffer[i][j] = 0;
		}
	}
}

int main(){
	int size = 3600; // array size, larger to handle all points
	struct vector3 verticesArray[size];
	// Uncomment next line for cube and change size
	//createCubeVerticesArray(verticesArray);
	createTorusVerticesArray(verticesArray);

	struct vector3 center = {0, 0, DISTANCE};

	struct vector3 x = {1, 0, 0};
	struct vector3 y = {0, 1, 0};
	struct vector3 z = {0, 0, 1};

	// rotation angles
	double theta = 0.025;
	double phi = 0.02;
	double gamma = 0.015;

	double z_buffer[HEIGHT][WIDTH];
	double minZ;
	
	while (true){ // Ctrl+C to stop
		updateAxis(&x, &y, &z, theta, phi, gamma);
		resetZBuffer(z_buffer);
		minZ = updateZBuffer(z_buffer, verticesArray, size, x, y, z, center);
		drawFrame(z_buffer, minZ);
		usleep(10000);
	}

	return 0;
}
