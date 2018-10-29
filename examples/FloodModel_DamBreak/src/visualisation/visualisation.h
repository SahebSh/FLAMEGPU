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

#define SIMULATION_DELAY 20

// flood model specific ranges
#define Z0_MIN 0.0f
#define Z0_MAX 4.0f

#define H_MIN 0.0f
#define H_MAX 3.0f


// constants
const unsigned int WINDOW_WIDTH = 512;
const unsigned int WINDOW_HEIGHT = 512;

//frustrum
const double NEAR_CLIP = 0.1;
const double FAR_CLIP = 300;

//Circle model fidelity
const int SPHERE_SLICES = 8;
const int SPHERE_STACKS = 8;
const double SPHERE_RADIUS = 1;

//Viewing Distance
const double VIEW_DISTANCE = 256;


//light position
//GLfloat LIGHT_POSITION[] = { 0.0, 0.0, 1000.0f, 0.0f };
GLfloat LIGHT_POSITION[] = {0.0, 0.0, 0.0f, 0.0f};

#endif __VISUALISATION_H