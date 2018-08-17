
  /*
  * FLAME GPU v 1.5.X for CUDA 9
  * Copyright University of Sheffield.
  * Original Author: Dr Paul Richmond (user contributions tracked on https://github.com/FLAMEGPU/FLAMEGPU)
  * Contact: p.richmond@sheffield.ac.uk (http://www.paulrichmond.staff.shef.ac.uk)
  *
  * University of Sheffield retain all intellectual property and
  * proprietary rights in and to this software and related documentation.
  * Any use, reproduction, disclosure, or distribution of this software
  * and related documentation without an express license agreement from
  * University of Sheffield is strictly prohibited.
  *
  * For terms of licence agreement please attached licence or view licence
  * on www.flamegpu.com website.
  *
  */

  // includes, project
  #include <cuda_runtime.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <time.h> //added

#include <GL/glew.h>
#include <GL/glut.h>
#include <cuda_gl_interop.h>
	    
#include "header.h"
#include "visualisation.h"

#include "GLUTInputController.h"	//added
#include "PedestrianPopulation.h"	//added
#include "NavMapPopulation.h"		//added
#include "MenuDisplay.h"			//added
#include "GlobalsController.h"		//added


extern void stepFLAMESimulation()	//added
{
	singleIteration();
}

#define FOVY 45.0

#define Z0_RANGE (Z0_MAX - Z0_MIN)
#define H_RANGE (H_MAX - H_MIN)

//full screen mode
int fullScreenMode;


// bo variables
GLuint sphereVerts;
GLuint sphereNormals;

//Simulation output buffers/textures
cudaGraphicsResource_t FloodCell_Default_cgr;
GLuint FloodCell_Default_tbo;
GLuint FloodCell_Default_displacementTex;


//framerate
float start_time;
float end_time;
float frame_time;
float fps;
int frames;
int av_frames;

// Flood model visualisation setup
//cudaEvent_t start, stop;
//const int display_rate = 50;
//int frame_count;
//float frame_time = 0.0;

// mouse controls				// commented , already defined and existed in GLUTinputController
//int mouse_old_x, mouse_old_y; 
//int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;
float translate_z = -VIEW_DISTANCE;
bool water = true;

// keyboard controls
#if defined(PAUSE_ON_START)
bool paused = true;
#else
bool paused = false;
#endif

// vertex Shader
GLuint vertexShader;
GLuint fragmentShader;
GLuint shaderProgram;
GLuint vs_displacementMap;
GLuint vs_mapIndex;
GLuint vs_water;


#ifdef SIMULATION_DELAY
//delay
int delay_count = 0;
#endif

// prototypes
int initGL();
void initShader();
void createVBO( GLuint* vbo, GLuint size);
//void deleteVBO( GLuint* vbo); // already fefined in BufferObjects
void createTBO( cudaGraphicsResource_t* cudaResource, GLuint* tbo, GLuint* tex, GLuint size);
void deleteTBO( cudaGraphicsResource_t* cudaResource, GLuint* tbo);
void setVertexBufferData();
void reshape(int width, int height);
void display();
//void keyboard( unsigned char key, int x, int y); //// already fefined in GLUTinputController
void special(int key, int x, int y);
//void mouse(int button, int state, int x, int y); // already fefined in GLUTinputController
//void motion(int x, int y);
void runCuda();

//void toggleFullScreenMode(); //added (embedded in visualisation.h)
float getFPS();			   //added (embedded in visualisation.h)


/* Error check function for safe CUDA API calling */
#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

/* Error check function for post CUDA Kernel calling */
#define gpuErrchkLaunch() { gpuLaunchAssert(__FILE__, __LINE__); }
inline void gpuLaunchAssert(const char *file, int line, bool abort=true)
{
	gpuAssert( cudaPeekAtLastError(), file, line );
#ifdef _DEBUG
	gpuAssert( cudaDeviceSynchronize(), file, line );
#endif
   
}

