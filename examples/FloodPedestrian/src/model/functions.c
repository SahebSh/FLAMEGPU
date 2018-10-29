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
#define GLOBAL_MANNING		0.018000 
#define CFL					0.5
#define TOL_H				10.0e-4
#define BIG_NUMBER			800000

// Spatial and timing scalers for pedestrian model
#define SCALE_FACTOR		0.03125
#define I_SCALER			(SCALE_FACTOR*0.35f)
#define MESSAGE_RADIUS		d_message_pedestrian_location_radius
#define MIN_DISTANCE		0.0001f

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

	// loading constant variables in host function
	int x_min, x_max, y_min, y_max;
	x_min = *get_xmin();
	x_max = *get_xmax();
	y_min = *get_ymin();
	y_max = *get_ymax();

	// get the first dt from default pedestrian dt (defined ini 0.xml file)// this is to make dt changable in different conditions
	// where the simulation is required to be faster (accelerating the simulation of movement of people)
	double dt_ped = *get_dt_ped();
	// To sync flood dt with the speed of pedestrians. (the distance passed by flood water should be equal to that of pedestrian 
	// with average speed rate of 1.39 m/s) . Experienced that it is almost 250 times faster thatn the speed of people (by doing a number of sims)
	float speed_factor_pedestrians = (dt_ped /260);

	// the size of the computaional cell (flood agents)
	// sqrt(xmachine_memory_FloodCell_MAX) is the number of agents in each direction. 
	double dxl = (x_max - x_min) / sqrt(xmachine_memory_FloodCell_MAX);
	double dyl = (y_max - y_min) / sqrt(xmachine_memory_FloodCell_MAX);

	////////// adaptive time-stepping commented to take way to constant time-step initialised in input file (0.xml)////////////
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

	//set_dt(&dt_init); //commented to load dt from 0.xml
	set_DXL(&dxl);
	set_DYL(&dyl);
	set_sim_time(&sim_init);
	set_TIME_SCALER(&speed_factor_pedestrians);


	// loading the pre-defined dimension of sandbags in 0.xml file
	float sandbag_length = *get_sandbag_length();
	float sandbag_width  = *get_sandbag_width();

	// calculating the area that one single sandbag can possibly occupy
	float area_sandbags  = sandbag_length * sandbag_width ;
	// The area based on the resolution of the domain
	float area_floodcell = dxl * dyl;
	//  the number of sandbag required to fill the area of one single navmap/flood agent
	int fill_area = rintf((float)area_floodcell / (float)area_sandbags);

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

	// dt_flood will be chosen in case the model is required to run on static time-stepping, 
	double dt_flood = *get_dt_flood();

	// setting general dt equal to pedestrian dt initially since there is no flood at the start of the simulation
	set_dt(&dt_ped);
	
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
	//double flow_h_max = max_navmap_static_h_variable();
	double flow_h_max = max_FloodCell_Default_h_variable();

	// rescaling speed factor based on the time-stepping of flood
	float speed_factor_pedestrians = (old_dt / 260); // use this for adaptive time-stepping
	
	set_TIME_SCALER(&speed_factor_pedestrians);

	int auto_dt_on = *get_auto_dt_on();
	//setting dt to that of defined for flood model to preserve the stability of numerical solutions for flood water
	// can either be set to static for (in which dt is set based on the defined value in 0.xml file) or either adaptive value (minTimeStep)
	if (flow_h_max >= epsilon) // when there is flooding
	{
		// if the adaptive time-stepping is activated
		if (auto_dt_on == ON)
		{
			set_dt(&minTimeStep); // adaptive time-stepping.
		}
		else
		{
			set_dt(&dt_flood);  // to enable static time-stepping uncomment this line 
		}
	}

	// to track the simulation time
	//double new_sim_time = old_sim_time + minTimeStep; //commented to load from 0.xml
	double new_sim_time = old_sim_time + old_dt;

	//set_dt(&minTimeStep); //commented to load from 0.xml
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
	int count_dead = 0;
	int count_alive_dry = 0;
	int count_alive_run = 0;
	int count_alive_walk = 0;
	int count_alive_still = 0;
	int count_heros = 0;

	// loop over the number of pedestrians
	for (int index = 0 ; index < no_pedestrians; index++)
	{
		int pedestrians_state = get_agent_default_variable_state(index);
		
		// counting the number of pedestrians with different states
		if (pedestrians_state == STATE_DEAD)
		{
			count_dead++;
		}
		else if (pedestrians_state == STATE_ALIVE_DRY)
		{
			count_alive_dry++;
		}
		else if (pedestrians_state == STATE_ALIVE_RUN)
		{
			count_alive_run++;
		}
		else if (pedestrians_state == STATE_ALIVE_WALK)
		{
			count_alive_walk++;
		}
		else if (pedestrians_state == STATE_ALIVE_STILL)
		{
			count_alive_still++;
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

	for (int index = 0; index < no_FloodCells; index++)
	{
		// Loading water flow info from navmap cells
		double flow_h = get_FloodCell_Default_variable_h(index);
		
		double flow_discharge_x = get_FloodCell_Default_variable_qx(index);
		double flow_discharge_y = get_FloodCell_Default_variable_qy(index);

		//calculating the velocity of water
		double flow_velocity_x = flow_discharge_x / flow_h;
		double flow_velocity_y = flow_discharge_y / flow_h;

		double flow_veloxity_xy = max(flow_velocity_x, flow_velocity_y);    // taking maximum discharge between both x and y direction
		double flow_discharge_xy = max(flow_discharge_x, flow_discharge_y); // taking maximum velocity between both x and y direction
		
		
		// taking maximum velocity between all the agents
		if (flow_veloxity_xy > flow_velocity_max)
		{
			flow_velocity_max = flow_veloxity_xy;
		}
		
		if (flow_discharge_xy > flow_qxy_max)
		{
			flow_qxy_max = flow_discharge_xy;
		}

	}

	// calculatin hazard rating for emergency alarm status
	double HR = flow_h_max * (flow_velocity_max + 0.5);
	
	// Defining emergency alarm based on maximum velocity and height of water based on EA's guidence document
	// decide the emergency alarm after the first iteration (in whicn emergency alarm is set to NO_ALARM)
	//if (evacuation_stat == ON)
	//{
		if (emergency_alarm == NO_ALARM) //
		{
			if (HR == 0.0f)
			{
				emergency_alarm = NO_ALARM;
			}
			else if (HR <= 0.75)
			{
				emergency_alarm = LOW_FLOOD;
			}
			else if (HR > 0.75 && HR <= 1.25)
			{
				emergency_alarm = MODERATE_FLOOD;
			}
			else if (HR > 1.25 && HR <= 2.5)
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
			if (HR > 0.75 && HR <= 1.25)
			{
				emergency_alarm = MODERATE_FLOOD;
			}
			else if (HR > 1.25 && HR <= 2.5)
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
			if (HR > 1.25 && HR <= 2.5)
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

	// To assign zero values to the constant variables governing the emission rate and the probablity of exits
	float dont_emit = 0.0f;
	int dont_exit = 0;
	int do_exit = 10;
	
	// Changing the probability and emission // has to be applied for 7 Exits available in the model (number of exits can be extended, 
	// but needs further compilation, after generating the inputs for pedestrians, the flood model needs to be modified to operate on the
	// the desired number of exits)
	//if (evacuation_stat == ON)
//	{
		if (emergency_alarm > NO_ALARM) // greater than zero
		{
			// Stop emission of pedestian when emergency siren is issued
			set_EMMISION_RATE_EXIT1(&dont_emit);
			set_EMMISION_RATE_EXIT2(&dont_emit);
			set_EMMISION_RATE_EXIT3(&dont_emit);
			set_EMMISION_RATE_EXIT4(&dont_emit);
			set_EMMISION_RATE_EXIT5(&dont_emit);
			set_EMMISION_RATE_EXIT6(&dont_emit);
			set_EMMISION_RATE_EXIT7(&dont_emit);
			set_EMMISION_RATE_EXIT8(&dont_emit);
			set_EMMISION_RATE_EXIT9(&dont_emit);
			set_EMMISION_RATE_EXIT10(&dont_emit);

			// it is ONLY effective when new pedestrians are produced during the flooding (in case less emission rate is used)
			// Kept here in purpose to keep this capability
			if (emergency_exit_number == 1)
			{
				set_EXIT1_PROBABILITY(&do_exit);

				set_EXIT2_PROBABILITY(&dont_exit);
				set_EXIT3_PROBABILITY(&dont_exit);
				set_EXIT4_PROBABILITY(&dont_exit);
				set_EXIT5_PROBABILITY(&dont_exit);
				set_EXIT6_PROBABILITY(&dont_exit);
				set_EXIT7_PROBABILITY(&dont_exit);
				set_EXIT8_PROBABILITY(&dont_exit);
				set_EXIT9_PROBABILITY(&dont_exit);
				set_EXIT10_PROBABILITY(&dont_exit);

			}
			else
				if (emergency_exit_number == 2)
				{
					set_EXIT2_PROBABILITY(&do_exit);

					set_EXIT1_PROBABILITY(&dont_exit);
					set_EXIT3_PROBABILITY(&dont_exit);
					set_EXIT4_PROBABILITY(&dont_exit);
					set_EXIT5_PROBABILITY(&dont_exit);
					set_EXIT6_PROBABILITY(&dont_exit);
					set_EXIT7_PROBABILITY(&dont_exit);
					set_EXIT8_PROBABILITY(&dont_exit);
					set_EXIT9_PROBABILITY(&dont_exit);
					set_EXIT10_PROBABILITY(&dont_exit);

				}
				else
					if (emergency_exit_number == 3)
					{
						set_EXIT3_PROBABILITY(&do_exit);

						set_EXIT1_PROBABILITY(&dont_exit);
						set_EXIT2_PROBABILITY(&dont_exit);
						set_EXIT4_PROBABILITY(&dont_exit);
						set_EXIT5_PROBABILITY(&dont_exit);
						set_EXIT6_PROBABILITY(&dont_exit);
						set_EXIT7_PROBABILITY(&dont_exit);
						set_EXIT8_PROBABILITY(&dont_exit);
						set_EXIT9_PROBABILITY(&dont_exit);
						set_EXIT10_PROBABILITY(&dont_exit);

					}
					else
						if (emergency_exit_number == 4)
						{
							set_EXIT4_PROBABILITY(&do_exit);

							set_EXIT1_PROBABILITY(&dont_exit);
							set_EXIT2_PROBABILITY(&dont_exit);
							set_EXIT3_PROBABILITY(&dont_exit);
							set_EXIT5_PROBABILITY(&dont_exit);
							set_EXIT6_PROBABILITY(&dont_exit);
							set_EXIT7_PROBABILITY(&dont_exit);
							set_EXIT8_PROBABILITY(&dont_exit);
							set_EXIT9_PROBABILITY(&dont_exit);
							set_EXIT10_PROBABILITY(&dont_exit);

						}
						else
							if (emergency_exit_number == 5)
							{
								set_EXIT5_PROBABILITY(&do_exit);

								set_EXIT1_PROBABILITY(&dont_exit);
								set_EXIT2_PROBABILITY(&dont_exit);
								set_EXIT3_PROBABILITY(&dont_exit);
								set_EXIT4_PROBABILITY(&dont_exit);
								set_EXIT6_PROBABILITY(&dont_exit);
								set_EXIT7_PROBABILITY(&dont_exit);
								set_EXIT8_PROBABILITY(&dont_exit);
								set_EXIT9_PROBABILITY(&dont_exit);
								set_EXIT10_PROBABILITY(&dont_exit);

							}
							else
								if (emergency_exit_number == 6)
								{
									set_EXIT6_PROBABILITY(&do_exit);

									set_EXIT1_PROBABILITY(&dont_exit);
									set_EXIT2_PROBABILITY(&dont_exit);
									set_EXIT3_PROBABILITY(&dont_exit);
									set_EXIT4_PROBABILITY(&dont_exit);
									set_EXIT5_PROBABILITY(&dont_exit);
									set_EXIT7_PROBABILITY(&dont_exit);
									set_EXIT8_PROBABILITY(&dont_exit);
									set_EXIT9_PROBABILITY(&dont_exit);
									set_EXIT10_PROBABILITY(&dont_exit);

								}
								else
									if (emergency_exit_number == 7)
									{
										set_EXIT7_PROBABILITY(&do_exit);

										set_EXIT1_PROBABILITY(&dont_exit);
										set_EXIT2_PROBABILITY(&dont_exit);
										set_EXIT3_PROBABILITY(&dont_exit);
										set_EXIT4_PROBABILITY(&dont_exit);
										set_EXIT5_PROBABILITY(&dont_exit);
										set_EXIT6_PROBABILITY(&dont_exit);
										set_EXIT8_PROBABILITY(&dont_exit);
										set_EXIT9_PROBABILITY(&dont_exit);
										set_EXIT10_PROBABILITY(&dont_exit);

									}
									else
										if (emergency_exit_number == 8)
										{
											set_EXIT8_PROBABILITY(&do_exit);

											set_EXIT1_PROBABILITY(&dont_exit);
											set_EXIT2_PROBABILITY(&dont_exit);
											set_EXIT3_PROBABILITY(&dont_exit);
											set_EXIT4_PROBABILITY(&dont_exit);
											set_EXIT5_PROBABILITY(&dont_exit);
											set_EXIT6_PROBABILITY(&dont_exit);
											set_EXIT7_PROBABILITY(&dont_exit);
											set_EXIT9_PROBABILITY(&dont_exit);
											set_EXIT10_PROBABILITY(&dont_exit);

										}
										else
											if (emergency_exit_number == 9)
											{
												set_EXIT9_PROBABILITY(&do_exit);

												set_EXIT1_PROBABILITY(&dont_exit);
												set_EXIT2_PROBABILITY(&dont_exit);
												set_EXIT3_PROBABILITY(&dont_exit);
												set_EXIT4_PROBABILITY(&dont_exit);
												set_EXIT5_PROBABILITY(&dont_exit);
												set_EXIT6_PROBABILITY(&dont_exit);
												set_EXIT7_PROBABILITY(&dont_exit);
												set_EXIT8_PROBABILITY(&dont_exit);
												set_EXIT10_PROBABILITY(&dont_exit);

											}
											else
												if (emergency_exit_number == 10)
												{
													set_EXIT10_PROBABILITY(&do_exit);

													set_EXIT1_PROBABILITY(&dont_exit);
													set_EXIT2_PROBABILITY(&dont_exit);
													set_EXIT3_PROBABILITY(&dont_exit);
													set_EXIT4_PROBABILITY(&dont_exit);
													set_EXIT5_PROBABILITY(&dont_exit);
													set_EXIT6_PROBABILITY(&dont_exit);
													set_EXIT7_PROBABILITY(&dont_exit);
													set_EXIT8_PROBABILITY(&dont_exit);
													set_EXIT9_PROBABILITY(&dont_exit);

												}

		}
	//}
	

	int fill_cap = *get_fill_cap();
	double dxl = *get_DXL();
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


	int update_stopper = *get_update_stopper();
	int stop_on = ON;
	int stop_off = OFF;
	// increment the number of layers once the previous layer is applied, 
	//		the stopper updates only once it reaches to the length of dike and prevents keeping on incrementing 
	//		for the navmap being updated by more sandbags up to the fill_dke
	if (extended_length == rescaled_dike_length && update_stopper == OFF)
	{
		sandbag_layers++; 

		set_update_stopper(&stop_on);
	}
	else if (extended_length < rescaled_dike_length) // restarting the value of stopper for subsequent iteration to met the requirement of first conditional statement
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
	float dike_height		= *get_dike_height();

	if ((layers_applied * sandbag_height) >= dike_height)
	{
		set_sandbagging_on(&stop_off);
	}

	
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
	if (count_dead > max_dead)
		set_max_dead(&count_dead);
	if (count_alive_run > max_alive_wet)
		set_max_alive_wet(&count_alive_run);
	if (count_alive_walk > max_alive_walk)
		set_max_alive_walk(&count_alive_walk);
	if (count_alive_still > max_alive_stuck)
		set_max_alive_stuck(&count_alive_still);
	if (flow_h_max > max_depth)
		set_max_depth(&flow_h_max);
	if (flow_velocity_max > max_velocity)
		set_max_velocity(&flow_velocity_max);
	

	//printing the simulation time
	float peak_time = *get_DISCHARGE_PEAK_TIME();

	if (round(new_sim_time) == round(peak_time) )
	{
		printf("\n\n\n***********************AT PEAK TIME**********************************\n");
		printf("***********************AT PEAK TIME***************************************************************\n");
	}

	printf("\n\n\n**************************************************************************************\n");
	printf("**************************************************************************************\n");
	printf("\nElapsed simulation time = %.3f seconds\tOR\t%.2f minutes\tOR\t%.2f Hours \n", new_sim_time, new_sim_time/60, new_sim_time/3600); //prints simulation time 
	//printing the total number of pedestrians in each iteration
	printf("\n****************************** Pedestrian information *******************************\n");
	printf("total number of pedestrians = %d \n", no_pedestrians);
	//printing the number of hero pedestrians in each iteration
	printf("number of hero pedestrians  = %d \n", count_heros);
	printf("\n****************************** States of pedestrians *******************************\n");
	//printing the number of dead pedestrians in each iteration
	printf("\nnumber of DEAD pedestrians = %d \n", count_dead);
	//printing the number of pedestrians in dry zones in each iteration
	printf("\nnumber of ALIVE pedestrians in DRY zones	= %d \n", count_alive_dry);
	//printing the number of alive pedestrians in wet area running to safe haven in each iteration
	printf("\nnumber of ALIVE pedestrians in wet area who can still RUN = %d \n", count_alive_run);
	//printing the number of dead pedestrians in wet area walking to the safe haven in each iteration
	printf("\nnumber of ALIVE pedestrians in wet area who can only WALK = %d \n", count_alive_walk);
	//printing the number of dead pedestrians got stuck into water in each iteration
	printf("\nnumber of ALIVE pedestrians in wet area STUCK in water	  = %d \n", count_alive_still);

	//printing the number of dead pedestrians got stuck into water in each iteration
	printf("\nMaximum number of people DEAD so far	  = %d \n", max_dead);
	printf("\nMaximum number of people in WET area and could still RUN so far	= %d \n", max_alive_wet);
	printf("\nMaximum number of people in WET area and could only WALK so far	= %d \n", max_alive_walk);
	printf("\nMaximum number of people in WET area and GOT STUCK so far	  = %d \n", max_alive_stuck);

	//printing the maximum height of water
	printf("\n****************************** Flood water information *******************************\n");
	printf("\nThe severity of the flood    = %d \n", emergency_alarm);
	printf("\nThe Hazard Rating (HR) is = %.3f \n", HR);
	printf("\nThe maximum reached HR is = %.3f \n", HR_max);

	printf("\nmaximum depth of water     = %.3f  m\n", flow_h_max);
	printf("\nmaximum discharge of water = %.3f  m2/s \n", flow_qxy_max);
	printf("\nmaximum velocity of water  = %.3f  m/s \n", flow_velocity_max);

	printf("\nmaximum reached depth of water so far = %.3f  m \n", max_depth);
	printf("\nmaximum reached velocity of water  = %.3f  m/s \n", max_velocity);
	
	printf("\n****************************** Sandbagging information *******************************\n");
	printf("\n the total number of sandbags is =%d \n", total_sandbags);
	printf("\n number of applied layers so far =%d \n", layers_applied);
	printf("\n extended length of sandbags is  =%.3f \n", extended_length);

	//printf("\n the number of put sandbag layers is =  %d \n", sandbag_layers);
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
	int half_x = 0.5*xmax ;
	int half_y = 0.5*ymax ;

	float x_rescaled = (( x / half_x) - 1);
	float y_rescaled = (( y / half_y) - 1);

	return make_float2(x_rescaled, y_rescaled);
}


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
	if ((INFLOW_BOUNDARY == NORTH) && (sim_time >= FLOOD_START_TIME) && (sim_time <= FLOOD_END_TIME)) // flood end time prevents extra water depth when not needed
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
	else if ((INFLOW_BOUNDARY == EAST) && (sim_time >= FLOOD_START_TIME)) // provide water depth exactly before the when the flood starts
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
	else if ((INFLOW_BOUNDARY == SOUTH) && (sim_time >= FLOOD_START_TIME))
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
	else if ((INFLOW_BOUNDARY == WEST) && (sim_time >= FLOOD_START_TIME))
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

	if (time <= FLOOD_START_TIME)
	{
		q_t = DISCHARGE_INITIAL;
	}
	else if ((time > FLOOD_START_TIME) && (time <= DISCHARGE_PEAK_TIME))
	{
		q_t = (((DISCHARGE_PEAK - DISCHARGE_INITIAL) / (DISCHARGE_PEAK_TIME - FLOOD_START_TIME))*(time - FLOOD_START_TIME)) + DISCHARGE_INITIAL;
	}
	else if ((time > DISCHARGE_PEAK_TIME) && (time <= FLOOD_END_TIME))
	{
		q_t = (((DISCHARGE_INITIAL - DISCHARGE_PEAK) / (FLOOD_END_TIME - DISCHARGE_PEAK_TIME))*(time - DISCHARGE_PEAK_TIME)) + DISCHARGE_PEAK;
	}
	else
	{
		q_t = DISCHARGE_END;
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

	// if the option of body effect on water is activated then remove the topography in each iteration
	if (body_effect_on == ON)
	{
		if (msg->z0 == 1.76)
		{
			agent->z0 = 0;
		}
		else
		{
			agent->z0 = msg->z0;
		}
	}
	else // if the option of body effect is not activated, still take the topography equal to that of flood model
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
		if ((separation < MESSAGE_RADIUS)&&(separation>MIN_DISTANCE)){
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


	double spent_dropping;
	double spent_picking;
	
	// activate only when pickup time is not zero (agent has picked up a sandbag)
	if (agent->pickup_time > 0.0f)
	{
		spent_picking = agent->pickup_time + pickup_duration;
	}

	if (agent->drop_time > 0.0f)
	{
		spent_dropping = agent->drop_time + drop_duration;
	}

	// directing hero agents between pickup and drop point
	// get the new exit_no when emergency alarm is issued, also guide the hero agents for picking up sandbags
	if (emer_alarm > NO_ALARM )
	{
		agent->exit_no = evacuation_exit_number;
		
		if (sandbagging_on == ON)
		{
			if (sim_time >= sandbagging_start_time )
			{
				if (sim_time <= sandbagging_end_time)
				{
					if (agent->hero_status == 1 && agent->pickup_time == 0.0f && agent->drop_time == 0.0f) // shows hero is in pickup stage
					{
						agent->exit_no = pickup_point;

						if (exit_location == pickup_point)
						{
							agent->pickup_time = sim_time;
						}

					}
					else if (agent->hero_status == 1 && agent->pickup_time != 0.0f && agent->drop_time == 0.0f) // shows hero is in pickup stage
					{
						agent->exit_no = pickup_point;

						if (sim_time > spent_picking)
						{
							agent->exit_no = drop_point;
							
							agent->carry_sandbag = 1;

							if (exit_location == drop_point)
							{
								agent->drop_time = sim_time;
								agent->pickup_time = 0.0;
							}

						}
					}
					else if (agent->hero_status == 1 && agent->pickup_time == 0.0f && agent->drop_time != 0.0f)
					{
						agent->exit_no = drop_point ;
						agent->carry_sandbag = 0;
						
						if (sim_time > spent_dropping)
						{
							agent->drop_time   = 0.0;
							agent->pickup_time = 0.0;
						}
						
					}
					
				}
			}
		}
	} 


	//if (agent->hero_status == 1)
//	{
	//	printf(" pedestrian carrying a sandbag? = %d \n", agent->carry_sandbag);
	//	printf("The picking time of heros   = %f \n", agent->pickup_time);
	//	printf("The dropping time of heros  = %f \n", agent->drop_time);
	//	printf("The due time for picking up = %f \n", spent_picking);
	//	printf("The due time for dropping   = %f \n", spent_dropping);
	//	///printf(" x of pedestrian is  %f \n", agent->x);
	//	///printf(" x of navmap_cell is  %f \n", current_message->exit4_x);
	//	///printf("Difference of x %f \n", difference_x);
	//	///printf("Difference of y %f \n", difference_y);
	//	
	//	//printf("The duration of picked sadgbags= %f \n", picking_spent_time);
//	}
	//
	
	//goal force
	glm::vec2 goal_force;

	
		if (agent->exit_no == 1 )
		{
			goal_force = glm::vec2(current_message->exit0_x, current_message->exit0_y);
			if (exit_location == 1)
			{
				if (EXIT1_STATE == 1 && EXIT1_PROBABILITY != 0) //EXIT1_PROBABILITY != 0 prevents killing agents while flooding at the exits (for sandbagging test is added MS08102018)
					kill_agent = 1;
						
				else
					agent->exit_no = getNewExitLocation(rand48);
			}
		}
		else if (agent->exit_no == 2)
		{
			goal_force = glm::vec2(current_message->exit1_x, current_message->exit1_y);
			if (exit_location == 2)
				if (EXIT2_STATE == 1 && EXIT2_PROBABILITY != 0)
					kill_agent = 1;	
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 3 )
		{
			goal_force = glm::vec2(current_message->exit2_x, current_message->exit2_y);
			if (exit_location == 3)
				if (EXIT3_STATE == 1 && EXIT3_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 4 )
		{
			goal_force = glm::vec2(current_message->exit3_x, current_message->exit3_y);
			if (exit_location == 4)
				if (EXIT4_STATE == 1 && EXIT4_PROBABILITY != 0)
					kill_agent = 1;	
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 5)
		{
			goal_force = glm::vec2(current_message->exit4_x, current_message->exit4_y);
			if (exit_location == 5)
				if (EXIT5_STATE == 1 && EXIT5_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 6 )
		{
			goal_force = glm::vec2(current_message->exit5_x, current_message->exit5_y);
			if (exit_location == 6)
				if (EXIT6_STATE == 1 && EXIT6_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 7 )
		{
			goal_force = glm::vec2(current_message->exit6_x, current_message->exit6_y);
			if (exit_location == 7)
				if (EXIT7_STATE == 1 && EXIT7_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 8)
		{
			goal_force = glm::vec2(current_message->exit7_x, current_message->exit7_y);
			if (exit_location == 8)
				if (EXIT8_STATE == 1 && EXIT8_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 9)
		{
			goal_force = glm::vec2(current_message->exit8_x, current_message->exit8_y);
			if (exit_location == 9)
				if (EXIT9_STATE == 1 && EXIT9_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}
		else if (agent->exit_no == 10)
		{
			goal_force = glm::vec2(current_message->exit9_x, current_message->exit9_y);
			if (exit_location == 10)
				if (EXIT10_STATE == 1 && EXIT10_PROBABILITY != 0)
					kill_agent = 1;
				else
					agent->exit_no = getNewExitLocation(rand48);
		}


	//scale goal force
	goal_force *= GOAL_WEIGHT;

	agent->steer_x += collision_force.x + goal_force.x;
	agent->steer_y += collision_force.y + goal_force.y;

	//update height
	agent->height = current_message->height;

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
	
	// Changing the state of pedestrians with respect to water flow charaacteristics (based on EA 2015 guidence document and Dawsons work)

			if (agent->state != STATE_DEAD)
			{
					if (water_height <= epsilon && water_velocity <= epsilon)
					{
							agent->state = STATE_ALIVE_DRY;
					}
					else
					{
						if (water_height <= DANGEROUS_H && water_velocity <= DANGEROUS_V)
						{
							agent->state = STATE_ALIVE_RUN;
						}
						else if (water_height <= DANGEROUS_H && water_velocity > DANGEROUS_V)
						{
							agent->state = STATE_ALIVE_WALK;
						}
						else if (water_height > DANGEROUS_H && water_velocity <= DANGEROUS_V)
						{
							agent->state = STATE_ALIVE_STILL;
						}
						else if (water_height > DANGEROUS_H && water_velocity > DANGEROUS_V )
						{
							agent->state = STATE_DEAD;
						}
					}
					
				}

			//}

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
__FLAME_GPU_FUNC__ int move(xmachine_memory_agent* agent){

	glm::vec2 agent_pos = glm::vec2(agent->x, agent->y);
	
	//// for those pedestrian agents who are not able to move take zero velocity (I dont kill the pedestrians on 
	// the domain to save their exact numbers with their states, if killed, then there is no info about them)
	//// if the state is dead or alive but still (waiting for help) follow below
	if ((agent->state == STATE_DEAD) || (agent->state == STATE_ALIVE_STILL))
	{
		agent->velx = 0.0f;
		agent->vely = 0.0f;
	}
	
	glm::vec2 agent_vel = glm::vec2(agent->velx, agent->vely);

	glm::vec2 agent_steer = glm::vec2(agent->steer_x, agent->steer_y);

	float current_speed = length(agent_vel) + 0.025f;//(powf(length(agent_vel), 1.75f)*0.01f)+0.025f;;


    //apply more steer if speed is greater
	agent_vel += current_speed*agent_steer;
	float speed = length(agent_vel);
	//limit speed
	if (speed >= agent->speed){
		agent_vel = normalize(agent_vel)*agent->speed;
		speed = agent->speed;
	}

	//update position
	agent_pos += agent_vel*TIME_SCALER;

    
	//animation
	agent->animate += (agent->animate_dir * powf(speed,2.0f)*TIME_SCALER*100.0f);
	if (agent->animate >= 1)
		agent->animate_dir = -1;
	if (agent->animate <= 0)
		agent->animate_dir = 1;

	//lod
	agent->lod = 1;

	//update
	agent->x = agent_pos.x;
	agent->y = agent_pos.y;
	agent->velx = agent_vel.x;
	agent->vely = agent_vel.y;

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

		if (emit_agent){
			float x = ((agent->x+0.5f)/(d_message_navmap_cell_width/ENV_WIDTH))-ENV_MAX;
			float y = ((agent->y+0.5f)/(d_message_navmap_cell_width/ENV_WIDTH))-ENV_MAX;
			int exit = getNewExitLocation(rand48);
			float animate = rnd<DISCRETE_2D>(rand48);
			float speed = (rnd<DISCRETE_2D>(rand48))*0.5f + 1.0f;
			
			if (evacuation_on == ON && sandbagging_on == ON)
			{
				// limit the number of pedestrians to a certain number
				if (count_population < pedestrians_population)
				{
					// produce certain number of hero pedestrians with hero_status =1

					if (count_heros <= hero_population)
					{
						add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1, 1, 1, 0.0f, 0.0f, 0);
					}
					else
					{
						add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1, 1, 0, 0.0f, 0.0f, 0);
					}

				}
			}
			else
			{
				// limit the number of pedestrians to a certain number
				if (count_population < pedestrians_population)
				{			
						add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1, 1, 0, 0.0f, 0.0f, 0);

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
	float2 navmap_loc = FindRescaledGlobalPosition_navmap(agent, make_double2(0.0, 0.0));

	// Vector of agents' position
	glm::vec2 agent_pos = glm::vec2(navmap_loc.x, navmap_loc.y);

	// This function calculates the global position of each navmap based on the size of flood domain 
	//float2 navmap_loc_global = FindGlobalPosition_navmap(agent, make_double2(0.0, 0.0));
	
	//Loading a single message
	xmachine_message_PedData* msg = get_first_PedData_message(PedData_messages, partition_matrix, (float)navmap_loc.x, (float)navmap_loc.y, 0.0);

	//glm::vec2 drop_pos, pickup_pos;
	
	/*if (pickup_point == 1)
		pickup_pos = glm::vec2(agent->exit0_x, agent->exit0_y);
	if (drop_point == 1)
		drop_pos = glm::vec2(agent->exit0_x, agent->exit0_y);*/

	// get the exit location of navmap cell
	int exit_location = agent->exit_no;

	// load messages from pedestrian
	while (msg)	
	{
		// Check if the body effect option is activated. If so, take pedestrians as moving objects (obstacles for water propagation)
		if (body_effect_on == ON)
		{
			// To locate the position of each pedestrian within the grid of navmap agents
			glm::vec2 msg_pos = glm::vec2(msg->x, msg->y);
			float distance = length(agent_pos - msg_pos);
			if (distance < MESSAGE_RADIUS)
			{
				agent->z0 = 1.76; // average height of a man is considered  = 1.76 m
			}

		}
		

		if (evacuation_on == ON)
		{
			if (sim_time > sandbagging_start_time && sim_time < sandbagging_end_time)
			{
				if (exit_location == drop_point)
				{
					if ( msg->hero_status == ON)
					{

						if (msg->pickup_time == 0.0f && msg->drop_time != 0.0f && msg->carry_sandbag == 1) // condition showing the moment when sandbag is dropped
						{
							agent->sandbag_capacity++;
						}
					}
				}
			}
		}
			msg = get_next_PedData_message(msg, PedData_messages, partition_matrix);
	}


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
	if (agent->z0 < msg->z0 ) // Do not update if the floodCell agents has already a topgraphic feature (e.g. building data)
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
