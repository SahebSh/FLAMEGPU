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

#ifndef _FLAMEGPU_FUNCTIONS
#define _FLAMEGPU_FUNCTIONS

#include "header.h"
#include "CustomVisualisation.h"
#include "cutil_math.h"

 // This is to output the computational time for each message function within each iteration (added by MS22May2018) 
 //#define INSTRUMENT_ITERATIONS 1
 //#define INSTRUMENT_AGENT_FUNCTIONS 1
 //#define OUTPUT_POPULATION_PER_ITERATION 1

 // global constant of the flood model
#define epsilon				1.0e-3
#define emsmall				1.0e-12 
#define GRAVITY				9.80665 
//#define GLOBAL_MANNING		0.018000
#define GLOBAL_MANNING		0.01100 // clear cement for the shopping centre area
#define CFL					0.5
#define TOL_H				10.0e-4
#define BIG_NUMBER			800000

// Spatial and timing scalers for pedestrian model
//#define SCALE_FACTOR		0.03125
//#define I_SCALER			(SCALE_FACTOR*0.35f)
#define MESSAGE_RADIUS		d_message_pedestrian_location_radius
//#define MIN_DISTANCE		0.0001f


//defining dengerous water flow
#define DANGEROUS_H			0.25		//treshold for dengerous height of water (m) or 9 kmph
#define DANGEROUS_V			2.5			//treshold for dengerous relevant velocity of water flow (m/s)

// Defining the state of pedestrians
#define STATE_ALIVE_DRY		1
#define STATE_ALIVE_RUN		2
#define STATE_ALIVE_WALK	3
#define STATE_ALIVE_STILL	4
#define STATE_DEAD			0

//Flood severity to set to emergency alarm
#define NO_ALARM			0
#define LOW_FLOOD			1
#define MODERATE_FLOOD		2
#define SIGNIFICANT_FLOOD	3
#define EXTREME_FLOOD		4

//// To stop choosing regular exits and stop emitting pedestrians in the domain
#define ON			1
#define OFF			0

#define PI 3.1415f
#define RADIANS(x) (PI / 180.0f) * x

__FLAME_GPU_INIT_FUNC__ void initConstants()
{
	// This function assign initial values to DXL, DYL, and dt

	//double dt_init; //commented to load dt from 0.xml
	double sim_init = 0;

	// initial water depth to support initial inflow discharge at the inflow boundary (in meters)
	double init_depth_boundary = 0.010;

	// loading constant variables in host function
	int x_min, x_max, y_min, y_max;
	x_min = *get_xmin();
	x_max = *get_xmax();
	y_min = *get_ymin();
	y_max = *get_ymax();
	


	// the size of the computaional cell (flood agents)
	// sqrt(xmachine_memory_FloodCell_MAX) is the number of agents in each direction. 
	double dxl = (x_max - x_min) / sqrt(xmachine_memory_FloodCell_MAX);
	double dyl = (y_max - y_min) / sqrt(xmachine_memory_FloodCell_MAX);

	////////// Adaptive time-step function to assign initial dt value based on CFL criteria //////////// no need to be uncommented if there is no initial discharge/depth of water
	//// for each flood agent in the state default
	// NOTE: this function is only applicable where the dam-break cases are studied in which there is initial depth of water
	/*for (int index = 0; index < get_agent_FloodCell_Default_count(); index++)
	{*/
	//Loading data from the agents
	//double hp = get_FloodCell_Default_variable_h(index);
	//if (hp > TOL_H)
	//{
	//	double qx = get_FloodCell_Default_variable_qx(index);
	//	double qy = get_FloodCell_Default_variable_qy(index);
	//	double up = qx / hp;
	//	double vp = qy / hp;
	//	dt_init = fminf(CFL * dxl / (fabs(up) + sqrt(GRAVITY * hp)), CFL * dyl / (fabs(vp) + sqrt(GRAVITY * hp)));
	//}
	//store for timestep calc
	//dt_init = fminf(dt_init, dt_xy);
	//}
	/////////////////////////////////////////////////////////////////////////////////

	set_DXL(&dxl);
	set_DYL(&dyl);
	set_sim_time(&sim_init);
	set_init_depth_boundary(&init_depth_boundary);


	// Assigning pedestrian model default values to steering force parameters
	//float STEER_WEIGHT = 0.10f;
	//float AVOID_WEIGHT = 0.020f;
	//float COLLISION_WEIGHT = 0.5 ;
	//float GOAL_WEIGHT = 0.20f;
	//float TIME_SCALER = 0.007f; // it is purposefully selected. Be very careful while changing it

	int exit_state_on = 1; 

	//set_STEER_WEIGHT(&STEER_WEIGHT);
	//set_AVOID_WEIGHT(&AVOID_WEIGHT);
	//set_COLLISION_WEIGHT(&COLLISION_WEIGHT);
	//set_GOAL_WEIGHT(&GOAL_WEIGHT);
	//set_TIME_SCALER(&TIME_SCALER);

	// enabling the exits in the domain
	set_EXIT1_STATE(&exit_state_on);
	set_EXIT2_STATE(&exit_state_on);
	set_EXIT3_STATE(&exit_state_on);
	set_EXIT4_STATE(&exit_state_on);
	set_EXIT5_STATE(&exit_state_on);
	set_EXIT6_STATE(&exit_state_on);
	set_EXIT7_STATE(&exit_state_on);
	set_EXIT8_STATE(&exit_state_on);
	set_EXIT9_STATE(&exit_state_on);
	set_EXIT10_STATE(&exit_state_on);


	// loading the pre-defined dimension of sandbags in 0.xml file
	float sandbag_length = *get_sandbag_length();
	float sandbag_width  = *get_sandbag_width();

	// calculating the area that one single sandbag can possibly occupy
	float area_sandbags  = sandbag_length * sandbag_width ;
	// The area based on the resolution of the domain
	float area_floodcell = dxl * dyl;
	
	// an integer to calculate how many sandbag is required to cover the surface of flood agent
	int fill_area;
	
	// for fine resolution studies where the area the surface of flood agents is less than the sandbags' surface area
	if (area_floodcell < area_sandbags)
		area_floodcell = area_sandbags;

	fill_area = rintf((float)area_floodcell / (float)area_sandbags);
		

	// update the global constant variable that shows the number of sandbags to fill one single navmap/flood agent
	set_fill_cap(&fill_area);

}