const char vertexShaderSource[] =
{
	"#extension GL_EXT_gpu_shader4 : enable										\n"
	"uniform samplerBuffer displacementMap;										\n"
	"attribute in float mapIndex;												\n"
	"uniform bool water;														\n"		
	"varying vec3 normal, lightDir;												\n"
	"varying vec4 colour;														\n"
	"void main()																\n"
	"{																			\n"
	"	vec4 position = gl_Vertex;											    \n"
	"	vec4 lookup = texelFetchBuffer(displacementMap, (int)mapIndex);		    \n"
	"	if (water)	                											\n"
	"		colour = vec4(1.0f-lookup.w, 1.0f-lookup.w, 1.0f, 0.0);				\n"
	"	else	                												\n"
	"		colour = vec4(lookup.z, lookup.z, lookup.z, 0.0);					\n"
	"	lookup.w = 1.0;												    		\n"
	"	position.x += lookup.x;											    	\n"
	"	position.y += lookup.y;											    	\n"
	"   gl_Position = gl_ModelViewProjectionMatrix * position;		    		\n"
	"																			\n"
	"	vec3 mvVertex = vec3(gl_ModelViewMatrix * position);			    	\n"
	"	lightDir = vec3(gl_LightSource[0].position.xyz - mvVertex);				\n"
	"	normal = gl_NormalMatrix * gl_Normal;									\n"
    "}																			\n"
};

const char fragmentShaderSource[] = 
{  
	"varying vec3 normal, lightDir;												\n"
	"varying vec4 colour;														\n"
	"void main (void)															\n"
	"{																			\n"
	"	// Defining The Material Colors											\n"
	"	vec4 DiffuseColor = colour;					            		    	\n"
	"																			\n"
	"	// Scaling The Input Vector To Length 1									\n"
	"	vec3 n_normal = normalize(normal);							        	\n"
	"	vec3 n_lightDir = normalize(lightDir);	                                \n"
	"																			\n"
	"	// Calculating The Diffuse Term And Clamping It To [0;1]				\n"
	"	float DiffuseTerm = clamp(dot(n_normal, n_lightDir), 0.0, 1.0);\n"
	"																			\n"
	"	// Calculating The Final Color											\n"
	"	gl_FragColor = DiffuseColor * DiffuseTerm;								\n"
	"																			\n"
	"}																			\n"
};

//GPU Kernels

__global__ void output_FloodCell_agent_to_VBO(xmachine_memory_FloodCell_list* agents, glm::vec4* vbo, glm::vec3 centralise, int population_width){

	//global thread index
	int index = __mul24(blockIdx.x,blockDim.x) + threadIdx.x;

	vbo[index].x = 0.0;
	vbo[index].y = 0.0;
	vbo[index].z = 0.0;
	
    vbo[index].x = agents->x[index] - centralise.x;
    vbo[index].y = agents->y[index] - centralise.y;
    vbo[index].z = 1.0f - ((agents->z0[index] - Z0_MIN )/ Z0_RANGE);
    vbo[index].w = ((agents->h[index] - H_MIN)/ H_RANGE); //inverse
}


