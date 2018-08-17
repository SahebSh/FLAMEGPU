/*
 * Copyright 2011 University of Sheffield.
 * Author: Dr Paul Richmond 
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
#ifndef __VISUALISATION_H
#define __VISUALISATION_H

#ifndef FALSE					//added
enum BOOLEAN { FALSE, TRUE };	//added
typedef enum BOOLEAN BOOLEAN;	//added
#endif


enum TOGGLE_STATE { TOGGLE_OFF, TOGGLE_ON };	//added
typedef enum TOGGLE_STATE TOGGLE_STATE;			//added

#define SIMULATION_DELAY 20

// flood model specific ranges
#define Z0_MIN 0.0f
#define Z0_MAX 4.0f

#define H_MIN 0.0f
#define H_MAX 3.0f

#define ENV_MAX 1.0f			//added
#define ENV_MIN -ENV_MAX		//added
#define ENV_WIDTH (2*ENV_MAX)	//added

// prototypes
void toggleFullScreenMode();
void checkGLError();
float getFPS();


// constants
//const unsigned int WINDOW_WIDTH = 512;
//const unsigned int WINDOW_HEIGHT = 512;
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

//frustrum
const double NEAR_CLIP = 0.1;
const double FAR_CLIP = 300;

//Circle model fidelity
const int SPHERE_SLICES = 8;
const int SPHERE_STACKS = 8;
const double SPHERE_RADIUS = 1;

//Viewing Distance
const double VIEW_DISTANCE = 256;

//external functions
extern void stepFLAMESimulation(); //added


//light position
//GLfloat LIGHT_POSITION[] = { 0.0, 0.0, 1000.0f, 0.0f };
//GLfloat LIGHT_POSITION[] = {0.0, 0.0, 0.0f, 0.0f}; //flood default

//GLfloat LIGHT_POSITION[] = { 25.0, 25.0f, 25.0f, 1.0f }; // pedestrian default

#endif __VISUALISATION_H