// assigning dt for the next iteration
__FLAME_GPU_STEP_FUNC__ void DELTA_T_func()
{
	//// This function comprises following functions: (to be operated after each iteration of the simulation)
	//	1- Set time-step (dt) as a global variable defined in XMLModelFile.xml-> environment: 
	//				* Operates on two different time-steps, it can both take a user-defined dt (in 0.xml file) 
	//				  for the simulation of pedestrians (where the there is no water flow and no need for stablising 
	//				  the numerical solution by defining appropriate dt), AND it can set another time-step where 
	//				  there is a water flow and the model needs to be stabilised. The later one can alternatively be set to  
	//				  work on adaptive time stepping by taking the minimum calculated time-step of all the flood agents in the model
	//			
	//	2- Calculates the simulation time from the start of the simulation, with respect to the defined time-step (see 1)
	//			
	//	3- Sets the number of hero pedestrians based on the pre-defined percentage in 0.xml file 'hero_percentage'
	//			
	//	4- Counts the number of pedestrians based on their states with respect to the flood water (e.g. dead, alive, etc)
	//	
	//	5- Issue an emergency alarm based on maximum depth and velocity of the water, it operates based on Hazard Rate (HR) 
	//		defined by Enrironment Agency EA guidence document, it operates to define four different situations
	//		ALSO, when the evacuation time is reached, the emergency alarm is issued prior to the start of the flood, however, 
	//		if there is no degined evacuation time, once the flooding starts, the emergency alarm is issued and everyone is 
	//		supposed to evacuate the area instantly. 
	//	
	//	6- The probablity of exit points and emission rate is modified to force the pedestrians using only the 
	//		emergency exit defined by user in 0.xml file. ALSO, in the time of evacuation there is no emission of the pedestrians
	//		from the exit points
	//	
	//	7- Set the number of sandbags required to fill one single navmap/flood agent in order to increase the topography
	//		up to the height of sandbags
	//
	//	8- Calculates the extended length, and updates the number of layers which is being applied and already applied
	//		more explanation: There is a need to update the number of layers once one layer is applied, e.g. once the first layer 
	//		is completed along the length of defined dike, then the second layer is expected to be applied. Subsequently, 
	//		based on the number of layers, the topography is raised with respect to the height of a sandbag. 
	//
	//	9- Finishes sandbagging process once it reaches to the defined height of dike
	//
	// **** WHAT is missing (for MS to consider later): (MS comments)
	//		1- the model does not observe the proposed width of sandbag dike
	//
	

	//get the old values
	double old_sim_time = *get_sim_time();

	//Loading the data from memory. activated to load dt from 0.xml ()
	double old_dt = *get_dt(); 
	double dt_ped = *get_dt_ped();
	double dt_flood = *get_dt_flood();
	// new variable for dt
	double new_dt;

	// setting general dt equal to pedestrian dt initially since there is no flood at the start of the simulation
	//set_dt(&dt_ped);


	// loading the global constant varible
	int emergency_alarm = *get_emer_alarm();
	// take the status of evacuation
	int evacuation_stat = *get_evacuation_on();
	// when to evacuate
	double evacuation_start = *get_evacuation_start_time();
	//where the safe haven is
	int emergency_exit_number = *get_evacuation_exit_number();

	// to enable the adaptive time-step uncomment this line
	// Defining the time_step of the simulation with contributation of CFL
	double minTimeStep = min_FloodCell_Default_timeStep_variable(); // for adaptive time stepping
																	//double minTimeStep = 0.005; // for static time-stepping and measuring computation time

	//Take the maximum height of water in the domain
	double flow_h_max = max_FloodCell_Default_h_variable();
	
	// loading the time scaler from the last iteration
	//float TIME_SCALER_ped = *get_TIME_SCALER_INIT(); // commented MS27092019 16:06
	//float TIME_SCALER = *get_TIME_SCALER(); // commented MS27092019 16:06
	
	// TIME_SCALER based on flood timestep
	//float TIME_SCALER_flood;
	
	// update the dt_ped from the last iteration 
	//double dt_ped = min_agent_default_dt_ped_variable();

	int auto_dt_on = *get_auto_dt_on();
	//setting dt to that of defined for flood model to preserve the stability of numerical solutions for flood water
	// can either be set to static for (in which dt is set based on the defined value in 0.xml file) or either adaptive value (minTimeStep)
	new_dt = dt_ped; 

	if (flow_h_max > epsilon)
	{
		new_dt = dt_flood;

		if (auto_dt_on == ON)
		{
			new_dt = minTimeStep;
		}
	}
	
		
	// Setting new time step to the simulation
	set_dt(&new_dt);


	// to track the simulation time
	//double new_sim_time = old_sim_time + minTimeStep; //commented to load from 0.xml
	double new_sim_time = old_sim_time + old_dt;

	set_sim_time(&new_sim_time);

	// load the number of pedestrians in each iteration
	int no_pedestrians = get_agent_agent_default_count();

	// loading the percentage of hero pedestrians
	float hero_percentage = *get_hero_percentage();
	// calculating the number of hero pedestrians with respect to their percentage in the population
	int hero_population = (float)hero_percentage * (int)no_pedestrians;

	// Store the number of pedestrians in each iteration
	set_count_population(&no_pedestrians);

	//store the population of hero pedestrians
	set_hero_population(&hero_population);

	// counter constants 
	int count_trapped = 0;
	int count_in_dry = 0;
	int count_wetted = 0;
	int count_alive_disrupted1 = 0;
	int count_alive_disrupted2 = 0;
	int count_heros = 0;

	// loop over the number of pedestrians
	for (int index = 0 ; index < no_pedestrians; index++)
	{
		int pedestrians_state = get_agent_default_variable_HR_state(index);

		// counting the number of pedestrians with different states
		if (pedestrians_state == STATE_DEAD)
		{
			count_trapped++;
		}
		else if (pedestrians_state == STATE_ALIVE_DRY)
		{
			count_in_dry++;
		}
		else if (pedestrians_state == STATE_ALIVE_RUN)
		{
			count_wetted++;
		}
		else if (pedestrians_state == STATE_ALIVE_WALK)
		{
			count_alive_disrupted1++;
		}
		else if (pedestrians_state == STATE_ALIVE_STILL)
		{
			count_alive_disrupted2++;
		}

		int pedestrian_hero_status = get_agent_default_variable_hero_status(index);
		
		if (pedestrian_hero_status == 1)
		{
			count_heros++;
		}

	}

	// to track the number of existing heros
	set_count_heros(&count_heros);
	
	//Get the number of navmap agents for looping over the domain
	//int no_navmap = get_agent_navmap_static_count();
	int no_FloodCells = get_agent_FloodCell_Default_count();

	
	//a value to track the maximum velocity, initialising to minimum possible double 
	double flow_velocity_max = DBL_MIN;
	double flow_qxy_max = DBL_MIN;

	int wet_counter; // a variable to count the number of floodcell agents with depth of water (for average estimation of the flow variables)
	
	double sum_depth = DBL_MIN;
	//double ave_depth = DBL_MIN;

	double sum_velx = DBL_MIN;
	//double ave_velx = DBL_MIN;

	double sum_vely = DBL_MIN;
	//double ave_vely = DBL_MIN;

	//double ave_velxy = DBL_MIN;

	// Hazard rate, will be calculated locally (by local depth and velocity) for each agent then the maximum of all will be used 
	double HR_loc; //
	// to estimate maximum HR
	double HR = DBL_MIN;

	for (int index = 0; index < no_FloodCells; index++)
	{
		// Loading water flow info from navmap cells
		double flow_h = get_FloodCell_Default_variable_h(index);

		// counting the number of wet flood agents (for further estimation of average flow values)
		if ( flow_h > epsilon )
			wet_counter++; 
		
		// calculating velocities
		double flow_velocity_x = get_FloodCell_Default_variable_qx(index) / (float)flow_h;
		double flow_velocity_y = get_FloodCell_Default_variable_qy(index) / (float)flow_h;

		double flow_velocity_xy = max(flow_velocity_x, flow_velocity_y);  // taking maximum velocity between both x and y direction
		double flow_discharge_xy = max(get_FloodCell_Default_variable_qx(index), get_FloodCell_Default_variable_qy(index)); // taking maximum discharge between both x and y direction

		//calculating local HR, taking each agent individually
		HR_loc = flow_h * (flow_velocity_xy + 0.5);
		// taking maximum HR between all the agents
		if (HR_loc > HR)
			HR = HR_loc;

	    // summming up the depth of water over the domain (for averages' approximation)
		if (flow_h > epsilon)
			sum_depth += flow_h;
		else
			sum_depth += 0.0;
		 

		// summming up the velocity of water in x-axis direction over the domain
		if (fabs(flow_velocity_x) > 0.0 ) // excluding not appropriate data
			sum_velx += (float)flow_velocity_x;
		else
			sum_velx += 0;

		// summming up the velocity of water in y-axis direction over the domain
		if (fabs(flow_velocity_y) > 0.0) // excluding not appropriate data
			sum_vely += (float)flow_velocity_y;
		else
			sum_vely += 0;
		
		// taking maximum velocity between all the agents
		if (flow_velocity_xy > flow_velocity_max)
		{
			flow_velocity_max = flow_velocity_xy;
		}
		// taking maximum discharge between all the agents
		if (flow_discharge_xy > flow_qxy_max)
		{
			flow_qxy_max = flow_discharge_xy;
		}
	}

	//printf("\n NUMBER OF WETTED FLOODCELL AGENTS  = %d  \n", wet_counter);

	// calculating the average water depth over wet domain (eliminate taking into account the dry zones)
	//ave_depth = (float)sum_depth / (float)wet_counter;
	//ave_velx  = (float)fabs(sum_velx) / (float)wet_counter;
	//ave_vely  = (float)fabs(sum_vely) / (float)wet_counter;

	// taking the maximum averaged velocity 
	//ave_velxy = max(ave_velx, ave_vely);


	// calculatin hazard rating for emergency alarm status
	//double HR = flow_h_max * (flow_velocity_max + 0.5); // calculating HR based on maximum values
	//double HR = ave_depth * (ave_velxy + 0.5); // calculating HR based on average values
	
	// Defining emergency alarm based on maximum velocity and height of water based on EA's guidence document
	// decide the emergency alarm after the first iteration (in whicn emergency alarm is set to NO_ALARM)
	//if (evacuation_stat == ON)
	//{
		if (emergency_alarm == NO_ALARM) //
		{
			if (HR <= epsilon)
			{
				emergency_alarm = NO_ALARM;
			}
			else if (HR <= 0.75)
			{
				emergency_alarm = LOW_FLOOD;
			}
			else if (HR > 0.75 && HR <= 1.5)
			{
				emergency_alarm = MODERATE_FLOOD;
			}
			else if (HR > 1.5 && HR <= 2.5)
			{
				emergency_alarm = SIGNIFICANT_FLOOD;
			}
			else if (HR > 2.5)
			{
				emergency_alarm = EXTREME_FLOOD;
			}
		}
		else if (emergency_alarm == LOW_FLOOD) //
		{
			if (HR > 0.75 && HR <= 1.5)
			{
				emergency_alarm = MODERATE_FLOOD;
			}
			else if (HR > 1.5 && HR <= 2.5)
			{
				emergency_alarm = SIGNIFICANT_FLOOD;
			}
			else if (HR > 2.5)
			{
				emergency_alarm = EXTREME_FLOOD;
			}
		}
		else if (emergency_alarm == MODERATE_FLOOD) //
		{
			if (HR > 1.5 && HR <= 2.5)
			{
				emergency_alarm = SIGNIFICANT_FLOOD;
			}
			else if (HR > 2.5)
			{
				emergency_alarm = EXTREME_FLOOD;
			}
		}
		else if (emergency_alarm == SIGNIFICANT_FLOOD) //
		{

			if (HR > 2.5)
			{
				emergency_alarm = EXTREME_FLOOD;
			}
		}
	//}

	// Issuing evacuation siren when the defined evacuation start time (evacuation_start_time) is reached. This alarm will vary within each iteration
	// with respect to the maximum height and velocity of the water to show the severity of the flood inundation.
	if (evacuation_stat == ON)
	{
		if (new_sim_time >= evacuation_start)
		{
			emergency_alarm = LOW_FLOOD;
		}
	}
	

	// Assigning the value of emergency alarm to 'emer_alarm' constant variable in each iteration
	set_emer_alarm(&emergency_alarm);

	// To assign zero values to the constant variables governing the emission rate and the probablity of exits (DO NOT FORGET TO UNCOMMENT IF EVACUATION STRATEGY NEEDED)
	//float dont_emit = 0.0f;
	//int dont_exit = 0;
	//int do_exit = 10;

	int body_effect_on = *get_body_effect_on();
	
	// Changing the probability and emission // has to be applied for 7 Exits available in the model (number of exits can be extended, 
	// but needs further compilation, after generating the inputs for pedestrians, the flood model needs to be modified to operate on the
	// the desired number of exits)

	// These lines of logical statements govern the emergence rate and accessability of the exits in the domain, commented for Flume study (MS09092019)
		//////if ((emergency_alarm > NO_ALARM) && (body_effect_on == OFF)) // greater than zero
		//////{
		//////	// Stop emission of pedestian when emergency alarm is issued
		//////	set_EMMISION_RATE_EXIT1(&dont_emit);
		//////	set_EMMISION_RATE_EXIT2(&dont_emit);
		//////	set_EMMISION_RATE_EXIT3(&dont_emit);
		//////	set_EMMISION_RATE_EXIT4(&dont_emit);
		//////	set_EMMISION_RATE_EXIT5(&dont_emit);
		//////	set_EMMISION_RATE_EXIT6(&dont_emit);
		//////	set_EMMISION_RATE_EXIT7(&dont_emit);
		//////	set_EMMISION_RATE_EXIT8(&dont_emit);
		//////	set_EMMISION_RATE_EXIT9(&dont_emit);
		//////	set_EMMISION_RATE_EXIT10(&dont_emit);

		//////	// it is ONLY effective when new pedestrians are produced during the flooding (in case less emission rate is used)
		//////	// Kept here in purpose to keep this capability
		//////	if (emergency_exit_number == 1)
		//////	{
		//////		set_EXIT1_PROBABILITY(&do_exit);

		//////		set_EXIT2_PROBABILITY(&dont_exit);
		//////		set_EXIT3_PROBABILITY(&dont_exit);
		//////		set_EXIT4_PROBABILITY(&dont_exit);
		//////		set_EXIT5_PROBABILITY(&dont_exit);
		//////		set_EXIT6_PROBABILITY(&dont_exit);
		//////		set_EXIT7_PROBABILITY(&dont_exit);
		//////		set_EXIT8_PROBABILITY(&dont_exit);
		//////		set_EXIT9_PROBABILITY(&dont_exit);
		//////		set_EXIT10_PROBABILITY(&dont_exit);

		//////	}
		//////	else
		//////		if (emergency_exit_number == 2)
		//////		{
		//////			set_EXIT2_PROBABILITY(&do_exit);

		//////			set_EXIT1_PROBABILITY(&dont_exit);
		//////			set_EXIT3_PROBABILITY(&dont_exit);
		//////			set_EXIT4_PROBABILITY(&dont_exit);
		//////			set_EXIT5_PROBABILITY(&dont_exit);
		//////			set_EXIT6_PROBABILITY(&dont_exit);
		//////			set_EXIT7_PROBABILITY(&dont_exit);
		//////			set_EXIT8_PROBABILITY(&dont_exit);
		//////			set_EXIT9_PROBABILITY(&dont_exit);
		//////			set_EXIT10_PROBABILITY(&dont_exit);

		//////		}
		//////		else
		//////			if (emergency_exit_number == 3)
		//////			{
		//////				set_EXIT3_PROBABILITY(&do_exit);

		//////				set_EXIT1_PROBABILITY(&dont_exit);
		//////				set_EXIT2_PROBABILITY(&dont_exit);
		//////				set_EXIT4_PROBABILITY(&dont_exit);
		//////				set_EXIT5_PROBABILITY(&dont_exit);
		//////				set_EXIT6_PROBABILITY(&dont_exit);
		//////				set_EXIT7_PROBABILITY(&dont_exit);
		//////				set_EXIT8_PROBABILITY(&dont_exit);
		//////				set_EXIT9_PROBABILITY(&dont_exit);
		//////				set_EXIT10_PROBABILITY(&dont_exit);

		//////			}
		//////			else
		//////				if (emergency_exit_number == 4)
		//////				{
		//////					set_EXIT4_PROBABILITY(&do_exit);

		//////					set_EXIT1_PROBABILITY(&dont_exit);
		//////					set_EXIT2_PROBABILITY(&dont_exit);
		//////					set_EXIT3_PROBABILITY(&dont_exit);
		//////					set_EXIT5_PROBABILITY(&dont_exit);
		//////					set_EXIT6_PROBABILITY(&dont_exit);
		//////					set_EXIT7_PROBABILITY(&dont_exit);
		//////					set_EXIT8_PROBABILITY(&dont_exit);
		//////					set_EXIT9_PROBABILITY(&dont_exit);
		//////					set_EXIT10_PROBABILITY(&dont_exit);

		//////				}
		//////				else
		//////					if (emergency_exit_number == 5)
		//////					{
		//////						set_EXIT5_PROBABILITY(&do_exit);

		//////						set_EXIT1_PROBABILITY(&dont_exit);
		//////						set_EXIT2_PROBABILITY(&dont_exit);
		//////						set_EXIT3_PROBABILITY(&dont_exit);
		//////						set_EXIT4_PROBABILITY(&dont_exit);
		//////						set_EXIT6_PROBABILITY(&dont_exit);
		//////						set_EXIT7_PROBABILITY(&dont_exit);
		//////						set_EXIT8_PROBABILITY(&dont_exit);
		//////						set_EXIT9_PROBABILITY(&dont_exit);
		//////						set_EXIT10_PROBABILITY(&dont_exit);

		//////					}
		//////					else
		//////						if (emergency_exit_number == 6)
		//////						{
		//////							set_EXIT6_PROBABILITY(&do_exit);

		//////							set_EXIT1_PROBABILITY(&dont_exit);
		//////							set_EXIT2_PROBABILITY(&dont_exit);
		//////							set_EXIT3_PROBABILITY(&dont_exit);
		//////							set_EXIT4_PROBABILITY(&dont_exit);
		//////							set_EXIT5_PROBABILITY(&dont_exit);
		//////							set_EXIT7_PROBABILITY(&dont_exit);
		//////							set_EXIT8_PROBABILITY(&dont_exit);
		//////							set_EXIT9_PROBABILITY(&dont_exit);
		//////							set_EXIT10_PROBABILITY(&dont_exit);

		//////						}
		//////						else
		//////							if (emergency_exit_number == 7)
		//////							{
		//////								set_EXIT7_PROBABILITY(&do_exit);

		//////								set_EXIT1_PROBABILITY(&dont_exit);
		//////								set_EXIT2_PROBABILITY(&dont_exit);
		//////								set_EXIT3_PROBABILITY(&dont_exit);
		//////								set_EXIT4_PROBABILITY(&dont_exit);
		//////								set_EXIT5_PROBABILITY(&dont_exit);
		//////								set_EXIT6_PROBABILITY(&dont_exit);
		//////								set_EXIT8_PROBABILITY(&dont_exit);
		//////								set_EXIT9_PROBABILITY(&dont_exit);
		//////								set_EXIT10_PROBABILITY(&dont_exit);

		//////							}
		//////							else
		//////								if (emergency_exit_number == 8)
		//////								{
		//////									set_EXIT8_PROBABILITY(&do_exit);

		//////									set_EXIT1_PROBABILITY(&dont_exit);
		//////									set_EXIT2_PROBABILITY(&dont_exit);
		//////									set_EXIT3_PROBABILITY(&dont_exit);
		//////									set_EXIT4_PROBABILITY(&dont_exit);
		//////									set_EXIT5_PROBABILITY(&dont_exit);
		//////									set_EXIT6_PROBABILITY(&dont_exit);
		//////									set_EXIT7_PROBABILITY(&dont_exit);
		//////									set_EXIT9_PROBABILITY(&dont_exit);
		//////									set_EXIT10_PROBABILITY(&dont_exit);

		//////								}
		//////								else
		//////									if (emergency_exit_number == 9)
		//////									{
		//////										set_EXIT9_PROBABILITY(&do_exit);

		//////										set_EXIT1_PROBABILITY(&dont_exit);
		//////										set_EXIT2_PROBABILITY(&dont_exit);
		//////										set_EXIT3_PROBABILITY(&dont_exit);
		//////										set_EXIT4_PROBABILITY(&dont_exit);
		//////										set_EXIT5_PROBABILITY(&dont_exit);
		//////										set_EXIT6_PROBABILITY(&dont_exit);
		//////										set_EXIT7_PROBABILITY(&dont_exit);
		//////										set_EXIT8_PROBABILITY(&dont_exit);
		//////										set_EXIT10_PROBABILITY(&dont_exit);

		//////									}
		//////									else
		//////										if (emergency_exit_number == 10)
		//////										{
		//////											set_EXIT10_PROBABILITY(&do_exit);

		//////											set_EXIT1_PROBABILITY(&dont_exit);
		//////											set_EXIT2_PROBABILITY(&dont_exit);
		//////											set_EXIT3_PROBABILITY(&dont_exit);
		//////											set_EXIT4_PROBABILITY(&dont_exit);
		//////											set_EXIT5_PROBABILITY(&dont_exit);
		//////											set_EXIT6_PROBABILITY(&dont_exit);
		//////											set_EXIT7_PROBABILITY(&dont_exit);
		//////											set_EXIT8_PROBABILITY(&dont_exit);
		//////											set_EXIT9_PROBABILITY(&dont_exit);

		//////										}

		//////}
	//}
	

	// fill_cap is the required no. of sandbags to fill one navmap agent (one unit)
	int fill_cap = *get_fill_cap();
	double dxl = *get_DXL();
	double dyl = *get_DYL(); // used in outputting commands
	float extended_length = *get_extended_length();
	// the number of sandbags put in each layer
	int max_sandbags = max_navmap_static_sandbag_capacity_variable();
	
	extended_length = (max_sandbags / fill_cap) * dxl;
	set_extended_length(&extended_length);

	// load the number of applied layers
	int sandbag_layers = *get_sandbag_layers(); // shows initial layer 
	
	// to re-scale the length of proposed dike to fit the resolution for further comparison to the extended length
	float dike_length = *get_dike_length();
	int integer = dike_length / dxl;
	float decimal = dike_length / dxl;
	float res = decimal - integer;
	float dike_length_rescaled = (dike_length / dxl) - res;
	float rescaled_dike_length = dike_length_rescaled * dxl;

	// the number of sandbags, required in total to build the barrier with the proposed height (with respect to the size of the mesh/resplution)
	//int required_sandbags = dike_length_rescaled * fill_cap ;


	int update_stopper = *get_update_stopper();
	int stop_on = ON;
	int stop_off = OFF;

	// increment the number of layers once the previous layer is deployed, 
	//		the stopper updates only once it reaches to the length of dike and prevents keeping on incrementing 
	//		for the navmap being updated by more sandbags up to the fill_dke
	if (extended_length == rescaled_dike_length && update_stopper == OFF)
	{
		sandbag_layers++; 

		set_update_stopper(&stop_on);

	}
	else if (extended_length < rescaled_dike_length) // restarting the value of stopper for subsequent iteration to meet the requirement of first conditional statement
	{
		set_update_stopper(&stop_off);
	}

	// update the global constant variable in the environment
	set_sandbag_layers(&sandbag_layers);



	// total number of put sandbags, this variable is updated once the layer is finished.
	int total_sandbags = (sandbag_layers - 1) * (fill_cap * (rescaled_dike_length / dxl));// +max_sandbags;

	// This variable shows how many layers is completed
	// layers completed so far, not possible since in each iteration that reaches to the length of the dike the capacity will be restarted
	// (rescaled_dike_length / dxl) is the number of filled navmap up to the extended length of sandbag area
	// and [ (rescaled_dike_length / dxl) * fill_cap ] is the total number of navmaps to reach to the proposed dike length
	//(total_sandbags - fill_cap) is because at the moment there are two exit navmaps so, in total there is a shortage of one block, 
	// meaning that, the last navmap-drop_point wont be filled with enought sandbag
	int layers_applied = rintf((total_sandbags - fill_cap) / ((rescaled_dike_length / dxl) * fill_cap));
	
	float sandbag_height = *get_sandbag_height();
	float dike_height	= *get_dike_height();

	if ((layers_applied * sandbag_height) >= dike_height)
	{
		set_sandbagging_on(&stop_off);
	}

	// loading variables on host
	double HR_max		= *get_HR();
	int max_dead		= *get_max_dead();
	int max_alive_wet	= *get_max_alive_wet();
	int max_alive_walk	= *get_max_alive_walk();
	int max_alive_stuck	= *get_max_alive_stuck();
	double max_velocity = *get_max_velocity();
	double max_depth	= *get_max_depth();

	// storing the maximum number of people with different states in whole the simulation
	if (HR > HR_max)
		set_HR(&HR);
	if (count_trapped > max_dead)
		set_max_dead(&count_trapped);
	if (count_wetted > max_alive_wet)
		set_max_alive_wet(&count_wetted);
	if (count_alive_disrupted1 > max_alive_walk)
		set_max_alive_walk(&count_alive_disrupted1);
	if (count_alive_disrupted2 > max_alive_stuck)
		set_max_alive_stuck(&count_alive_disrupted2);
	if (flow_h_max > max_depth)
		set_max_depth(&flow_h_max);
	if (flow_velocity_max > max_velocity)
		set_max_velocity(&flow_velocity_max);


	
	///////////////////////////////////////////////GENERAL OUTPUT IN COMMAND WINDOWS , uncomment any required /////////////////////////////////////////////////////////////////

	//printing the simulation time
	float start_time	= *get_inflow_start_time();
	float peak_time		= *get_inflow_peak_time();
	float end_time		= *get_inflow_end_time();


	printf("\n\n\n**************************************************************************************\n");
	printf("**************************************************************************************\n");
	printf("\nElapsed simulation time = %.3f seconds\tOR\t%.2f minutes\tOR\t%.2f Hours \n", new_sim_time, new_sim_time/60, new_sim_time/3600); //prints simulation time 
	//printing the total number of pedestrians in each iteration
	printf("\n****************************** Pedestrian information *******************************\n");
	printf("Total number of pedestrians = %d \n", no_pedestrians);
	//printing the number of hero pedestrians in each iteration
	//printf("Total number of emergency responders = %d \n", count_heros);
	////printf("\n****************************** States of pedestrians *******************************\n");
	//////printing the number of pedestrians in dry zones in each iteration
	//printf("\nTotal number of pedestrians in dry areas = %d \n", count_in_dry);
	//////printing the number of alive pedestrians in wet area running to safe haven in each iteration
	//printf("\nTotal number of pedestrians in wet areas (safe) = %d \n", count_wetted);
	//////printing the number of dead pedestrians in wet area walking to the safe haven in each iteration
	////printf("\nTotal number of pedestrians in wet areas (slightly disrupted) = %d \n", count_alive_disrupted1);
	////////printing the number of dead pedestrians got stuck into water in each iteration
	////printf("\nTotal number of pedestrians in wet areas (severely disrupted)	= %d \n", count_alive_disrupted2);
	//printf("\nTotal number of pedestrians in wet areas (disrupted)	= %d \n", count_alive_disrupted1 + count_alive_disrupted1);
	////printing the number of pedestrians trapped in floodwater in each iteration
	//printf("\nTotal number of trapped pedestrians = %d \n", count_trapped);

	////printing the number of dead pedestrians got stuck into water in each iteration
	//printf("\nMaximum number of pedestrians in safe wet areas = %d \n", max_alive_wet);
	//printf("\nMaximum number of pedestrians slightly disrupted in wet areas = %d \n", max_alive_walk);
	//printf("\nMaximum number of pedestrians severely disrupted in wet areas	= %d \n", max_alive_stuck);
	//printf("\nMaximum number of pedestrians trapped so far	  = %d \n", max_dead);

	//printing the maximum height of water
	printf("\n****************************** Floodwater information *******************************\n");
	printf("\nFlood severity category = %d \n", abs(emergency_alarm-1));
	printf("\nMaximum Hazard Rating (HR) = %.3f \n", HR);
	printf("\nMaximum reached HR so far  = %.3f \n", HR_max);

	printf("\nmaximum depth of water = %.3f  m\n", flow_h_max);
	//printf("\nmaximum discharge of water = %.3f  m2/s \n", flow_qxy_max);
	printf("\nmaximum velocity of water = %.3f  m/s \n", flow_velocity_max);

	//printf("\nmaximum reached depth of water = %.3f  m \n", max_depth);
	//printf("\nmaximum reached velocity of water  = %.3f  m/s \n", max_velocity);

	//printf("\n AVERAGE depth of water  = %.3f  m \n", ave_depth);
	//printf("\n AVERAGE velocity of water  = %f  m \n", ave_velxy);
	////printf("\nAVERAGE velocity-x of water = %.3f  m/s \n", depth_ave);

	////// commented because there is no use of it in flume models
	////printf("\n****************************** Sandbagging information *******************************\n");
	////printf("\nTotal number of sandbags required to build 1-layer high barrier = %d \n", required_sandbags);
	////printf("\nMaximum number of deployed sandbags = %d \n", max_sandbags);
	////printf("\nTotal number of deployed sandbags = %d \n", total_sandbags);
	////printf("\nTotal number of deployed sandbag layers so far = %d \n", layers_applied);
	////printf("\nExtended length of sandbag barrier = %.3f \n", extended_length);
	////	
	//printf("\n the time sandbag layer is  deployed =%.3f \n", completion_time); // no need for now MS04032019 14:01
	

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	////printf("\n the number of put sandbag layers is =  %d \n", sandbag_layers);
	///////////////////////////////////////// Printing the info of flood and pedestrian numbers to a file ///////////////////////////////
	// Outputting the information to an appendable file which is updated in each iteration by adding data to the end of document
	FILE * fp = fopen("iterations/output.txt", "a");
	//
	if (fp == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	fprintf(fp, "%.3f\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%.3f\t\t%.3f\t\t%.3f\t\t%.3f\t\t%d\t\t%d\t\t%d\t\t%d\t\t\n", new_sim_time, no_pedestrians, count_in_dry, count_wetted, max_alive_walk, count_alive_disrupted2,count_trapped,HR, flow_h_max, flow_velocity_max, HR_max, max_dead, max_alive_wet, max_alive_walk, max_alive_stuck);

	fclose(fp);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
	///////////////////////////////////// OUTPUTTING FOR PROFILES ///////////////////////////////////////////////////
		//// for printing outputs in each iteration, it defines the name of each outputed file the same as simulation time e.g. 32.000.txt, which
		////	includes the outputted data at simulation time = 32 sec
	// Define when to output the data
	
	// using outputting time for some cases, otherwise the results will be outputted at the end of flooding events
	double outputting_time_manual = *get_outputting_time();
	
	double outputting_time_auto = *get_inflow_end_time();

	// loading outputting time intervals (defined by the user in input file (map.xml) )
	double outputting_time_interval = *get_outputting_time_interval();

	double outputting_time;

	if (outputting_time_manual != 0.0f)
	{
		outputting_time = outputting_time_manual;
	}
	else
	{
		outputting_time = outputting_time_auto;
	}

	double mid_start_time = start_time + 0.5f*(peak_time - start_time);
	double mid_peak_time  = peak_time  + 0.5f*(end_time  - peak_time );
	
	// rounding the simulation time for outputting purposes (see the second conditional statement below)
	double round_new_sim_time = round(new_sim_time);

	
	// this is to later check if the output interval time is reached or not
	double residual_time = fmod(round_new_sim_time, outputting_time_interval);

	// Outputting static results for the profile of water and the location of people with differen states
	if (round_new_sim_time <= outputting_time )
	{

	// outputting the results in multiple time intervals 
		if (   (round_new_sim_time == start_time)
			|| ( new_sim_time >= mid_start_time && new_sim_time < mid_start_time + old_dt)
			|| (round_new_sim_time == peak_time)
			|| (new_sim_time >= mid_peak_time && new_sim_time < mid_peak_time + old_dt)
			|| (round_new_sim_time == end_time)
			|| (round_new_sim_time == outputting_time)
			// outputting based on the temporal time interval defined by the user
			|| (residual_time == 0.000f) )

			{

			//// outputting the results for only one iteration after the end of output time / 
			//if (new_sim_time >= outputting_time && new_sim_time < outputting_time + old_dt)
			//{
				//	// Get some values and construct an output path.
				const char * directory = getOutputDir();
				unsigned int iteration = getIterationNumber();


				////	std::string outputFilename = std::string(std::string(directory) + "custom-output-" + std::to_string(iteration) + ".csv");
				std::string outputFilename = std::string(std::string(directory) + std::to_string(new_sim_time) + "flood" + ".csv");

				std::string outputFilename2 = std::string(std::string(directory) + std::to_string(new_sim_time) + "ped" + ".csv");

				FILE * fp2 = fopen(outputFilename.c_str(), "w");

				FILE * fp3 = fopen(outputFilename2.c_str(), "w");

				if (fp2 != nullptr || fp3 != nullptr)
				{

					// Output a header row for the CSV (flood analysis)
					fprintf(fp2, "x\t\ty\t\tDepth\t\t\tVelocity\t\t\tHR\n");

					// Output a header row for the CSV (pedestrian state analysis)
					fprintf(fp3, "x\t\ty\t\tstate\n");

					///////Uncomment if //////////// OUTPUTTING FLOOD DATA FOR SPATIO-TEMPORAL ANALYSIS (producing profile of water velocity and depth sliced in the middle of the domain in a given sim time 'outputting_time' ) /////////////////////
					for (int index = 0; index < no_FloodCells; index++)
					{
						// Loading water flow info from navmap cells
						double flow_h = get_FloodCell_Default_variable_h(index);

						int x = get_FloodCell_Default_variable_x(index);
						int y = get_FloodCell_Default_variable_y(index);

						double flow_discharge_x = get_FloodCell_Default_variable_qx(index);
						double flow_discharge_y = get_FloodCell_Default_variable_qy(index);


						//calculating the velocity of water
						double flow_velocity_x, flow_velocity_y;

						if (flow_h != 0)
						{
							flow_velocity_x = flow_discharge_x / flow_h;
							flow_velocity_y = flow_discharge_y / flow_h;
						}
						else
						{
							flow_velocity_x = 0;
							flow_velocity_y = 0;
						}

						double flow_velocity_xy = max(flow_velocity_x, flow_velocity_y);   // taking maximum discharge between both x and y direction

						// calculating local hazard rate
						double hazard_rate = flow_h*(flow_velocity_xy + 0.5);

						
						// NOTE: for outputing the cross sectional profile of water in the centre line of x-axis, 
						// uncomment the if condition statement
						// Otherwise, outputs of the entire flood grid
						//int middle_pos = sqrt(no_FloodCells) / 2;

						//if (x == middle_pos)
						//{
							fprintf(fp2, "%.3f\t\t%.3f\t\t%f\t\t%f\t\t%.3f\n", x*dxl, y*dyl, flow_h, flow_velocity_xy, hazard_rate);
						// }
					}
					/////////////////////////////////////////////////////////////////////////



				//	//////////////////////////////// Outputting the location of dead/trapped pedestrian /////////////////////////////////////////
					for (int index = 0; index < no_pedestrians; index++)
					{
						int pedestrians_state = get_agent_default_variable_HR_state(index);
						float x = get_agent_default_variable_x(index);
						float y = get_agent_default_variable_y(index);
						double xmax = *get_xmax();
						double ymax = *get_ymax();

						double HR_ped = get_agent_default_variable_HR(index);

						//		// counting the number of pedestrians with different states

						// uncomment below if statement to limit the outputs for peds with specific status
						//if (pedestrians_state == STATE_DEAD || pedestrians_state == STATE_ALIVE_STILL || pedestrians_state == STATE_ALIVE_WALK)
						//{
							// print the global position of pedestians
							fprintf(fp3, "%.3f\t\t%.3f\t\t%d\t\t%.3f\n", (x - 1)*(0.5*xmax) + xmax, (y - 1)*(0.5*ymax) + ymax, pedestrians_state, HR_ped);
						//}
					}
					/////////////////////////////////////////////////////////////////////////

				}
				else
				{
					fprintf(stderr, "Error: file %s could not be created for customOutputStepFunction\n", outputFilename.c_str());

					fprintf(stderr, "Error: file %s could not be created for customOutputStepFunction\n", outputFilename2.c_str());
				}

				if (fp2 != nullptr && fp2 != stdout && fp2 != stderr) {
					fclose(fp2);
					fp2 = nullptr;
				}

				if (fp3 != nullptr && fp3 != stdout && fp3 != stderr) {
					fclose(fp3);
					fp3 = nullptr;
				}
			} // simulation timing second if statement end
		} // simulation timing first if statement end

}


inline __device__ double3 hll_x(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R);
inline __device__ double3 hll_y(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R);
inline __device__ double3 F_SWE(double hh, double qx, double qy);
inline __device__ double3 G_SWE(double hh, double qx, double qy);
inline __device__ double inflow(double time);

enum ECellDirection { NORTH = 1, EAST = 2, SOUTH = 3, WEST = 4 };

struct __align__(16) AgentFlowData
{
	double z0;
	double h;
	double et;
	double qx;
	double qy;
};


struct __align__(16) LFVResult
{
	// Commented by MS12DEC2017 12:24pm
	/*__device__ LFVResult(double _h_face, double _et_face, double2 _qFace)
	{
	h_face = _h_face;
	et_face = _et_face;
	qFace = _qFace;
	}
	__device__ LFVResult()
	{
	h_face = 0.0;
	et_face = 0.0;
	qFace = make_double2(0.0, 0.0);
	}*/

	double  h_face;
	double  et_face;
	double2 qFace;

};


inline __device__ AgentFlowData GetFlowDataFromAgent(xmachine_memory_FloodCell* agent)
{
	// This function loads the data from flood agents
	// helpful in hydrodynamically modification where the original data is expected to remain intact


	AgentFlowData result;

	result.z0 = agent->z0;
	result.h = agent->h;
	result.et = agent->z0 + agent->h; // added by MS27Sep2017
	result.qx = agent->qx;
	result.qy = agent->qy;

	return result;

}


//// Boundary condition in ghost cells // not activated in this model (MS commented)
//inline __device__ void centbound(xmachine_memory_FloodCell* agent, const AgentFlowData& FlowData, AgentFlowData& centBoundData)
//{
//	// NB. It is assumed that the ghost cell is at the same level as the present cell.
//	//** Assign the the same topography data for the ghost cell **!
//	
//	// NOTE: this function is not applicable in current version of the floodPedestrian model "MS comments"
//
//	//Default is a reflective boundary
//	centBoundData.z0 = FlowData.z0;
//
//	centBoundData.h = FlowData.h;
//
//	centBoundData.et = FlowData.et; // added by MS27Sep2017 // 
//
//	centBoundData.qx = -FlowData.qx; //
//
//	centBoundData.qy = -FlowData.qy; //
//
//}


// This function should be called when minh_loc is greater than TOL_H
inline __device__ double2 friction_2D(double dt_loc, double h_loc, double qx_loc, double qy_loc)
{
	//This function takes the friction term into account for wet agents


	double2 result;

	if (h_loc > TOL_H)
	{

		// Local velocities    
		double u_loc = qx_loc / h_loc;
		double v_loc = qy_loc / h_loc;

		// Friction forces are incative as the flow is motionless.
		if ((fabs(u_loc) <= emsmall)
			&& (fabs(v_loc) <= emsmall)
			)
		{
			result.x = qx_loc;
			result.y = qy_loc;
		}
		else
		{
			// The is motional. The FRICTIONS CONTRUBUTION HAS TO BE ADDED SO THAT IT DOESN'T REVERSE THE FLOW.

			double Cf = GRAVITY * pow(GLOBAL_MANNING, 2.0) / pow(h_loc, 1.0 / 3.0);

			double expULoc = pow(u_loc, 2.0);
			double expVLoc = pow(v_loc, 2.0);

			double Sfx = -Cf * u_loc * sqrt(expULoc + expVLoc);
			double Sfy = -Cf * v_loc * sqrt(expULoc + expVLoc);

			double DDx = 1.0 + dt_loc * (Cf / h_loc * (2.0 * expULoc + expVLoc) / sqrt(expULoc + expVLoc));
			double DDy = 1.0 + dt_loc * (Cf / h_loc * (expULoc + 2.0 * expVLoc) / sqrt(expULoc + expVLoc));

			result.x = qx_loc + (dt_loc * (Sfx / DDx));
			result.y = qy_loc + (dt_loc * (Sfy / DDy));

		}
	}
	else
	{
		result.x = 0.0;
		result.y = 0.0;
	}

	return result;
}


inline __device__ void Friction_Implicit(xmachine_memory_FloodCell* agent, double dt)
{
	//double dt = agent->timeStep;

	if (GLOBAL_MANNING > 0.0)
	{
		AgentFlowData FlowData = GetFlowDataFromAgent(agent);

		if (FlowData.h <= TOL_H)
		{
			return;
		}

		double2 frict_Q = friction_2D(dt, FlowData.h, FlowData.qx, FlowData.qy);

		agent->qx = frict_Q.x;
		agent->qy = frict_Q.y;

	}

}


__FLAME_GPU_FUNC__ int PrepareWetDry(xmachine_memory_FloodCell* agent, xmachine_message_WetDryMessage_list* WerDryMessage_messages)
{
	// This function broadcasts the information of wet flood agents and takes the dry flood agents out of the domain


	if (agent->inDomain)
	{

		AgentFlowData FlowData = GetFlowDataFromAgent(agent);

		double hp = FlowData.h; // = agent->h ; MS02OCT2017


		agent->minh_loc = hp;


		add_WetDryMessage_message<DISCRETE_2D>(WerDryMessage_messages, 1, agent->x, agent->y, agent->minh_loc);
	}
	else
	{
		add_WetDryMessage_message<DISCRETE_2D>(WerDryMessage_messages, 0, agent->x, agent->y, BIG_NUMBER);
	}

	return 0;

}

__FLAME_GPU_FUNC__ int ProcessWetDryMessage(xmachine_memory_FloodCell* agent, xmachine_message_WetDryMessage_list* WetDryMessage_messages)
{

	// This function loads the information of neighbouring flood agents and by taking the maximum height of water ane decide when to 
	// apply the friction term by considering a pre-defined threshold TOL_H

	if (agent->inDomain)
	{

		//looking up neighbours values for wet/dry tracking
		xmachine_message_WetDryMessage* msg = get_first_WetDryMessage_message<DISCRETE_2D>(WetDryMessage_messages, agent->x, agent->y);

		double maxHeight = agent->minh_loc;

		// MS COMMENTS : WHICH HEIGH OF NEIGHBOUR IS BEING CHECKED HERE? ONE AGAINST OTHER AGENTS ? POSSIBLE ?
		while (msg)
		{
			if (msg->inDomain)
			{
				agent->minh_loc = min(agent->minh_loc, msg->min_hloc);
			}

			if (msg->min_hloc > maxHeight)
			{
				maxHeight = msg->min_hloc;
			}

			msg = get_next_WetDryMessage_message<DISCRETE_2D>(msg, WetDryMessage_messages);
		}



		maxHeight = agent->minh_loc;


		if (maxHeight > TOL_H) // if the Height of water is not less than TOL_H => the friction is needed to be taken into account MS05Sep2017
		{

			//	//Friction term has been disabled for radial deam-break
			Friction_Implicit(agent, dt); //

		}
		else
		{

			//	//need to go high, so that it won't affect min calculation when it is tested again . Needed to be tested MS05Sep2017 which is now temporary. needs to be corrected somehow
			agent->minh_loc = BIG_NUMBER;

		}


	}

	//printf("qx = %f \t qy = %f \n", agent->qx, agent->qy);

	return 0;
}


inline __device__ LFVResult LFV(const AgentFlowData& FlowData)
{
	// assigning value to local face variables (taken identical to original value)
	// lcoal face variables are designed to prevent race condition where more than one
	// agent is using a mutual data (accessing the same data location on memory at the same time)

	LFVResult result;


	result.h_face = FlowData.h;

	result.et_face = FlowData.et;

	result.qFace.x = FlowData.qx;

	result.qFace.y = FlowData.qy;

	return result;
}

__inline __device__ double2 FindGlobalPosition(xmachine_memory_FloodCell* agent, double2 offset)
{
	// This function finds the global location of any discrete partitioned agent (flood/navmap) 
	// with respect to the defined dimension of the domain (xmin,xmax, yamin, yamax)

	double x = (agent->x * DXL) + offset.x;
	double y = (agent->y * DYL) + offset.y;

	return make_double2(x, y);
}

//__inline __device__ float2 FindGlobalPosition_navmap(xmachine_memory_navmap* agent, double2 offset)
//{
//	//find global position in actual domain
//	float x = (agent->x * DXL) + offset.x;
//	float y = (agent->y * DYL) + offset.y;
//
//	return make_float2(x, y);
//}

__inline __device__ float2 FindRescaledGlobalPosition_navmap(xmachine_memory_navmap* agent, double2 offset)
{
	// since the location of pedestrian is bounded between -1 to 1, to load appropriate data from
	// correct location of pedestrians (performed by navmap agents), the location of navmap is 
	// rescaled to fit [-1 1] so as to make their scales identical

	//find global position in actual domain
	float x = (agent->x * DXL) + offset.x;
	float y = (agent->y * DYL) + offset.y;

	// to bound the domain within [-1 1]
	// zero point in [-1 1]
	int half_x = 0.5*xmax ;
	int half_y = 0.5*ymax ;
	// the location in [-1 1]
	float x_rescaled = (( x / half_x) - 1);
	float y_rescaled = (( y / half_y) - 1);

	return make_float2(x_rescaled, y_rescaled);
}

//__inline __device__ float walking_speed(xmachine_memory_agent* agent)
//{
//	float walking_speed;
//
//	float M = ( powf(agent->v_water, 2.0)*agent->d_water) / GRAVITY + (0.5 * powf(agent->d_water, 2.0));
//
//	walking_speed = 0.53 * powf(M, -0.19); 
//
//	return walking_speed;
//}

__FLAME_GPU_FUNC__ int PrepareSpaceOperator(xmachine_memory_FloodCell* agent, xmachine_message_SpaceOperatorMessage_list* SpaceOperatorMessage_messages)
{
	// This function is to broadcase local face variables of flood agents to their neighbours
	// Also initial depth of water to support incoming discharge is considered in this function

	// 
	//// finding the location of flood agents in global scale (May need to find a better way to find boundary agents)
	//// EAST
	double2 face_location_N = FindGlobalPosition(agent, make_double2(0.0, DYL*0.5));
	double2 face_location_E = FindGlobalPosition(agent, make_double2(DXL*0.5, 0.0));
	double2 face_location_S = FindGlobalPosition(agent, make_double2(0.0, -DYL*0.5));
	double2 face_location_W = FindGlobalPosition(agent, make_double2(-DXL*0.5, 0.0));

	double difference_E = fabs(face_location_E.x - xmax);
	double difference_W = fabs(face_location_W.x - xmin);
	double difference_N = fabs(face_location_N.y - ymax);
	double difference_S = fabs(face_location_S.y - ymin);
	//

	// Here provides a rovide a 0.01m water depth to support the discharge in an original dry flood agent at active inflow boundary 
	if ((INFLOW_BOUNDARY == NORTH) && (sim_time >= inflow_start_time) && (sim_time <= inflow_end_time)) // flood end time prevents extra water depth when not needed
	{
		// agents located at the NORTH boundary north
		if (difference_N <  DYL) // '1' is the minimum difference between the edge of the last flood agent and the boundary agents' location
		{
			if ((face_location_W.x >= x1_boundary) && (face_location_E.x <= x2_boundary))
			{
				if (fabs(agent->h) < TOL_H)
				{
					agent->h = init_depth_boundary;
					agent->minh_loc = init_depth_boundary;
				}
			}


		}
	}
	else if ((INFLOW_BOUNDARY == EAST) && (sim_time >= inflow_start_time)) // provide water depth exactly before the when the flood starts
	{
		// agents located at the EAST boundary
		if (difference_E < DXL)
		{
			if ((face_location_S.y >= y1_boundary) && (face_location_N.y <= y2_boundary))
			{
				if (fabs(agent->h) < TOL_H)
				{
					agent->h = init_depth_boundary;
					agent->minh_loc = init_depth_boundary;
				}
			}
		}
	}
	else if ((INFLOW_BOUNDARY == SOUTH) && (sim_time >= inflow_start_time))
	{
		// agents located at the EAST boundary
		if (difference_S <  DYL)
		{
			if ((face_location_W.x >= x1_boundary) && (face_location_E.x <= x2_boundary))
			{
				if (fabs(agent->h) < TOL_H)
				{
					agent->h = init_depth_boundary;
					agent->minh_loc = init_depth_boundary;
				}
			}
		}
	}
	else if ((INFLOW_BOUNDARY == WEST) && (sim_time >= inflow_start_time))
	{
		// agents located at the EAST boundary
		if (difference_W <  DXL)
		{

			if ((face_location_S.y >= y1_boundary) && (face_location_N.y <= y2_boundary))
			{
				if (fabs(agent->h) < TOL_H)
				{
					agent->h = init_depth_boundary;
					agent->minh_loc = init_depth_boundary;
				}
			}
		}
	}


	AgentFlowData FlowData = GetFlowDataFromAgent(agent);

	LFVResult faceLFV = LFV(FlowData);

	//EAST FACE
	agent->etFace_E = faceLFV.et_face;
	agent->hFace_E = faceLFV.h_face;
	agent->qxFace_E = faceLFV.qFace.x;
	agent->qyFace_E = faceLFV.qFace.y;

	//WEST FACE
	agent->etFace_W = faceLFV.et_face;
	agent->hFace_W = faceLFV.h_face;
	agent->qxFace_W = faceLFV.qFace.x;
	agent->qyFace_W = faceLFV.qFace.y;

	//NORTH FACE
	agent->etFace_N = faceLFV.et_face;
	agent->hFace_N = faceLFV.h_face;
	agent->qxFace_N = faceLFV.qFace.x;
	agent->qyFace_N = faceLFV.qFace.y;

	//SOUTH FACE
	agent->etFace_S = faceLFV.et_face;
	agent->hFace_S = faceLFV.h_face;
	agent->qxFace_S = faceLFV.qFace.x;
	agent->qyFace_S = faceLFV.qFace.y;


	if (agent->inDomain
		&& agent->minh_loc > TOL_H)
	{
		//broadcast internal LFV values to surrounding cells
		add_SpaceOperatorMessage_message<DISCRETE_2D>(SpaceOperatorMessage_messages,
			1,
			agent->x, agent->y,
			agent->hFace_E, agent->etFace_E, agent->qxFace_E, agent->qyFace_E,
			agent->hFace_W, agent->etFace_W, agent->qxFace_W, agent->qyFace_W,
			agent->hFace_N, agent->etFace_N, agent->qxFace_N, agent->qyFace_N,
			agent->hFace_S, agent->etFace_S, agent->qxFace_S, agent->qyFace_S
			);
	}
	else
	{
		//broadcast internal LFV values to surrounding cells
		add_SpaceOperatorMessage_message<DISCRETE_2D>(SpaceOperatorMessage_messages,
			0,
			agent->x, agent->y,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0
			);
	}

	return 0;
}

inline __device__ void WD(double h_L,
	double h_R,
	double et_L,
	double et_R,
	double qx_L,
	double qx_R,
	double qy_L,
	double qy_R,
	ECellDirection ndir,
	double& z_LR,
	double& h_L_star,
	double& h_R_star,
	double& qx_L_star,
	double& qx_R_star,
	double& qy_L_star,
	double& qy_R_star
)

{
	// This function provide a non-negative reconstruction of the Riemann-states.

	double z_L = et_L - h_L;
	double z_R = et_R - h_R;

	double u_L = 0.0;
	double v_L = 0.0;
	double u_R = 0.0;
	double v_R = 0.0;

	if (h_L <= TOL_H)
	{
		u_L = 0.0;
		v_L = 0.0;

	}
	else
	{
		u_L = qx_L / h_L;
		v_L = qy_L / h_L;
	}


	if (h_R <= TOL_H)
	{
		u_R = 0.0;
		v_R = 0.0;

	}
	else
	{
		u_R = qx_R / h_R;
		v_R = qy_R / h_R;
	}

	z_LR = max(z_L, z_R);

	double delta;

	switch (ndir)
	{
	case NORTH:
	case EAST:
	{
		delta = max(0.0, -(et_L - z_LR));
	}
	break;

	case WEST:
	case SOUTH:
	{
		delta = max(0.0, -(et_R - z_LR));
	}
	break;

	}

	h_L_star = max(0.0, et_L - z_LR);
	double et_L_star = h_L_star + z_LR;
	qx_L_star = h_L_star * u_L;
	qy_L_star = h_L_star * v_L;

	h_R_star = max(0.0, et_R - z_LR);
	double et_R_star = h_R_star + z_LR;
	qx_R_star = h_R_star * u_R;
	qy_R_star = h_R_star * v_R;

	if (delta > 0.0)
	{
		z_LR = z_LR - delta;
		et_L_star = et_L_star - delta;
		et_R_star = et_R_star - delta;
	}
	//else
	//{
	//	z_LR = z_LR;
	//	et_L_star = et_L_star;
	//	et_R_star = et_R_star;
	//}


	h_L_star = et_L_star - z_LR;
	h_R_star = et_R_star - z_LR;


}


inline __device__ double3 hll_x(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R)
{
	// This function is to calculate numerical flux in x-axis direction
	double3 F_face = make_double3(0.0, 0.0, 0.0);

	double u_L = 0.0;
	double v_L = 0.0;
	double u_R = 0.0;
	double v_R = 0.0;

	if ((h_L <= TOL_H) && (h_R <= TOL_H))
	{
		F_face.x = 0.0;
		F_face.y = 0.0;
		F_face.z = 0.0;

		return F_face;
	}
	else
	{

		if (h_L <= TOL_H)
		{
			h_L = 0.0;
			u_L = 0.0;
			v_L = 0.0;
		}
		else
		{

			u_L = qx_L / h_L;
			v_L = qy_L / h_L;
		}


		if (h_R <= TOL_H)
		{
			h_R = 0.0;
			u_R = 0.0;
			v_R = 0.0;
		}
		else
		{
			u_R = qx_R / h_R;
			v_R = qy_R / h_R;
		}

		double a_L = sqrt(GRAVITY * h_L);
		double a_R = sqrt(GRAVITY * h_R);

		double h_star = pow(((a_L + a_R) / 2.0 + (u_L - u_R) / 4.0), 2) / GRAVITY;
		double u_star = (u_L + u_R) / 2.0 + a_L - a_R;
		double a_star = sqrt(GRAVITY * h_star);

		double s_L, s_R;

		if (h_L <= TOL_H)
		{
			s_L = u_R - (2.0 * a_R);
		}
		else
		{
			s_L = min(u_L - a_L, u_star - a_star);
		}



		if (h_R <= TOL_H)
		{
			s_R = u_L + (2.0 * a_L);
		}
		else
		{
			s_R = max(u_R + a_R, u_star + a_star);
		}

		double s_M = ((s_L * h_R * (u_R - s_R)) - (s_R * h_L * (u_L - s_L))) / (h_R * (u_R - s_R) - (h_L * (u_L - s_L)));

		double3 F_L, F_R;

		//FSWE3 F_L = F_SWE((double)h_L, (double)qx_L, (double)qy_L);
		F_L = F_SWE(h_L, qx_L, qy_L);

		//FSWE3 F_R = F_SWE((double)h_R, (double)qx_R, (double)qy_R);
		F_R = F_SWE(h_R, qx_R, qy_R);


		if (s_L >= 0.0)
		{
			F_face.x = F_L.x;
			F_face.y = F_L.y;
			F_face.z = F_L.z;

			//return F_L; // 
		}

		else if ((s_L < 0.0) && s_R >= 0.0)

		{

			double F1_M = ((s_R * F_L.x) - (s_L * F_R.x) + s_L * s_R * (h_R - h_L)) / (s_R - s_L);

			double F2_M = ((s_R * F_L.y) - (s_L * F_R.y) + s_L * s_R * (qx_R - qx_L)) / (s_R - s_L);

			//			
			if ((s_L < 0.0) && (s_M >= 0.0))
			{
				F_face.x = F1_M;
				F_face.y = F2_M;
				F_face.z = F1_M * v_L;
				//				
			}
			else if ((s_M < 0.0) && (s_R >= 0.0))
			{
				//				
				F_face.x = F1_M;
				F_face.y = F2_M;
				F_face.z = F1_M * v_R;
				//					
			}
		}

		else if (s_R < 0)
		{
			//			
			F_face.x = F_R.x;
			F_face.y = F_R.y;
			F_face.z = F_R.z;
			//	
			//return F_R; // 
		}

		return F_face;

	}
	//	

}

inline __device__ double3 hll_y(double h_S, double h_N, double qx_S, double qx_N, double qy_S, double qy_N)
{
	// This function is to calculate numerical flux in y-axis direction

	double3 G_face = make_double3(0.0, 0.0, 0.0);
	// This function calculates the interface fluxes in x-direction.
	double u_S = 0.0;
	double v_S = 0.0;
	double u_N = 0.0;
	double v_N = 0.0;

	if ((h_S <= TOL_H) && (h_N <= TOL_H))
	{
		G_face.x = 0.0;
		G_face.y = 0.0;
		G_face.z = 0.0;

		return G_face;
	}
	else
	{

		if (h_S <= TOL_H)
		{
			h_S = 0.0;
			u_S = 0.0;
			v_S = 0.0;
		}
		else
		{

			u_S = qx_S / h_S;
			v_S = qy_S / h_S;
		}


		if (h_N <= TOL_H)
		{
			h_N = 0.0;
			u_N = 0.0;
			v_N = 0.0;
		}
		else
		{
			u_N = qx_N / h_N;
			v_N = qy_N / h_N;
		}

		double a_S = sqrt(GRAVITY * h_S);
		double a_N = sqrt(GRAVITY * h_N);

		double h_star = pow(((a_S + a_N) / 2.0 + (v_S - v_N) / 4.0), 2.0) / GRAVITY;
		double v_star = (v_S + v_N) / 2.0 + a_S - a_N;
		double a_star = sqrt(GRAVITY * h_star);

		double s_S, s_N;

		if (h_S <= TOL_H)
		{
			s_S = v_N - (2.0 * a_N);
		}
		else
		{
			s_S = min(v_S - a_S, v_star - a_star);
		}



		if (h_N <= TOL_H)
		{
			s_N = v_S + (2.0 * a_S);
		}
		else
		{
			s_N = max(v_N + a_N, v_star + a_star);
		}

		double s_M = ((s_S * h_N * (v_N - s_N)) - (s_N * h_S * (v_S - s_S))) / (h_N * (v_N - s_N) - (h_S * (v_S - s_S)));


		double3 G_S, G_N;

		G_S = G_SWE(h_S, qx_S, qy_S);

		G_N = G_SWE(h_N, qx_N, qy_N);


		if (s_S >= 0.0)
		{
			G_face.x = G_S.x;
			G_face.y = G_S.y;
			G_face.z = G_S.z;

			//return G_S; //

		}

		else if ((s_S < 0.0) && (s_N >= 0.0))

		{

			double G1_M = ((s_N * G_S.x) - (s_S * G_N.x) + s_S * s_N * (h_N - h_S)) / (s_N - s_S);

			double G3_M = ((s_N * G_S.z) - (s_S * G_N.z) + s_S * s_N * (qy_N - qy_S)) / (s_N - s_S);
			//			
			if ((s_S < 0.0) && (s_M >= 0.0))
			{
				G_face.x = G1_M;
				G_face.y = G1_M * u_S;
				G_face.z = G3_M;
				//				
			}
			else if ((s_M < 0.0) && (s_N >= 0.0))
			{
				//				
				G_face.x = G1_M;
				G_face.y = G1_M * u_N;
				G_face.z = G3_M;
				//					
			}
		}

		else if (s_N < 0)
		{
			//			
			G_face.x = G_N.x;
			G_face.y = G_N.y;
			G_face.z = G_N.z;

			//return G_N; //
			//	
		}

		return G_face;

	}
	//	
}

__FLAME_GPU_FUNC__ int ProcessSpaceOperatorMessage(xmachine_memory_FloodCell* agent, xmachine_message_SpaceOperatorMessage_list* SpaceOperatorMessage_messages)
{
	// This function updates the state of flood agents by solving shallow water equations (SWEs) aiming Finite Volume (FV) method 
	// Boundary condition is considered within this function

	double3 FPlus = make_double3(0.0, 0.0, 0.0);
	double3 FMinus = make_double3(0.0, 0.0, 0.0);
	double3 GPlus = make_double3(0.0, 0.0, 0.0);
	double3 GMinus = make_double3(0.0, 0.0, 0.0);

	double2 face_location_N = FindGlobalPosition(agent, make_double2(0.0, DYL*0.5));
	double2 face_location_E = FindGlobalPosition(agent, make_double2(DXL*0.5, 0.0));
	double2 face_location_S = FindGlobalPosition(agent, make_double2(0.0, -DYL*0.5));
	double2 face_location_W = FindGlobalPosition(agent, make_double2(-DXL*0.5, 0.0));

	// Initialising EASTER face
	double zbF_E = agent->z0;
	double hf_E = 0.0;

	// Left- and right-side local face values of easter interface
	double h_L = agent->hFace_E;
	double et_L = agent->etFace_E;
	double qx_L = agent->qxFace_E; // 
	double qy_L = agent->qyFace_E; //

	//Initialisation of left- and right-side values of the interfaces for Riemann solver
	double h_R = h_L;
	double et_R = et_L;
	double qx_R;
	double qy_R;

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Applying boundary condition at EAST boundary
	if (BOUNDARY_EAST_STATUS == 1) // 1:Transmissive
	{
		if (INFLOW_BOUNDARY == EAST) // 1:NORTH 2:EAST 3:SOUTH 4:WEST // meaning boundary has inflow
		{
			if ((face_location_S.y >= y1_boundary) && (face_location_N.y <= y2_boundary)) // defining the inflow position
			{
				qx_R = -inflow(sim_time);//-0.2; // inflow data (minus reverse the flow -> is a must)
				qy_R = qy_L;
			}
			else
			{
				qx_R = qx_L;
				qy_R = qy_L;
			}
		}
		else
		{
			qx_R = qx_L;
			qy_R = qy_L;
		}
	}
	else if (BOUNDARY_EAST_STATUS == 2)  // 2:reflective
	{
		if (INFLOW_BOUNDARY == EAST) // 1:NORTH 2:EAST 3:SOUTH 4:WEST // meaning boundary has inflow
		{
			if ((face_location_S.y >= y1_boundary) && (face_location_N.y <= y2_boundary)) // defining the inflow position
			{
				qx_R = -inflow(sim_time);//-0.2; // inflow data (minus reverse the flow -> is a must)
				qy_R = qy_L;
			}
			else
			{
				qx_R = -qx_L;
				qy_R = qy_L;
			}
		}
		else
		{
			qx_R = -qx_L;
			qy_R = qy_L;
		}

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////

	double z_F = 0.0;
	double h_F_L = 0.0;
	double h_F_R = 0.0;
	double qx_F_L = 0.0;
	double qx_F_R = 0.0;
	double qy_F_L = 0.0;
	double qy_F_R = 0.0;

	//Wetting and drying "depth-positivity-preserving" reconstructions
	//WD(h_L, h_R, et_L, et_R, q_L.x, q_R.x, q_L.y, q_R.y, EAST, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, EAST, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);


	// Flux accross the cell
	FPlus = hll_x(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_E = z_F;
	hf_E = h_F_L;


	// Initialising WESTERN face
	double zbF_W = agent->z0;
	double hf_W = 0.0;
	//double qxf_W = 0.0;
	//double qyf_W = 0.0;

	z_F = 0.0;
	h_F_L = 0.0;
	h_F_R = 0.0;
	qx_F_L = 0.0;
	qx_F_R = 0.0;
	qy_F_L = 0.0;
	qy_F_R = 0.0;

	h_R = agent->hFace_W;
	et_R = agent->etFace_W;

	qx_R = agent->qxFace_W;
	qy_R = agent->qyFace_W;



	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Applying boundary condition at WEST boundary

	h_L = h_R;
	et_L = et_R;

	if (BOUNDARY_WEST_STATUS == 1) // 1:Transmissive
	{
		if (INFLOW_BOUNDARY == WEST) // 1:NORTH  2:EAST  3:SOUTH  4:WEST // meaning boundary has inflow
		{
			if ((face_location_S.y >= y1_boundary) && (face_location_N.y <= y2_boundary)) // defining the inflow position
			{
				qx_L = inflow(sim_time); // inflow data
				qy_L = qy_R;
			}
			else
			{
				qx_L = qx_R;
				qy_L = qy_R;
			}
		}
		else
		{
			qx_L = qx_R;
			qy_L = qy_R;
		}

	}
	else if (BOUNDARY_WEST_STATUS == 2)  // 2:reflective
	{

		if (INFLOW_BOUNDARY == WEST) // 1:NORTH  2:EAST  3:SOUTH  4:WEST // meaning boundary has inflow
		{
			if ((face_location_S.y >= y1_boundary) && (face_location_N.y <= y2_boundary)) // defining the inflow position
			{
				qx_L = inflow(sim_time); // inflow data
				qy_L = qy_R;
			}
			else
			{
				qx_L = -qx_R;
				qy_L = qy_R;
			}
		}
		else
		{
			qx_L = -qx_R;
			qy_L = qy_R;
		}
	}
		
	/////////////////////////////////////////////////////////////////////////////////////////////////////


	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, WEST, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	FMinus = hll_x(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_W = z_F;
	hf_W = h_F_R;
	//qxf_W = qx_F_R;
	//qyf_W = qy_F_R;


	// Initialising NORTHERN face

	double zbF_N = agent->z0;
	double hf_N = 0.0;
	//double qxf_N = 0.0;
	//double qyf_N = 0.0;

	z_F = 0.0;
	h_F_L = 0.0;
	h_F_R = 0.0;
	qx_F_L = 0.0;
	qx_F_R = 0.0;
	qy_F_L = 0.0;
	qy_F_R = 0.0;

	h_L = agent->hFace_N;
	et_L = agent->etFace_N;

	qx_L = agent->qxFace_N;
	qy_L = agent->qyFace_N;


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Applying boundary condition at NORTH boundary

	h_R = h_L;
	et_R = et_L;

	if (BOUNDARY_NORTH_STATUS == 1) // 1:Transmissive
	{
		if (INFLOW_BOUNDARY == NORTH) // 1:NORTH  2:EAST  3:SOUTH  4:WEST // meaning boundary has inflow
		{
			if ((face_location_W.x >= x1_boundary) && (face_location_E.x <= x2_boundary)) // defining the inflow position
			{
				qx_R = qx_L; // inflow data
				qy_R = -inflow(sim_time);
			}
			else
			{
				qx_R = qx_L;
				qy_R = qy_L;
			}
		}
		else
		{
			qx_R = qx_L;
			qy_R = qy_L;
		}
	}
	else if (BOUNDARY_NORTH_STATUS == 2)  // 2:reflective
	{
		if (INFLOW_BOUNDARY == NORTH) // 1:NORTH  2:EAST  3:SOUTH  4:WEST // meaning boundary has inflow
		{
			if ((face_location_W.x >= x1_boundary) && (face_location_E.x <= x2_boundary)) // defining the inflow position
			{
				qx_R = qx_L; // inflow data
				qy_R = -inflow(sim_time);
			}
			else
			{
				qx_R = qx_L;
				qy_R = -qy_L;
			}
		}
		else
		{
			qx_R = qx_L;
			qy_R = -qy_L;
		}

	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, NORTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	GPlus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_N = z_F;
	hf_N = h_F_L;



	// Initialising SOUTHERN face

	double zbF_S = agent->z0;
	double hf_S = 0.0;
	//double qxf_S = 0.0;
	//double qyf_S = 0.0;

	z_F = 0.0;
	h_F_L = 0.0;
	h_F_R = 0.0;
	qx_F_L = 0.0;
	qx_F_R = 0.0;
	qy_F_L = 0.0;
	qy_F_R = 0.0;

	h_R = agent->hFace_S;
	et_R = agent->etFace_S;

	qx_R = agent->qxFace_S;
	qy_R = agent->qyFace_S;

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Applying boundary condition at SOUTH boundary

	h_L = h_R;
	et_L = et_R;

	if (BOUNDARY_SOUTH_STATUS == 1) // 1:Transmissive
	{
		if (INFLOW_BOUNDARY == SOUTH) // 1:NORTH  2:EAST  3:SOUTH  4:WEST // meaning boundary has inflow
		{
			if ((face_location_W.x >= x1_boundary) && (face_location_E.x <= x2_boundary)) // defining the inflow position
			{
				qx_L = qx_R; // inflow data
				qy_L = inflow(sim_time);
			}
			else
			{
				qx_L = qx_R;
				qy_L = qy_R;
			}
		}
		else
		{
			qx_L = qx_R;
			qy_L = qy_R;
		}
	}
	else if (BOUNDARY_SOUTH_STATUS == 2)  // 2:reflective
	{
		if (INFLOW_BOUNDARY == SOUTH) // 1:NORTH  2:EAST  3:SOUTH  4:WEST // meaning boundary has inflow
		{
			if ((face_location_W.x >= x1_boundary) && (face_location_E.x <= x2_boundary)) // defining the inflow position
			{
				qx_L = qx_R; // inflow data
				qy_L = inflow(sim_time);
			}
			else
			{
				qx_L = qx_R;
				qy_L = -qy_R;
			}
		}
		else
		{
			qx_L = qx_R;
			qy_L = -qy_R;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////


	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, SOUTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	GMinus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_S = z_F;
	hf_S = h_F_R;



	xmachine_message_SpaceOperatorMessage* msg = get_first_SpaceOperatorMessage_message<DISCRETE_2D>(SpaceOperatorMessage_messages, agent->x, agent->y);

	while (msg)

	{
		if (msg->inDomain)
		{
			//  Local EAST values and Neighbours' WEST Values are NEEDED
			// EAST PART (PLUS in x direction)
			//if (msg->x + 1 == agent->x //Previous
			if (msg->x - 1 == agent->x
				&& agent->y == msg->y)
			{
				double& h_R = msg->hFace_W;
				double& et_R = msg->etFace_W;
				double2 q_R = make_double2(msg->qFace_X_W, msg->qFace_Y_W);


				double  h_L = agent->hFace_E;
				double  et_L = agent->etFace_E;
				double2 q_L = make_double2(agent->qxFace_E, agent->qyFace_E);

				double z_F = 0.0;
				double h_F_L = 0.0;
				double h_F_R = 0.0;
				double qx_F_L = 0.0;
				double qx_F_R = 0.0;
				double qy_F_L = 0.0;
				double qy_F_R = 0.0;


				//printf("x =%d \t\t y=%d et_L_E=%f \t q_L_E.x=%f  \t q_L_E.y=%f  \n", agent->x, agent->y, et_L_E, q_L_E.x, q_L_E.y);

				//printf("x =%d \t\t y=%d h_R_E=%f \t q_R_E.x=%f  \t q_R_E.y=%f  \n", agent->x, agent->y, h_R_E, q_R_E.x, q_R_E.y);

				//Wetting and drying "depth-positivity-preserving" reconstructions
				WD(h_L, h_R, et_L, et_R, q_L.x, q_R.x, q_L.y, q_R.y, EAST, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

				// Flux across the cell(ic):
				FPlus = hll_x(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

				zbF_E = z_F;
				hf_E = h_F_L;
				//qxf_E = qx_F_L;
				//qyf_E = qy_F_L;

			}

			else
				//if (msg->x - 1 == agent->x //Previous
				if (msg->x + 1 == agent->x
					&& agent->y == msg->y)
				{
					// Local WEST, Neighbour EAST
					// West PART (Minus x direction)
					double& h_L = msg->hFace_E;
					double& et_L = msg->etFace_E;
					double2 q_L = make_double2(msg->qFace_X_E, msg->qFace_Y_E);


					double h_R = agent->hFace_W;
					double et_R = agent->etFace_W;
					double2 q_R = make_double2(agent->qxFace_W, agent->qyFace_W);

					double z_F = 0.0;
					double h_F_L = 0.0;
					double h_F_R = 0.0;
					double qx_F_L = 0.0;
					double qx_F_R = 0.0;
					double qy_F_L = 0.0;
					double qy_F_R = 0.0;

					//printf("x =%d \t\t y=%d h_R_E=%f \t q_R_E.x=%f  \t q_R_E.y=%f  \n", agent->x, agent->y, h_L_W, q_L_W.x, q_L_W.y);

					//* Wetting and drying "depth-positivity-preserving" reconstructions
					WD(h_L, h_R, et_L, et_R, q_L.x, q_R.x, q_L.y, q_R.y, WEST, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

					// Flux across the cell(ic):
					FMinus = hll_x(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

					zbF_W = z_F;
					hf_W = h_F_R;
					//qxf_W = qx_F_R;
					//qyf_W = qy_F_R;

				}
				else
					if (msg->x == agent->x
						&& agent->y == msg->y - 1) //(Previously)
												   //&& agent->y == msg->y + 1)
					{
						//Local NORTH, Neighbour SOUTH
						// North Part (Plus Y direction)
						double& h_R = msg->hFace_S;
						double& et_R = msg->etFace_S;
						double2 q_R = make_double2(msg->qFace_X_S, msg->qFace_Y_S);


						double h_L = agent->hFace_N;
						double et_L = agent->etFace_N;
						double2 q_L = make_double2(agent->qxFace_N, agent->qyFace_N);

						double z_F = 0.0;
						double h_F_L = 0.0;
						double h_F_R = 0.0;
						double qx_F_L = 0.0;
						double qx_F_R = 0.0;
						double qy_F_L = 0.0;
						double qy_F_R = 0.0;

						//printf("x =%d \t\t y=%d h_L_N=%f \t q_L_N.x=%f  \t q_L_N.y=%f  \n", agent->x, agent->y, h_L_N, q_L_N.x, q_L_N.y);

						//printf("x =%d \t\t y=%d h_R_N=%f \t q_R_N.x=%f  \t q_R_N.y=%f  \n", agent->x, agent->y, h_R_N, q_R_N.x, q_R_N.y);

						//* Wetting and drying "depth-positivity-preserving" reconstructions
						WD(h_L, h_R, et_L, et_R, q_L.x, q_R.x, q_L.y, q_R.y, NORTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

						// Flux across the cell(ic):
						GPlus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

						zbF_N = z_F;
						hf_N = h_F_L;
						//qxf_N = qx_F_L;
						//qyf_N = qy_F_L;


					}
					else
						if (msg->x == agent->x
							&& agent->y == msg->y + 1) // previously
													   //&& agent->y == msg->y - 1)
						{
							//Local SOUTH, Neighbour NORTH
							// South part (Minus y direction)
							double& h_L = msg->hFace_N;
							double& et_L = msg->etFace_N;
							double2 q_L = make_double2(msg->qFace_X_N, msg->qFace_Y_N);

							double h_R = agent->hFace_S;
							double et_R = agent->etFace_S;
							double2 q_R = make_double2(agent->qxFace_S, agent->qyFace_S);

							double z_F = 0.0;
							double h_F_L = 0.0;
							double h_F_R = 0.0;
							double qx_F_L = 0.0;
							double qx_F_R = 0.0;
							double qy_F_L = 0.0;
							double qy_F_R = 0.0;


							//printf("x =%d \t\t y=%d h_R_S=%f \t q_R_S.x=%f  \t q_R_S.y=%f  \n", agent->x, agent->y, h_R_S, q_R_S.x, q_R_S.y);
							//printf("x =%d \t\t y=%d h_L_S=%f \t q_L_S.x=%f  \t q_L_S.y=%f  \n", agent->x, agent->y, h_L_S, q_L_S.x, q_L_S.y);

							//* Wetting and drying "depth-positivity-preserving" reconstructions
							WD(h_L, h_R, et_L, et_R, q_L.x, q_R.x, q_L.y, q_R.y, SOUTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

							// Flux across the cell(ic):
							GMinus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

							zbF_S = z_F;
							hf_S = h_F_R;
							//qxf_S = qx_F_R;
							//qyf_S = qy_F_R;


						}
		}

		msg = get_next_SpaceOperatorMessage_message<DISCRETE_2D>(msg, SpaceOperatorMessage_messages);
	}

	// Topography slope
	double z1x_bar = (zbF_E - zbF_W) / 2.0;
	double z1y_bar = (zbF_N - zbF_S) / 2.0;

	// Water height average
	double h0x_bar = (hf_E + hf_W) / 2.0;
	double h0y_bar = (hf_N + hf_S) / 2.0;


	// Evaluating bed slope source terms
	double SS_1 = 0.0;
	double SS_2 = (-GRAVITY * h0x_bar * 2.0 * z1x_bar) / DXL;
	double SS_3 = (-GRAVITY * h0y_bar * 2.0 * z1y_bar) / DYL;

	// Update FV update function with adaptive timestep
	agent->h = agent->h - (dt / DXL) * (FPlus.x - FMinus.x) - (dt / DYL) * (GPlus.x - GMinus.x) + dt * SS_1;
	agent->qx = agent->qx - (dt / DXL) * (FPlus.y - FMinus.y) - (dt / DYL) * (GPlus.y - GMinus.y) + dt * SS_2;
	agent->qy = agent->qy - (dt / DXL) * (FPlus.z - FMinus.z) - (dt / DYL) * (GPlus.z - GMinus.z) + dt * SS_3;




	// Secure zero velocities at the wet/dry front

	double hp = agent->h;

	// Removes zero from taking the minumum of dt from the agents once it is checked in the next iteration 
	// for those agents with hp < TOL_H , in other words assign big number to the dry cells (not retaining zero dt)
	agent->timeStep = BIG_NUMBER;

	//// ADAPTIVE TIME STEPPING
	if (hp <= TOL_H)
	{
		agent->qx = 0.0;
		agent->qy = 0.0;

	}
	else
	{
		double up = agent->qx / hp;
		double vp = agent->qy / hp;

		//store for timestep calc
		agent->timeStep = fminf(CFL * DXL / (fabs(up) + sqrt(GRAVITY * hp)), CFL * DYL / (fabs(vp) + sqrt(GRAVITY * hp)));

	}



	return 0;
}

//


inline __device__ double3 F_SWE(double hh, double qx, double qy)
{
	//This function evaluates the physical flux in the x-direction

	double3 FF = make_double3(0.0, 0.0, 0.0);

	if (hh <= TOL_H)
	{
		FF.x = 0.0;
		FF.y = 0.0;
		FF.z = 0.0;
	}
	else
	{
		FF.x = qx;
		FF.y = (pow(qx, 2.0) / hh) + ((GRAVITY / 2.0)*pow(hh, 2.0));
		FF.z = (qx * qy) / hh;
	}

	return FF;

}


inline __device__ double3 G_SWE(double hh, double qx, double qy)
{
	//This function evaluates the physical flux in the y-direction


	double3 GG = make_double3(0.0, 0.0, 0.0);

	if (hh <= TOL_H)
	{
		GG.x = 0.0;
		GG.y = 0.0;
		GG.z = 0.0;
	}
	else
	{
		GG.x = qy;
		GG.y = (qx * qy) / hh;
		GG.z = (pow(qy, 2.0) / hh) + ((GRAVITY / 2.0)*pow(hh, 2.0));
	}

	return GG;

}

inline __device__ double inflow(double time)
{
	// This function produce a hydrograph of discharge which varies by simulation time

	double q_t;

	if (time <= inflow_start_time)
	{
		q_t = inflow_initial_discharge;
	}
	else if ((time > inflow_start_time) && (time <= inflow_peak_time))
	{
		q_t = (((inflow_peak_discharge - inflow_initial_discharge) / (inflow_peak_time - inflow_start_time))*(time - inflow_start_time)) + inflow_initial_discharge;
	}
	else if ((time > inflow_peak_time) && (time <= inflow_end_time))
	{
		q_t = (((inflow_initial_discharge - inflow_peak_discharge) / (inflow_end_time - inflow_peak_time))*(time - inflow_peak_time)) + inflow_peak_discharge;
	}
	else
	{
		q_t = inflow_end_discharge;
	}

	return q_t;
}

__FLAME_GPU_FUNC__ int outputFloodData(xmachine_memory_FloodCell* agent, xmachine_message_FloodData_list* FloodData_messages)
{
	// This function is created to broadcast the information of updated flood agents to navmap agents

	add_FloodData_message<DISCRETE_2D>(FloodData_messages, 1, agent->x, agent->y, agent->z0, agent->h, agent->qx, agent->qy);

	return 0; 
}

__FLAME_GPU_FUNC__ int updateNavmap(xmachine_memory_navmap* agent, xmachine_message_FloodData_list* FloodData_messages)
{
	// This function updates the information of navmap agents with the flood agents' data
	// lookup for single message
	xmachine_message_FloodData* msg = get_first_FloodData_message<DISCRETE_2D>(FloodData_messages, agent->x, agent->y);

	agent->h = msg->h;						
	agent->qx = msg->qx;
	agent->qy = msg->qy;

	// This is the primary step of taking pedestrians as moving objects, as this logical statements first remove the topography (human height) from the
	// last iteration of the simulation (on navmap agents) this will then be updated in later function (updateNavmapData). 
	// in simple words, if the option of body effect on the water flow is enabled then remove the topography of last iteration.
	if (body_effect_on == ON)
	{
		if (msg->z0 == body_height) //
		{
			agent->z0 = 0;
		}
		else
		{
			agent->z0 = msg->z0;
		}
	}
	else // if the option of body effect is not activated,  take the topography equal to that of flood model
	{
		agent->z0 = msg->z0;
	}


	return 0;
}



__FLAME_GPU_FUNC__ int getNewExitLocation(RNG_rand48* rand48){

	int exit1_compare  = EXIT1_PROBABILITY;
	int exit2_compare  = EXIT2_PROBABILITY  + exit1_compare;
	int exit3_compare  = EXIT3_PROBABILITY  + exit2_compare;
	int exit4_compare  = EXIT4_PROBABILITY  + exit3_compare;
	int exit5_compare  = EXIT5_PROBABILITY  + exit4_compare;
	int exit6_compare  = EXIT6_PROBABILITY  + exit5_compare;
	int exit7_compare  = EXIT7_PROBABILITY  + exit6_compare; //added by MS 12102018
	int exit8_compare  = EXIT8_PROBABILITY  + exit7_compare; //added by MS 12102018
	int exit9_compare  = EXIT9_PROBABILITY  + exit8_compare; //added by MS 12102018
//	int exit10_compare = EXIT10_PROBABILITY + exit9_compare; //added by MS 12102018

	float range = (float) (EXIT1_PROBABILITY +
				  EXIT2_PROBABILITY +
				  EXIT3_PROBABILITY +
				  EXIT4_PROBABILITY +
				  EXIT5_PROBABILITY +
				  EXIT6_PROBABILITY +
				  EXIT7_PROBABILITY +
				  EXIT8_PROBABILITY +
				  EXIT9_PROBABILITY +
				  EXIT10_PROBABILITY);

	float rand = rnd<DISCRETE_2D>(rand48)*range;

	if (rand < exit1_compare)
		return 1;
	else if (rand < exit2_compare)
		return 2;
	else if (rand < exit3_compare)
		return 3;
	else if (rand < exit4_compare)
		return 4;
	else if (rand < exit5_compare)
		return 5;
	else if (rand < exit6_compare)
		return 6;
	else if (rand < exit7_compare)
		return 7;					//added by MS 12102018
	else if (rand < exit8_compare)	//added by MS 12102018
		return 8;					//added by MS 12102018
	else if (rand < exit9_compare)	//added by MS 12102018
		return 9;					//added by MS 12102018
	else                            //added by MS 12102018
		return 10;					//added by MS 12102018

}

__FLAME_GPU_FUNC__ float getRandomHeight(RNG_rand48* rand48) {

	int h_50_100_compare  = PedHeight_50_100_probability;
	int h_100_140_compare = PedHeight_100_140_probability + h_50_100_compare;
	int h_140_160_compare = PedHeight_140_160_probability + h_100_140_compare;
	int h_160_170_compare = PedHeight_160_170_probability + h_140_160_compare;
	int h_170_180_compare = PedHeight_170_180_probability + h_160_170_compare;
	int h_180_200_compare = PedHeight_180_200_probability + h_170_180_compare;
	//int h_200_220_compare = PedHeight_200_220_probability + h_180_200_compare;


	float group_range = (float)(PedHeight_50_100_probability +
		PedHeight_100_140_probability +
		PedHeight_140_160_probability +
		PedHeight_160_170_probability +
		PedHeight_170_180_probability +
		PedHeight_180_200_probability +
		PedHeight_200_220_probability );


	float rand = rnd<CONTINUOUS>(rand48) * group_range;

	//float height_of_pedestrian = 0.5 + (( rand / group_range)*(1.0 - 0.5));
	
	if (rand < h_50_100_compare)
		return 0.5 + ((rand / group_range)*(1.0 - 0.5));
	else if (rand < h_100_140_compare)
		return 1.0 + ((rand / group_range)*(1.4 - 1.0));
	else if (rand < h_140_160_compare)
		return 1.4 + ((rand / group_range)*(1.6 - 1.4));
	else if (rand < h_160_170_compare)
		return 1.6 + ((rand / group_range)*(1.7 - 1.6));
	else if (rand < h_170_180_compare)
		return 1.7 + ((rand / group_range)*(1.8 - 1.7));
	else if (rand < h_180_200_compare)
		return 1.8 + ((rand / group_range)*(1.8 - 2.0));
	else
		return 2.0 + ((rand / group_range)*(2.2 - 2.0));
}

__FLAME_GPU_FUNC__ int getRandomGender(RNG_rand48* rand48) {

	int female_compare = gender_female_probability;
	int male_compare   = gender_male_probability + female_compare;

	float range = (float)(gender_female_probability +
		gender_male_probability);

	float rand = rnd<CONTINUOUS>(rand48) * range;

	if (rand < female_compare)
		return 1; // output female
	else if (rand < male_compare)
		return 2; // output male
	else
		return 0; 
}

/**
 * output_location FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structre of type xmachine_memory_agent. This represents a single agent instance and can be modified directly.
 * @param location_messages Pointer to output message list of type xmachine_message_location_list. Must be passed as an argument to the add_location_message function ??.
 */
__FLAME_GPU_FUNC__ int output_pedestrian_location(xmachine_memory_agent* agent, xmachine_message_pedestrian_location_list* pedestrian_location_messages){

    
	add_pedestrian_location_message(pedestrian_location_messages, agent->x, agent->y, 0.0);
  
    return 0;
}

/**
 * output_navmap_cells FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structre of type xmachine_memory_navmap. This represents a single agent instance and can be modified directly.
 * @param navmap_cell_messages Pointer to output message list of type xmachine_message_navmap_cell_list. Must be passed as an argument to the add_navmap_cell_message function ??.
 */
__FLAME_GPU_FUNC__ int output_navmap_cells(xmachine_memory_navmap* agent, xmachine_message_navmap_cell_list* navmap_cell_messages){
    

			add_navmap_cell_message<DISCRETE_2D>(navmap_cell_messages, 
			agent->x, agent->y, agent->z0, agent->h, agent->qx, agent->qy,
			agent->exit_no, 
			agent->height,
			agent->collision_x, agent->collision_y, 
			agent->exit0_x, agent->exit0_y,
			agent->exit1_x, agent->exit1_y,
			agent->exit2_x, agent->exit2_y,
			agent->exit3_x, agent->exit3_y,
			agent->exit4_x, agent->exit4_y,
			agent->exit5_x, agent->exit5_y,
			agent->exit6_x, agent->exit6_y,
			agent->exit7_x, agent->exit7_y, 
			agent->exit8_x, agent->exit8_y, 
			agent->exit9_x, agent->exit9_y);
 
    return 0;
}



/**
 * move FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structre of type xmachine_memory_agent. This represents a single agent instance and can be modified directly.
 * @param location_messages  location_messages Pointer to input message list of type xmachine_message__list. Must be passed as an argument to the get_first_location_message and get_next_location_message functions.* @param partition_matrix Pointer to the partition matrix of type xmachine_message_location_PBM. Used within the get_first__message and get_next__message functions for spatially partitioned message access.* @param rand48 Pointer to the seed list of type RNG_rand48. Must be passed as an arument to the rand48 function for genertaing random numbers on the GPU.
 */
__FLAME_GPU_FUNC__ int avoid_pedestrians(xmachine_memory_agent* agent, xmachine_message_pedestrian_location_list* pedestrian_location_messages, xmachine_message_pedestrian_location_PBM* partition_matrix, RNG_rand48* rand48){

	glm::vec2 agent_pos = glm::vec2(agent->x, agent->y);
	glm::vec2 agent_vel = glm::vec2(agent->velx, agent->vely);

	glm::vec2 navigate_velocity = glm::vec2(0.0f, 0.0f);
	glm::vec2 avoid_velocity = glm::vec2(0.0f, 0.0f);

	xmachine_message_pedestrian_location* current_message = get_first_pedestrian_location_message(pedestrian_location_messages, partition_matrix, agent->x, agent->y, 0.0);
	while (current_message)
	{
		glm::vec2 message_pos = glm::vec2(current_message->x, current_message->y);
		
		float separation = length(agent_pos - message_pos);

		//printf("\nseparation = %f \n", separation);
		//printf("\nMESSAGE_RADIUS = %f \n", MESSAGE_RADIUS);
		//printf("\n\nMIN_DISTANCE = %f \n", MIN_DISTANCE);

		if ((separation < MESSAGE_RADIUS)&&(separation > MIN_DISTANCE)){
			glm::vec2 to_agent = normalize(agent_pos - message_pos);
			float ang = acosf(dot(agent_vel, to_agent));
			float perception = 45.0f;

			//STEER
			if ((ang < RADIANS(perception)) || (ang > 3.14159265f-RADIANS(perception))){
				glm::vec2 s_velocity = to_agent;
				s_velocity *= powf(I_SCALER/separation, 1.25f)*STEER_WEIGHT;
				navigate_velocity += s_velocity;
			}

			//AVOID
			glm::vec2 a_velocity = to_agent;
			a_velocity *= powf(I_SCALER/separation, 2.00f)*AVOID_WEIGHT;
			avoid_velocity += a_velocity;						

		}
		 current_message = get_next_pedestrian_location_message(current_message, pedestrian_location_messages, partition_matrix);
	}

	//maximum velocity rule
	glm::vec2 steer_velocity = navigate_velocity + avoid_velocity;

	agent->steer_x = steer_velocity.x;
	agent->steer_y = steer_velocity.y;

    return 0;
}

  
/**
 * force_flow FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structre of type xmachine_memory_agent. This represents a single agent instance and can be modified directly.
 * @param navmap_cell_messages  navmap_cell_messages Pointer to input message list of type xmachine_message__list. Must be passed as an argument to the get_first_navmap_cell_message and get_next_navmap_cell_message functions.
 */
__FLAME_GPU_FUNC__ int force_flow(xmachine_memory_agent* agent, xmachine_message_navmap_cell_list* navmap_cell_messages, RNG_rand48* rand48){

	// the original position of pedestrian agents
    //map agent position into 2d grid
	/*int x = floor(((agent->x+ENV_MAX)/ENV_WIDTH)*d_message_navmap_cell_width);
	int y = floor(((agent->y+ENV_MAX)/ENV_WIDTH)*d_message_navmap_cell_width);*/

	// modified position of pedestrian agents
	//  x and y has been swapped so as to rotate the grid of navmap to be alligned with flood grid (MS02102018)
	int x = floor(((agent->y + ENV_MAX) / ENV_WIDTH)*d_message_navmap_cell_width); 
	int y = floor(((agent->x + ENV_MAX) / ENV_WIDTH)*d_message_navmap_cell_width);

	//lookup single message
    xmachine_message_navmap_cell* current_message = get_first_navmap_cell_message<CONTINUOUS>(navmap_cell_messages, x, y);

  
	glm::vec2 collision_force = glm::vec2(current_message->collision_x, current_message->collision_y);
	collision_force *= COLLISION_WEIGHT;



	//exit location of navmap cell
	int exit_location = current_message->exit_no;

	//agent death flag
	int kill_agent = 0;
	
	//goal force
	glm::vec2 goal_force;

	if (agent->exit_no == 1)
	{
		goal_force = glm::vec2(current_message->exit0_x, current_message->exit0_y);
		if (exit_location == 1)
			if (EXIT1_STATE == 1) //EXIT1_PROBABILITY != 0 prevents killing agents while flooding at the exits (for sandbagging test is added MS08102018)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);

	}
	else if (agent->exit_no == 2)
	{
		goal_force = glm::vec2(current_message->exit1_x, current_message->exit1_y);
		if (exit_location == 2)
			if (EXIT2_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 3)
	{
		goal_force = glm::vec2(current_message->exit2_x, current_message->exit2_y);
		if (exit_location == 3)
			if (EXIT3_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 4)
	{
		goal_force = glm::vec2(current_message->exit3_x, current_message->exit3_y);
		if (exit_location == 4)
			if (EXIT4_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 5)
	{
		goal_force = glm::vec2(current_message->exit4_x, current_message->exit4_y);
		if (exit_location == 5)
			if (EXIT5_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 6)
	{
		goal_force = glm::vec2(current_message->exit5_x, current_message->exit5_y);
		if (exit_location == 6)
			if (EXIT6_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 7)
	{
		goal_force = glm::vec2(current_message->exit6_x, current_message->exit6_y);
		if (exit_location == 7)
			if (EXIT7_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 8)
	{
		goal_force = glm::vec2(current_message->exit7_x, current_message->exit7_y);
		if (exit_location == 8)
			if (EXIT8_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 9)
	{
		goal_force = glm::vec2(current_message->exit8_x, current_message->exit8_y);
		if (exit_location == 9)
			if (EXIT9_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 10)
	{
		goal_force = glm::vec2(current_message->exit9_x, current_message->exit9_y);
		if (exit_location == 10)
			if (EXIT10_STATE == 1)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
	}
	
	//scale goal force
	goal_force *= GOAL_WEIGHT;

	agent->steer_x += collision_force.x + goal_force.x;
	agent->steer_y += collision_force.y + goal_force.y;

	//update height //////// Do not update the height of pedestrian (MS23092019)
	// agent->body_height = current_message->height;

	// take the maximum velocity of water in both x- and y-axis directions from navmap cells
	// absolute make the discharge as a positive value for further use in conditional statement 
	double water_height = abs(current_message->h); // take the absolute when topography is activated
	double water_qx = abs(current_message->qx);
	double water_qy = abs(current_message->qy);
	
	double water_velocity_x = 0.0f;
	double water_velocity_y = 0.0f;

	if (water_height != 0.0)
	{
		water_velocity_x = water_qx / water_height; 
		water_velocity_y = water_qy / water_height;
	}
	

	// Water velocity regardless of direction and whether it is (-) or (+)
	double water_velocity = max(water_velocity_x, water_velocity_y);

	// hazard rating the flow
	double HR = water_height * (water_velocity + 0.5);

	//////////////////////////////////// Incipient velocity estimation based on Xia et al (2014) proposed fomulation ////////////////////////////////////
	// 'u_c_s' and 'u_c_t' are incipient velocities due to sliding and toppling, respectively.
	double u_c_s, u_c_t;
	double alpha_s = 7.975; // Sliding constant (m^0.5s^-1) proposed by Xia et al 2014 (See Table 1)
	double beta_s = 0.018; // Sliding constant (-) proposed by Xia et al 2014 (See Table 1)
	double alpha_t = 3.472; // Toppling constant (m^0.5s^-1) proposed by Xia et al 2014 (See Table 1)
	double beta_t = 0.188; // Toppling constant (-) proposed by Xia et al 2014 (See Table 1)
	double alpha_1 = 0.633 ; // constant parameter proposed by Xia et al 2014 (See Table 1)
	double beta_1 = 0.367; // constant parameter proposed by Xia et al 2014 (See Table 1)
	double alpha_2 = 1.015e-3; // constant parameter proposed by Xia et al 2014 (See Table 1)
	double beta_2 = -4.927e-3; // constant parameter proposed by Xia et al 2014 (See Table 1)
	// NOTE 1: these parameters are proposed for a Chinese human body and they may vary by the user (ALL NEED TO BE ADDED IN INPUT FILE)
	// NOTE 2: These parameters are calibrated for a model of human body, for real human body experimental calibrations see Table 2 of Xia et al (2014)
	double rho = 1000; //density of water (kg/m3)

	if (water_height > 0.0f) // avoid calculations if there is no water
	{
		// incipient velocity based on sliding instability formulation
		u_c_s = alpha_s * powf( (water_height/agent->body_height), beta_s) * sqrtf( (agent->body_mass/(rho*agent->body_height*water_height))-(alpha_1*(water_height/agent->body_height)+ beta_1)*(((alpha_2*agent->body_mass)+beta_2)/powf(agent->body_height,2.0)) );
		
		// incipient velocity based on toppling instability formulation
		u_c_t = alpha_t * powf((water_height / agent->body_height), beta_t) * sqrtf( (agent->body_mass / (rho*powf(water_height, 2.0))) - ( (alpha_1/powf(agent->body_height, 2.0))+ (beta_1/(water_height*agent->body_height)))*((alpha_2*agent->body_mass) + beta_2) );
	}

	printf("\n\nSLIDING INCIPIENT VELOCITY = %f \n", u_c_s);
	printf("\n\nTOPPLING INCIPIENT VELOCITY = %f \n", u_c_t);


	// update agent's memory with new HR
	agent->HR = HR;

	// Update internal pedestrians' memory with the information of the water flows (for stability study purposes)
	agent->d_water = water_height; 
	agent->v_water = water_velocity;

	// Allocating the state of pedestrians based on Hazard Rating (HR) factor proposed by the EA

		if (HR <= epsilon && HR <= epsilon)
		{
			agent->HR_state = STATE_ALIVE_DRY;
		}				
		else
		{
			if (HR > epsilon && HR <= 0.75)
				agent->HR_state = STATE_ALIVE_RUN;
			else if (HR >  0.75 && HR <= 1.5)
				agent->HR_state = STATE_ALIVE_WALK; // Disrupted 1
			else if (HR > 1.5 && HR <= 2.5)
				agent->HR_state = STATE_ALIVE_STILL; // Disrupted 2
			else if (HR > 2.5)
				agent->HR_state = STATE_DEAD; // Still
		}
			
		
		// Allocating the stability state of the pedestrian based on incepient velocity at which the pedestrian might not be stable due to sliding or toppling
		if (water_height > 0.0f) // avoid calculations if there is no water
		{
			if (agent->v_water < u_c_s && agent->v_water < u_c_t)
				agent->stability_state = 0; // means agent is stable
			if (agent->v_water >= u_c_s && agent->v_water < u_c_t)
				agent->stability_state = 1;
			else if (agent->v_water < u_c_s && agent->v_water >= u_c_t)
				agent->stability_state = 2;
			else if (agent->v_water >= u_c_s && agent->v_water >= u_c_t)
				agent->stability_state = 3;
		}

			printf("\n\nThe water depth hit the pedestrian = %f \n", agent->d_water);
			printf("\n\nThe water velocity hit the pedestrian = %f \n", agent->v_water);
			printf("\n\nThe HR hit the pedestrian = %.5f \n", agent->HR);
			printf("\n\nStability status of the pedestrian = %d \n", agent->stability_state);

	// update the state of pedestrian with respect to the water flow info
	// this set of logical statements apply the rules to pedestrians to change their states with respect to water height and velocity
	// if the pedestrian is not dead, then estimate the next state based on the height and velocity of the water
	

    return kill_agent;
}

/**
 * move FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structre of type xmachine_memory_agent. This represents a single agent instance and can be modified directly.
 
 */
__FLAME_GPU_FUNC__ int move(xmachine_memory_agent* agent) {


	glm::vec2 agent_pos = glm::vec2(agent->x, agent->y);

	glm::vec2 agent_vel = glm::vec2(agent->velx, agent->vely);

	glm::vec2 agent_steer = glm::vec2(agent->steer_x, agent->steer_y);


	//// default current_speed based on FloodPedestrian model MS02092019
	float current_speed = length(agent_vel) + 0.025f;//(powf(length(agent_vel), 1.75f)*0.01f)+0.025f;
	
	// Calculate the magnitude of water depth and velocity pairs (MS 19092019) based on Ishigaki et al. 2009 -> reported in Postaccini 2018 
	float M = (powf(agent->v_water, 2.0)*agent->d_water) / GRAVITY + (0.5 * powf(agent->d_water, 2.0));

	// Defining the speed limit when normal condition is considered
	// Change the speed of pedestrian based on water depth and velocity (MS 19092019)
	
	if (agent->d_water > epsilon) // calculate new walking speed based on the experimental data by Postaccini et al (2018)
	{		
		//current_speed = walking_speed(agent);
		current_speed = 0.53 * powf(M, -0.19); //Postaccini et al (2018)
		// limit the speed if exceeds
		if (current_speed > init_speed)
			current_speed = init_speed;
	}

	  //Defining the speed limit in evacuation condition
	if (evacuation_on == ON)
	{
		// initially take the brisk walk speed for the evacuation condition as dominant
		current_speed = brisk_speed;

		if (agent->d_water > epsilon) // calculate new walking speed based on the experimental data by Postaccini et al
		{
			////current_speed = walking_speed(agent);
			//current_speed = 0.53 * powf(M, -0.19); //Postaccini et al (2018)

			// Limiting the walking speed to the initial brisk walk speed
			if (current_speed > brisk_speed)
				current_speed = brisk_speed;
		}
	}

	//// to print the speed of the pedestrian (MS03092019)
	printf("\nPedestrian walking speed = %.3f m/s", current_speed);
	//printf("\nThe height of the pedestrian = %f m", agent->body_height); // MS23092019
	//printf("\nThe body mass of the pedestrian = %f kg", agent->body_mass);
	//printf("\nThe gender of pedestrian = %d ", agent->gender);

	// vector of current position before the update for new iteration
	float r1 = length(agent_pos);

    //apply more steer if speed is greater
	agent_vel += current_speed*agent_steer;
	float speed = length(agent_vel); 

	float speed_ratio = 1.0f; // initial value for the ration, in case of 'speed' is less than 'agent->speed' . (meaning the updated speed is less than the current speed)
	
	//limit speed - if the new evaluated speed is more than the pedestrian's current speed.
	if (speed >= agent->speed)
	{
		agent_vel = normalize(agent_vel)*agent->speed;
		speed = agent->speed; // a normalised speed now. 
		speed_ratio = current_speed/speed ; // concerns the differences between the initial walking speed and that of in floodwater. 
		if (speed_ratio > 1.0f)
		{
			speed_ratio = 1.0; 
		}
	}

	//printf("\nThe walking ANIMATED speed = %.3f m/s", speed);
	//printf("\nspeed_ratio = %.3f m/s", speed_ratio);
	//printf("\ndt = %f s", dt);
	

	float time_step_ratio = (float)dt / (float)dt_ped ;

	//printf("\ndt_ped = %f s", dt_ped);
	//printf("\ndt_flood = %f s", dt_flood);
	//printf("\ntime_step_ratio = %f s", time_step_ratio);

	// Update the location of pedestrian
	agent_pos += agent_vel*time_step_ratio*speed_ratio*TIME_SCALER;

	// Prepare for animation
	agent->animate += 0.1*(agent->animate_dir * speed_ratio*powf(speed, 2.0f)*TIME_SCALER*100.0f)*time_step_ratio;

	// do not move pedestrian if they have lost their stability
	if ( agent->stability_state != 0 )
	{
		// Update the location of pedestrian
		agent_pos += -(agent_vel*time_step_ratio*speed_ratio*TIME_SCALER);
		// Prepare for animation
		agent->animate += (-0.1*(agent->animate_dir * speed_ratio*powf(speed, 2.0f)*TIME_SCALER*100.0f)*time_step_ratio);
	}
		
	// vector of updated position
	float r2 = length(agent_pos);

	// update pedestrian's dt based on the displacement in [xmin xmax] and walking speed
	agent->dt_ped = (((xmax - xmin) / ENV_WIDTH) * fabs(r2 - r1)) / speed;
	//printf("\nENV_WIDTH = %f s", ENV_WIDTH);
	//printf("\nr2 = %f s", r2);
	//printf("\nr1 = %f s", r1);
	//printf("\ndt_ped for next iteration = %f s", agent->dt_ped);


	if (agent->animate >= 1)
		agent->animate_dir = -1;
	if (agent->animate <= 0)
		agent->animate_dir = 1;

	//double temporary_time_scaler= ;

	//printf("\n***time_scale_new = %f s", time_scale_new);
	//printf("\n***TIME_SCALER = %f s", TIME_SCALER);

	//lod
	agent->lod = 1;

	//update
	agent->x = agent_pos.x;
	agent->y = agent_pos.y;
	agent->velx = agent_vel.x;
	agent->vely = agent_vel.y;

	//printf("velocity in x= %f and y= %f", agent_vel.x, agent_vel.y);

	//bound by wrapping
	if (agent->x < -1.0f)
		agent->x+=2.0f;
	if (agent->x > 1.0f)
		agent->x-=2.0f;
	if (agent->y < -1.0f)
		agent->y+=2.0f;
	if (agent->y > 1.0f)
		agent->y-=2.0f;


    return 0;
}



/**
 * generate_pedestrians FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structre of type xmachine_memory_navmap. This represents a single agent instance and can be modified directly.
 * @param agent_agents Pointer to agent list of type xmachine_memory_agent_list. This must be passed as an argument to the add_agent_agent function to add a new agent.* @param rand48 Pointer to the seed list of type RNG_rand48. Must be passed as an arument to the rand48 function for genertaing random numbers on the GPU.
 */
__FLAME_GPU_FUNC__ int generate_pedestrians(xmachine_memory_navmap* agent, xmachine_memory_agent_list* agent_agents, RNG_rand48* rand48){

    if (agent->exit_no > 0)
	{
		float random = rnd<DISCRETE_2D>(rand48);
		bool emit_agent = false;

		if ((agent->exit_no == 1)&&((random < EMMISION_RATE_EXIT1*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 2)&&((random <EMMISION_RATE_EXIT2*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 3)&&((random <EMMISION_RATE_EXIT3*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 4)&&((random <EMMISION_RATE_EXIT4*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 5)&&((random <EMMISION_RATE_EXIT5*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 6)&&((random <EMMISION_RATE_EXIT6*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 7)&&((random <EMMISION_RATE_EXIT7*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 8) && ((random <EMMISION_RATE_EXIT8*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 9) && ((random <EMMISION_RATE_EXIT9*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 10) && ((random <EMMISION_RATE_EXIT10*TIME_SCALER)))
			emit_agent = true;


		// Do not emit pedestrians where the barrier is located
		if ((agent->exit_no == drop_point))
			emit_agent = false;

		if (emit_agent){
			float x = ((agent->x+0.5f)/(d_message_navmap_cell_width/ENV_WIDTH))-ENV_MAX;
			float y = ((agent->y+0.5f)/(d_message_navmap_cell_width/ENV_WIDTH))-ENV_MAX;
			int exit = getNewExitLocation(rand48);
			
			float animate = rnd<DISCRETE_2D>(rand48);			
			
			float body_height = getRandomHeight(rand48); // it is in metre 
			//body mass: in kg based on the paper "Estimation of weight in adults from height: a novel option for a quick bedside technique" : (body_height - 1)*100
			float body_mass = (body_height - 1)*100 + (-10) + (rnd<CONTINUOUS>(rand48)*(20)); // adding +/- 10% random telorance within: (-10) + (rnd<CONTINUOUS>(rand48)*(20)

			int gender = getRandomGender(rand48);

			//// For random initial speed generation
			//// For a pre-defined walking speed condition.  0.025 is now reduced from the initial speed to observe the actual defined one, since
			//// it is later added to the pedestrian for their first step walking in 'move' function.
			float speed = init_speed - 0.025;
			
			if (evacuation_on == ON && sandbagging_on == ON)
			{
				// limit the number of pedestrians to a certain number
				if (count_population < pedestrian_population)
				{
					// produce certain number of hero pedestrians with hero_status =1

					if (count_heros <= hero_population)
					{
						//add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->body_height, exit, speed, 1, animate, 1, 1, 1, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0);
						add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1, 1, 1, 0.0f, 0.0f, 0, 0, 0, 0, 0, body_height, body_mass, gender, 0);
					}
					else
					{
						//add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->body_height, exit, speed, 1, animate, 1, 1, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0);
						add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1, 1, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0,body_height, body_mass, gender, 0);
					}

				}
			}
			else
			{
				// limit the number of pedestrians to a certain number
				if (count_population < pedestrian_population)
				{			
						//add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->body_height, exit, speed , 1, animate, 1, 1, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0);
						add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1, 1, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0,body_height, body_mass, gender, 0);

				}
			}						
		}
	}


    return 0;
}

__FLAME_GPU_FUNC__ int output_PedData(xmachine_memory_agent* agent, xmachine_message_PedData_list* pedestrian_PedData_messages) {
	
	// This function broadcasts a message containing the information of pedestrian to navmap agents

	// Taking the grid position based on pedestrians' global position
	//int x = floor(((agent->x + ENV_MAX) / ENV_WIDTH)*d_message_navmap_cell_width);
	//int y = floor(((agent->y + ENV_MAX) / ENV_WIDTH)*d_message_navmap_cell_width);

	//printf(" x of pedestrian is = %f \n ", agent->x);

	add_PedData_message(pedestrian_PedData_messages, agent->x, agent->y, 0.0, agent->hero_status, agent->pickup_time, agent->drop_time, agent->exit_no, agent->carry_sandbag);

	return 0;
}

__FLAME_GPU_FUNC__ int updateNavmapData(xmachine_memory_navmap* agent, xmachine_message_PedData_list* PedData_messages, xmachine_message_PedData_PBM* partition_matrix, xmachine_message_updatedNavmapData_list* updatedNavmapData_messages)
{
	// This function loads the messages broadcasted from pedestrians containing their location in the domain and updates the sandbag capacity of navmap agents
	// in order to update the topography in 'updateNeighbourNavmap' function which is designed to take the filled capacity of navmap and increase the 
	// topographic height with respect to the number of sandbags put by the pedestrians
	// Also the information of updated navmap agents is broadcasted to their neighbours within this function


	// FindRescaledGlobalPosition_navmap function rescale the domain size of navmaps to fit [-1 1] so as to read the messages of pedestrians,
	// more, this function takes the location of each navmap in a domain bounded from -1 to 1 
	// for example, if xmax=250(m) and grid size 'n'=128 then for agent->x = 64 the global position is 128.0m and rescaled one in [-1 1] is 0 
	float2 navmap_loc = FindRescaledGlobalPosition_navmap(agent, make_double2(-0.5, -0.75));

	// Vector of agents' position
	glm::vec2 agent_pos = glm::vec2(navmap_loc.x, navmap_loc.y);

	// This function calculates the global position of each navmap based on the size of flood domain 
	//float2 navmap_loc_global = FindGlobalPosition_navmap(agent, make_double2(0.0, 0.0));

	////for outputting the location of drop point
	//if (agent->exit_no == drop_point)
	//{
	//	printf(" the location of drop point x=%f and y=%f \n ", navmap_loc_global.x, navmap_loc_global.y);
	//}

	
	//Loading a single message
	xmachine_message_PedData* msg = get_first_PedData_message(PedData_messages, partition_matrix, (float)navmap_loc.x, (float)navmap_loc.y, 0.0);

	//glm::vec2 drop_pos, pickup_pos;
	
	// get the exit location of navmap cell
	int exit_location = agent->exit_no;

	// load messages from pedestrian
	while (msg)	
	{
		// Check if the body effect option is activated. If so, take pedestrians as moving objects (obstacles for water propagation)
		if (body_effect_on == ON)
		{
			// locate the position of each pedestrian within the grid of navmap agents
			glm::vec2 msg_pos = glm::vec2(msg->x, msg->y);
			
			// distance between the navmap agent and the pedestrian
			float distance = length(agent_pos - msg_pos);
			
			// update the topography up to the human height only if there is no pre-defined topography in the model's initial condition
			if ((distance < MESSAGE_RADIUS) && (agent->z0 == 0.0f))  // MS added  && (agent->z0 == 0.0f) on 06092019
			{
					agent->z0 = body_height; 			
			}
		}
		

		if (evacuation_on == ON)
		{
			if (sim_time > sandbagging_start_time && sim_time < sandbagging_end_time)
			{
				if (exit_location == drop_point) // for updating the sandbag_capacity of ONLY the drop point
				{
					if ( msg->hero_status == ON) // only if the emergency responders reach to this point
					{

						if (msg->pickup_time == 0.0f && msg->drop_time != 0.0f && msg->carry_sandbag > 0) // condition showing the moment when pedestrian holds a sandbag
						{
							agent->sandbag_capacity++;
						}
					}
				}
			}
		}
			msg = get_next_PedData_message(msg, PedData_messages, partition_matrix);
	}


	//if (exit_location == drop_point)
	//{
	//	printf("\n *sandbag_capacity = %d \n", agent->sandbag_capacity);
	//}

		add_updatedNavmapData_message<DISCRETE_2D>(updatedNavmapData_messages, agent->x, agent->y, agent->z0, agent->drop_point, agent->sandbag_capacity, agent->exit_no);

	return 0;
}


__FLAME_GPU_FUNC__ int updateNeighbourNavmap(xmachine_memory_navmap* agent, xmachine_message_updatedNavmapData_list* updatedNavmapData_messages, xmachine_message_NavmapData_list* NavmapData_messages)
{
	// This function is created to extend the length of sandbag dike with respect to the starting point (drop_poin) and 
	//  x direction. 
	// Also, the updated data is broadcasted to flood agents
	
	//NOTE: in this current version of the model, the sandbag dike can only be extended in x direction (from west to east)
	// for future development (MS comments): there will be options to which direction sandbaging is going to be extended. 

	xmachine_message_updatedNavmapData* msg = get_first_updatedNavmapData_message<DISCRETE_2D>(updatedNavmapData_messages, agent->x, agent->y);

	// find the global position of the Western interface of the navmap where the sadbaging is started
	//double2 navmap_westface_loc = FindGlobalPosition(agent, make_double2(-DXL*0.5, 0.0));
 	//int agent_drop_point = agent->drop_point;

		while (msg)
		{
			if (sandbagging_on == ON) // eliminate updating the topography mistakenly while there is no need for sandbagging when the body_effect is on
			{
				if (agent->x == msg->x + 1) // agent located at the right side of the agent
				{
					if (agent->y == msg->y)
					{
						// if agent is located next to drop point which is *not* also located at exit point
						if (msg->exit_no == drop_point && agent->exit_no != drop_point) // taking messages from navmaps located at the exit points and exclude the exit navmap agents at drop point
						{
							if (msg->sandbag_capacity > fill_cap)
							{
								agent->sandbag_capacity = msg->sandbag_capacity - fill_cap;

								agent->drop_point = 1;

								//(**1) see below the while loop
								if (agent->sandbag_capacity == fill_cap
									|| agent->sandbag_capacity == fill_cap + 1
									|| agent->sandbag_capacity == fill_cap + 2
									|| agent->sandbag_capacity == fill_cap + 3
									|| agent->sandbag_capacity == fill_cap + 4
									|| agent->sandbag_capacity == fill_cap + 5
									|| agent->sandbag_capacity == fill_cap + 6
									|| agent->sandbag_capacity == fill_cap + 7
									|| agent->sandbag_capacity == fill_cap + 8)
								{
									agent->z0 = sandbag_layers * sandbag_height;
								}
							}
						}
						else if (msg->drop_point == ON) // if the message is coming from a drop_point agent
						{
							if (msg->sandbag_capacity > fill_cap)
							{
								agent->sandbag_capacity = msg->sandbag_capacity - fill_cap;

								agent->drop_point = 1;
								//(**1) see below the while loop
								if (agent->sandbag_capacity == fill_cap
									|| agent->sandbag_capacity == fill_cap + 1
									|| agent->sandbag_capacity == fill_cap + 2
									|| agent->sandbag_capacity == fill_cap + 3
									|| agent->sandbag_capacity == fill_cap + 4
									|| agent->sandbag_capacity == fill_cap + 5
									|| agent->sandbag_capacity == fill_cap + 6
									|| agent->sandbag_capacity == fill_cap + 7
									|| agent->sandbag_capacity == fill_cap + 8)
								{
									agent->z0 = sandbag_layers * sandbag_height;

									// (**2) see below the while loop
									if (agent->z0 > msg->z0)
									{
										agent->z0 = msg->z0;
									}
								}
							}
						}

					}
				}
			}

			msg = get_next_updatedNavmapData_message<DISCRETE_2D>(msg, updatedNavmapData_messages);
		}

		// (**1) Check if the capacity of navmap is reached to the required number to update the topography, it checks with the telorance of 8 sandbags,
		//		since for a large number of people putting multiple sandbags at the same time, sometimes, 
		//		it may reach more than fill_cap, and thus wont meet the logical statement

		// (**2) To keep the level of sandbag height at last sandbag navmap within which the layer counter is increased, 
		//		where the extended sandbag length is reached to the proposed dike length
		
	if (agent->exit_no == drop_point) // updating the navmap agents located on the drop_point
	{
			//(**1) see below the while loop
		if (agent->sandbag_capacity == fill_cap
			|| agent->sandbag_capacity == fill_cap + 1
			|| agent->sandbag_capacity == fill_cap + 2
			|| agent->sandbag_capacity == fill_cap + 3
			|| agent->sandbag_capacity == fill_cap + 4
			|| agent->sandbag_capacity == fill_cap + 5
			|| agent->sandbag_capacity == fill_cap + 6
			|| agent->sandbag_capacity == fill_cap + 7
			|| agent->sandbag_capacity == fill_cap + 8)
		{
			agent->z0 = sandbag_layers * sandbag_height;
		}
	}


	//if (agent->drop_point == 1 || agent->exit_no == drop_point)
	//{
	//	printf(" Sandbag acapcity of the drop point navmap is = %d \n ", agent->sandbag_capacity);
	//	printf(" the height of sandbag is = %f \n ", agent->z0);
	//}

	// restart the capacity once a layer is applied
	if (extended_length >= dike_length)
	{
		agent->sandbag_capacity = 0;
	}

	add_NavmapData_message<DISCRETE_2D>(NavmapData_messages, agent->x, agent->y, agent->z0);

	return 0;
}


__FLAME_GPU_FUNC__ int UpdateFloodTopo(xmachine_memory_FloodCell* agent, xmachine_message_NavmapData_list* NavmapData_messages)
{

	// This function loads the information of navmap agents and updates the data of flood agents in each iteration

	xmachine_message_NavmapData* msg = get_first_NavmapData_message<DISCRETE_2D>(NavmapData_messages, agent->x, agent->y);

	// find the global position of the centre of flood agent
	//double2 flood_agent_loc = FindGlobalPosition(agent, make_double2(0.0, 0.0));
	/*while (msg)
	{*/	
	
	// restart the topography presenting the body of pedestrian to zero
	if (body_effect_on == ON)
	{
		agent->z0 = 0;
	}
	

	// Update new topography data to flood agents
	if (agent->z0 < msg->z0 ) // Do not update if the floodCell agent has already a topgraphic feature (e.g. building data)
	{
		if (agent->x == msg->x && agent->y == msg->y)
		{
			agent->z0 = msg->z0;
		}
	}

	/*	msg = get_next_NavmapData_message<DISCRETE_2D>(msg, NavmapData_messages);
	}*/


	return 0;
}





#endif