void initVisualisation()
{
	// Create GL context
	int   argc   = 1;
        char glutString[] = "GLUT application"; 
	char *argv[] = {glutString, NULL};
	//char *argv[] = {"GLUT application", NULL};
	glutInit( &argc, argv);
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize( WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow( "FLAME GPU Visualiser");

	// initialize GL
	if( !initGL()) {
			return;
	}
	initShader();

	//load nav map model		//added
	initNavMapPopulation();		//added

	//load pedestrians
	initPedestrianPopulation();	//added

	//initialise input control
	initInputConroller();		//added

	//init menu
	initMenuItems();			//added

	//FPS
	start_time = 0;
	end_time = 0;
	frame_time = 0;
	fps = 0;
	frames = 0;
	av_frames = 25;

	// register callbacks
	glutReshapeFunc( reshape);
	glutDisplayFunc( display);
	glutKeyboardFunc( keyboard);
	glutSpecialFunc( special);
	glutMouseFunc( mouse);
	//glutMotionFunc( motion);
    
	// create VBO's
	createVBO( &sphereVerts, SPHERE_SLICES* (SPHERE_STACKS+1) * sizeof(glm::vec3));
	createVBO( &sphereNormals, SPHERE_SLICES* (SPHERE_STACKS+1) * sizeof (glm::vec3));
	setVertexBufferData();

	// create TBO
	createTBO(&FloodCell_Default_cgr, &FloodCell_Default_tbo, &FloodCell_Default_displacementTex, xmachine_memory_FloodCell_MAX * sizeof( glm::vec4));
	

	//set shader uniforms
	glUseProgram(shaderProgram);

	//Flood visualisation setup
	//create a events for timer
	//cudaEventCreate(&start);
	//cudaEventCreate(&stop);
}

void runVisualisation(){

	//init FLAME GPU globals controller after 0.xml has been processed.
	initGlobalsController(); //added
	// Ensure all texts are updated
	updateAllTexts();		//added

	// start rendering mainloop
	glutMainLoop();
}

////////////////////////////////////////////////////////////////////////////////
//! Run the Cuda part of the computation
////////////////////////////////////////////////////////////////////////////////
void runCuda()
{
	if(!paused){
#ifdef SIMULATION_DELAY
	delay_count++;
	if (delay_count == SIMULATION_DELAY){
		delay_count = 0;
		singleIteration();
	}
#else
	singleIteration();
#endif
	}

	//kernals sizes
	int threads_per_tile = 256;
	int tile_size;
	dim3 grid;
	dim3 threads;
	glm::vec3 centralise;

	//pointer
	glm::vec4 *dptr;

	
	if (get_agent_FloodCell_Default_count() > 0)
	{
		// map OpenGL buffer object for writing from CUDA
        size_t accessibleBufferSize = 0;
        gpuErrchk(cudaGraphicsMapResources(1, &FloodCell_Default_cgr));
		gpuErrchk(cudaGraphicsResourceGetMappedPointer( (void**)&dptr, &accessibleBufferSize, FloodCell_Default_cgr));
		//cuda block size
		tile_size = (int) ceil((float)get_agent_FloodCell_Default_count()/threads_per_tile);
		grid = dim3(tile_size, 1, 1);
		threads = dim3(threads_per_tile, 1, 1);
        //discrete variables
        int population_width = (int)floor(sqrt((float)get_agent_FloodCell_Default_count()));
		centralise.x = population_width / 2.0;
        centralise.y = population_width / 2.0;
        centralise.z = 0.0;
        
		output_FloodCell_agent_to_VBO<< < grid, threads>>>(get_device_FloodCell_Default_agents(), dptr, centralise, population_width);
		gpuErrchkLaunch();
		// unmap buffer object
        gpuErrchk(cudaGraphicsUnmapResources(1, &FloodCell_Default_cgr));
	}
	
}

////////////////////////////////////////////////////////////////////////////////
//! Initialize GL
////////////////////////////////////////////////////////////////////////////////
int initGL()
{
	// initialize necessary OpenGL extensions
	glewInit();
	if (! glewIsSupported( "GL_VERSION_2_0 " 
		"GL_ARB_pixel_buffer_object")) {
		fprintf( stderr, "ERROR: Support for necessary OpenGL extensions missing.\n");
		fflush( stderr);
		//return 1; // from flood model
		return FALSE;
	}

	// default initialization
	glClearColor( 1.0, 1.0, 1.0, 1.0);
	glEnable( GL_DEPTH_TEST);

	reshape(WINDOW_WIDTH, WINDOW_HEIGHT);
	checkGLError();

	//lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	//return 1; // from flood model
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//! Initialize GLSL Vertex Shader
////////////////////////////////////////////////////////////////////////////////
void initShader()
{
	const char* v = vertexShaderSource;
	const char* f = fragmentShaderSource;

	//vertex shader
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &v, 0);
	glCompileShader(vertexShader);

	//fragment shader
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &f, 0);
	glCompileShader(fragmentShader);

	//program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// check for errors
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		printf("ERROR: Shader Compilation Error\n");
		char data[262144];
		int len;
		glGetShaderInfoLog(vertexShader, 262144, &len, data); 
		printf("%s", data);
	}
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		printf("ERROR: Shader Compilation Error\n");
		char data[262144];
		int len;
		glGetShaderInfoLog(fragmentShader, 262144, &len, data); 
		printf("%s", data);
	}
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (status == GL_FALSE){
		printf("ERROR: Shader Program Link Error\n");
	}

	// get shader variables
	vs_displacementMap = glGetUniformLocation(shaderProgram, "displacementMap");
	vs_mapIndex = glGetAttribLocation(shaderProgram, "mapIndex"); 
	vs_water = glGetUniformLocation(shaderProgram, "water");
}

////////////////////////////////////////////////////////////////////////////////
//! Create VBO (Vertext buffer object)
////////////////////////////////////////////////////////////////////////////////
void createVBO(GLuint* vbo, GLuint size)
{
	// create buffer object
	glGenBuffers( 1, vbo);
	glBindBuffer( GL_ARRAY_BUFFER, *vbo);

	// initialize buffer object
	glBufferData( GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);

	glBindBuffer( GL_ARRAY_BUFFER, 0);

	checkGLError();
}

////////////////////////////////////////////////////////////////////////////////
//! Delete VBO // commented 
////////////////////////////////////////////////////////////////////////////////
//void deleteVBO( GLuint* vbo)
//{
//	glBindBuffer( 1, *vbo);
//	glDeleteBuffers( 1, vbo);
//
//	*vbo = 0;
//}

////////////////////////////////////////////////////////////////////////////////
//! Create TBO
////////////////////////////////////////////////////////////////////////////////
void createTBO(cudaGraphicsResource_t* cudaResource, GLuint* tbo, GLuint* tex, GLuint size)
{
	// create buffer object
	glGenBuffers( 1, tbo);
	glBindBuffer( GL_TEXTURE_BUFFER_EXT, *tbo);

	// initialize buffer object
	glBufferData( GL_TEXTURE_BUFFER_EXT, size, 0, GL_DYNAMIC_DRAW);

	//tex
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_BUFFER_EXT, *tex);
	glTexBufferEXT(GL_TEXTURE_BUFFER_EXT, GL_RGBA32F_ARB, *tbo); 
	glBindBuffer(GL_TEXTURE_BUFFER_EXT, 0);

    // register buffer object with CUDA
    gpuErrchk(cudaGraphicsGLRegisterBuffer(cudaResource, *tbo, cudaGraphicsMapFlagsWriteDiscard)); 

    checkGLError();
}

////////////////////////////////////////////////////////////////////////////////
//! Delete TBO
////////////////////////////////////////////////////////////////////////////////
void deleteTBO(cudaGraphicsResource_t* cudaResource,  GLuint* tbo)
{
    gpuErrchk(cudaGraphicsUnregisterResource(*cudaResource));
    *cudaResource = 0;

    glBindBuffer( 1, *tbo);
    glDeleteBuffers( 1, tbo);

	*tbo = 0;
}

////////////////////////////////////////////////////////////////////////////////
//! Set Sphere Vertex Data
////////////////////////////////////////////////////////////////////////////////

static void setSphereVertex(glm::vec3* data, int slice, int stack) {
	float PI = 3.14159265358;
    
	double sl = 2*PI*slice/SPHERE_SLICES;
	double st = 2*PI*stack/SPHERE_STACKS;
 
	data->x = cos(st)*sin(sl) * SPHERE_RADIUS;
	data->y = sin(st)*sin(sl) * SPHERE_RADIUS;
	data->z = cos(sl) * SPHERE_RADIUS;
}


////////////////////////////////////////////////////////////////////////////////
//! Set Sphere Normal Data
////////////////////////////////////////////////////////////////////////////////

static void setSphereNormal(glm::vec3* data, int slice, int stack) {
	float PI = 3.14159265358;
    
	double sl = 2*PI*slice/SPHERE_SLICES;
	double st = 2*PI*stack/SPHERE_STACKS;
 
	data->x = cos(st)*sin(sl);
	data->y = sin(st)*sin(sl);
	data->z = cos(sl);
}


////////////////////////////////////////////////////////////////////////////////
//! Set Vertex Buffer Data
////////////////////////////////////////////////////////////////////////////////
void setVertexBufferData()
{
	int slice, stack;
	int i;

	// upload vertex points data
	glBindBuffer(GL_ARRAY_BUFFER, sphereVerts);
	glm::vec3* verts =( glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	i = 0;
	for (slice=0; slice<SPHERE_SLICES/2; slice++) {
		for (stack=0; stack<=SPHERE_STACKS; stack++) {
			setSphereVertex(&verts[i++], slice, stack);
			setSphereVertex(&verts[i++], slice+1, stack);
		}
    }
	glUnmapBuffer(GL_ARRAY_BUFFER);

	// upload vertex normal data
	glBindBuffer(GL_ARRAY_BUFFER, sphereNormals);
	glm::vec3* normals =( glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	i = 0;
	for (slice=0; slice<SPHERE_SLICES/2; slice++) {
		for (stack=0; stack<=SPHERE_STACKS; stack++) {
			setSphereNormal(&normals[i++], slice, stack);
			setSphereNormal(&normals[i++], slice+1, stack);
		}
    }
	glUnmapBuffer(GL_ARRAY_BUFFER);
}




////////////////////////////////////////////////////////////////////////////////
//! Reshape callback
////////////////////////////////////////////////////////////////////////////////

void reshape(int width, int height){
	// viewport
	glViewport( 0, 0, width, height);

	// projection
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(FOVY, (GLfloat)width / (GLfloat) height, NEAR_CLIP, FAR_CLIP);
	glMatrixMode(GL_MODELVIEW);								//added
	glLoadIdentity();										//added
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);				//added
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//added This is embedded in display function


	checkGLError();
}


////////////////////////////////////////////////////////////////////////////////
//! Display callback
////////////////////////////////////////////////////////////////////////////////
void display()
{
	//float millis;
	
	//CUDA start Timing
	//cudaEventRecord(start); //flood model
	start_time = clock();


	// run CUDA kernel to generate vertex positions
	runCuda();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//lookat
	gluLookAt(eye[0], eye[1], eye[2], look[0], look[1], look[2], up[0], up[1], up[2]); //added

	//zoom
	glTranslatef(0.0, 0.0, translate_z); 
	//move
	glRotatef(rotate_x, 1.0, 0.0, 0.0);
	glRotatef(rotate_y, 0.0, 0.0, 1.0);


	//Set light position
	//glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_POSITION); // flood mode/ commented

	//set water drawing state
	glUniform1i(vs_water, water);
	
	//Draw FloodCell Agents in Default state
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER_EXT, FloodCell_Default_displacementTex);

	//loop
	for (int i=0; i< get_agent_FloodCell_Default_count(); i++){
		glVertexAttrib1f(vs_mapIndex, (float)i);
		
		//draw using vertex and attribute data on the gpu (fast)
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVerts);
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, sphereNormals);
		glNormalPointer(GL_FLOAT, 0, 0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, SPHERE_SLICES * (SPHERE_STACKS+1));

		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	
	stepFLAMESimulation();							//added
	renderNavMapPopulation();						//added
	renderPedestrianPopulation();					//added

	drawInfoDisplay(WINDOW_WIDTH, WINDOW_HEIGHT);	//added
	drawMenuDisplay(WINDOW_WIDTH, WINDOW_HEIGHT);	//added

	//end timing
	glFinish();
	end_time = clock();
	frame_time += (end_time - start_time);
	if (frames == av_frames){
		fps = (float)av_frames / (frame_time / (float)CLOCKS_PER_SEC);
		updateAllEmmsionRatesTexts("");
		frames = 0;
		frame_time = 0.0f;
	}
	else{
		frames++;
	}
		

	//CUDA stop timing  // flood model method
	//cudaEventRecord(stop);
	//glFlush();
	//cudaEventSynchronize(stop);
	//cudaEventElapsedTime(&millis, start, stop);
	//frame_time += millis;

	// Flood model time tracking
	//if(frame_count == display_rate){
	//	char title [100];
	//	sprintf(title, "Execution & Rendering Total: %f (FPS), %f milliseconds per frame", display_rate/(frame_time/1000.0f), frame_time/display_rate);
	//	glutSetWindowTitle(title);

	//	//reset
	//	frame_count = 0;
 //   frame_time = 0.0;
	//}else{
	//	frame_count++;
	//}

	int display_rate = 50 ;

	if (frames == display_rate){
		char title[100];
		sprintf(title, "Execution & Rendering Total: %f (FPS), %f milliseconds per frame", display_rate / (frame_time / 1000.0f), frame_time / display_rate);
		glutSetWindowTitle(title);

		//reset
		frames = 0;
		frame_time = 0.0;
	}
	else{
		frames++;
	}


	//redraw
	glutSwapBuffers();
	glutPostRedisplay();

}

void toggleFullScreenMode()
{
	fullScreenMode = !fullScreenMode;
	glutFullScreen();
}

////////////////////////////////////////////////////////////////////////////////
//! Keyboard events handler /// commented
////////////////////////////////////////////////////////////////////////////////
//void keyboard( unsigned char key, int /*x*/, int /*y*/)
//{
//	switch( key) {
//	// Space == 32
//    case(32):
//        paused = !paused;
//        break;
//	case ('w'):
//		water = !water;
//		printf("Water rending state is %d\n", water);
//		break;
//    // Esc == 27
//	case(27) :
//		deleteVBO( &sphereVerts);
//		deleteVBO( &sphereNormals);
//		
//		deleteTBO( &FloodCell_Default_cgr, &FloodCell_Default_tbo);
//		
//		//cudaEventDestroy(start);
//		//cudaEventDestroy(stop);
//		exit(EXIT_SUCCESS);
//	}
//}




void special(int key, int x, int y){
    switch (key)
    {
    case(GLUT_KEY_RIGHT) :
        singleIteration();
        fflush(stdout);
        break;
    }
}


////////////////////////////////////////////////////////////////////////////////
//! Mouse event handlers		/// commented to rely on GLUTinputController
////////////////////////////////////////////////////////////////////////////////
//void mouse(int button, int state, int x, int y)
//{
//	if (state == GLUT_DOWN) {
//		mouse_buttons |= 1<<button;
//	} else if (state == GLUT_UP) {
//		mouse_buttons = 0;
//	}
//
//	mouse_old_x = x;
//	mouse_old_y = y;
//	glutPostRedisplay();
//}
//
//void motion(int x, int y)
//{
//	float dx, dy;
//	dx = x - mouse_old_x;
//	dy = y - mouse_old_y;
//
//	if (mouse_buttons & 1) {
//		rotate_x += dy * 0.2;
//		rotate_y += dx * 0.2;
//	} else if (mouse_buttons & 4) {
//		translate_z += dy * VIEW_DISTANCE * 0.001;
//	}
//
//  mouse_old_x = x;
//  mouse_old_y = y;
//}
//
float getFPS()
{
	return fps;
}

void checkGLError(){
  int Error;
  if((Error = glGetError()) != GL_NO_ERROR)
  {
    const char* Message = (const char*)gluErrorString(Error);
    fprintf(stderr, "OpenGL Error : %s\n", Message);
  }
}
