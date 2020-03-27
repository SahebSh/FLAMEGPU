#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>    // <-- NOTE: to use with std::max and std:min

// NOTE: run it with visual studio. To compile with g++, run "g++ -std=c++11 xmlGen.cpp -o xmlGen"

// This function generate 0.XML file based on the data of flood simulation as well as global variables in the environment
// The output of this code is the input to the flood modelling in FLAME-GPU


double bed_data(double x_int, double y_int);

#define SIZE 128 
#define xmin 0
#define xmax 15
#define ymin 0
#define ymax 15

#define OFF 0
#define ON 1

int main()
{
	FILE *fp = fopen("flood_init.xml", "w"); // <-- NOTE: changed output.txt to 0.xml
	if (fp == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	// Model constant :  they are imported to the output file          
	//double       timestep = 0.5; // assigned Temporary 
	int          inDomain = 1;

	//Constant time-step (sec)
	// time-step of pedestrian model 
	double dt_ped = 0.0f;
	//time-step of flood model for static time-stepping
	double dt_flood = 0.0f; 

	// Activate/deactivate the options of the model
	int auto_dt_on		= OFF;	// adaptive time-stepping for flood model
	int body_effect_on	= OFF;	// taking pedestrians as moving obstacles
	int evacuation_on	= OFF;	// early evacuation of people
	int sandbagging_on	= OFF;	// Sandbagging procedure after early evacuation siren

	//Defining Pedestrian population, used as constrain to limit the number of pedestrians in the domain
	int pedestrians_population = 1000; 
	float hero_percentage = 0.5; // shows the percentage of people who are considered as sandbaggers

	//Defining the key times of the event (in second)
	double FLOOD_START_TIME			= 200;
	double DISCHARGE_PEAK_TIME		= 400;
	double FLOOD_END_TIME			= 600;
	double evacuation_start_time	= 400;// Evacuation time defines when the pedestrians start to evacuate the residential area (ini seconds)
	double sandbagging_start_time	= 400;
	double sandbagging_end_time		= 4200;
	double pickup_duration			= 30;
	double drop_duration			= 30;

	// Define the initial discharge detail at the inflow boundary
	double DISCHARGE_INITIAL	= 0.0f;	// (unit: m2/s)
	double DISCHARGE_PEAK		= 1.3f;
	double DISCHARGE_END		= 0.0f;

	// BOUNDARY BEHAVIOUR: 
	//This variable where the inflow boundary is (possible values: NORTH:1  EAST:2  SOUTH:3  WEST:4)
	int INFLOW_BOUNDARY = 3;
	// These variables shows whether the bounadry is transmissive or reflective (possible values: Transmissive:1  Reflective:2)
	//Note: this is a must to take the inflow boundary aas trasmissive
	int BOUNDARY_EAST_STATUS  = 2;
	int BOUNDARY_WEST_STATUS  = 2;
	int BOUNDARY_NORTH_STATUS = 1;
	int BOUNDARY_SOUTH_STATUS = 2;

	// the location of inflow boundary (in metre):
	double x1_boundary = 110; // location of the start point in x-axis direction
	double x2_boundary = 190;// location of the end point in x-axis direction
	double y1_boundary = 40;// location of the start point in y-axis direction
	double y2_boundary = 120;// location of the end point in y-axis direction
	// initial water depth at the boundary to support discharge at the start of flooding
	double init_depth_boundary = 0.1;

	// Sandbag detail (in metre)
	// The most common sizes for sandbags are 14 by 26 inches (36 by 66 cm) to 17 by 32 inches (43 by 81 cm)
	// The size of Filled sandbag dimensions (default values): length 50cm, width 25cm, height 10cm (all dimensions are approximate) and Each sandbag contains approx. 15kg of sand
	float sandbag_length = 0.50f;
	float sandbag_height = 0.10f;
	float sandbag_width  = 0.25f;

	// the details of proposed sandbag dike (in metre)
	float dike_length	 = 120.0f;
	float dike_height	 = 0.5f;
	float dike_width	 = 3.0f; // this is not considered to be taken into account YET (for the current model)

	// Problem dependent variable that shows the number of the exit which will be used for pedestrian to evacuate the place
	// NOTE: for better undestranding of the location of each exit and their number please see ~/iterations/init_flood.jpg
	int evacuation_exit_number = 6;

	// the goal point where hero pedestrians can find sandbags
	int pickup_point = 5;

	// the goal point where hero pedestrians are expected to put their already picked-up sandbags in pickup_point
	int drop_point = 7;

	// The length of the domain
	double lx, ly; // <-- NOTE: variable names start with small letters, read more on https://www.programiz.com/c-programming/c-variables-constants

	// size of the domain
	double dx, dy;

	// iteration integers
	int i, j;

	auto z0_int = new double[SIZE + 1][SIZE + 1]();
	auto x_int = new double[SIZE + 1]();
	auto y_int = new double[SIZE + 1]();

	auto z0 = new double[SIZE][SIZE]();

	auto x = new double[SIZE]();
	auto y = new double[SIZE]();


	// initial flow rate
	double qx_initial = 0.0;
	double qy_initial = 0.0;

	// Mesh-grid propertise
	lx = xmax - xmin;
	ly = ymax - ymin;
	dx = (double)lx / (double)SIZE;
	dy = (double)ly / (double)SIZE;

	FILE *fp2 = fopen("init_topo.txt", "w");
	if (fp2 == NULL){
		printf("Error opening file!\n");
		exit(1);
	}

	fprintf(fp, "<states>\n");
	fprintf(fp, "<itno>0</itno>\n");
	fprintf(fp, " <environment>\n\n");
	//domain info
	fprintf(fp, "  <xmin>%d</xmin>\n", xmin);
	fprintf(fp, "  <xmax>%d</xmax>\n", xmax);
	fprintf(fp, "  <ymin>%d</ymin>\n", ymin);
	fprintf(fp, "  <ymax>%d</ymax>\n\n", ymax);
	//time-step:
	fprintf(fp, "  <dt_ped>%f</dt_ped>\n", dt_ped);
	fprintf(fp, "  <dt_flood>%f</dt_flood>\n\n", dt_flood);
	//Turn on/off buttons for the activation/deactivation of options provided in the model
	fprintf(fp, "  <auto_dt_on>%d</auto_dt_on>\n", auto_dt_on);
	fprintf(fp, "  <body_effect_on>%d</body_effect_on>\n", body_effect_on);
	fprintf(fp, "  <evacuation_on>%d</evacuation_on>\n", evacuation_on);
	fprintf(fp, "  <sandbagging_on>%d</sandbagging_on>\n\n", sandbagging_on);
	// pedestrian population and percentage of sandbaggers
	fprintf(fp, "  <pedestrians_population>%d</pedestrians_population>\n", pedestrians_population);// shows which exit is going to use for evacuation
	fprintf(fp, "  <hero_percentage>%f</hero_percentage>\n\n", hero_percentage);// shows which exit is going to use for evacuation
	//Key times
	fprintf(fp, "  <FLOOD_START_TIME>%f</FLOOD_START_TIME>\n", FLOOD_START_TIME);
	fprintf(fp, "  <DISCHARGE_PEAK_TIME>%f</DISCHARGE_PEAK_TIME>\n", DISCHARGE_PEAK_TIME);
	fprintf(fp, "  <FLOOD_END_TIME>%f</FLOOD_END_TIME>\n", FLOOD_END_TIME);
	fprintf(fp, "  <evacuation_start_time>%f</evacuation_start_time>\n", evacuation_start_time);// the start of the evacuation procedure
	fprintf(fp, "  <sandbagging_start_time>%f</sandbagging_start_time>\n", sandbagging_start_time);// the start of the evacuation procedure
	fprintf(fp, "  <sandbagging_end_time>%f</sandbagging_end_time>\n", sandbagging_end_time);// the start of the evacuation procedure
	fprintf(fp, "  <pickup_duration>%f</pickup_duration>\n", pickup_duration);// the start of the evacuation procedure
	fprintf(fp, "  <drop_duration>%f</drop_duration>\n\n", drop_duration);// the start of the evacuation procedure
	//flood discharge info
	fprintf(fp, "  <DISCHARGE_INITIAL>%f</DISCHARGE_INITIAL>\n", DISCHARGE_INITIAL);
	fprintf(fp, "  <DISCHARGE_PEAK>%f</DISCHARGE_PEAK>\n", DISCHARGE_PEAK);
	fprintf(fp, "  <DISCHARGE_END>%f</DISCHARGE_END>\n\n", DISCHARGE_END);
	//boundary behaviour info
	fprintf(fp, "  <INFLOW_BOUNDARY>%d</INFLOW_BOUNDARY>\n", INFLOW_BOUNDARY);
	fprintf(fp, "  <BOUNDARY_EAST_STATUS>%d</BOUNDARY_EAST_STATUS>\n", BOUNDARY_EAST_STATUS);
	fprintf(fp, "  <BOUNDARY_WEST_STATUS>%d</BOUNDARY_WEST_STATUS>\n", BOUNDARY_WEST_STATUS);
	fprintf(fp, "  <BOUNDARY_NORTH_STATUS>%d</BOUNDARY_NORTH_STATUS>\n", BOUNDARY_NORTH_STATUS);
	fprintf(fp, "  <BOUNDARY_SOUTH_STATUS>%d</BOUNDARY_SOUTH_STATUS>\n\n", BOUNDARY_SOUTH_STATUS);
	//inflow boundary location
	fprintf(fp, "  <x1_boundary>%f</x1_boundary>\n", x1_boundary);
	fprintf(fp, "  <x2_boundary>%f</x2_boundary>\n", x2_boundary);
	fprintf(fp, "  <y1_boundary>%f</y1_boundary>\n", y1_boundary);
	fprintf(fp, "  <y2_boundary>%f</y2_boundary>\n\n", y2_boundary);
	//initial depth of water at the start of flooding time (to support discharge)
	fprintf(fp, "  <init_depth_boundary>%f</init_depth_boundary>\n\n", init_depth_boundary);
	// sandbagging info
	fprintf(fp, "  <sandbag_length>%f</sandbag_length>\n", sandbag_length);
	fprintf(fp, "  <sandbag_height>%f</sandbag_height>\n", sandbag_height);
	fprintf(fp, "  <sandbag_width>%f</sandbag_width>\n", sandbag_width);
	fprintf(fp, "  <dike_length>%f</dike_length>\n", dike_length);
	fprintf(fp, "  <dike_height>%f</dike_height>\n", dike_height);
	fprintf(fp, "  <dike_width>%f</dike_width>\n\n", dike_width);

	// shows which exit is going to use for evacuation
	fprintf(fp, "  <evacuation_exit_number>%d</evacuation_exit_number>\n\n", evacuation_exit_number);
	// shows where hero pedestrians can pickup sandbags
	fprintf(fp, "  <pickup_point>%d</pickup_point>\n", pickup_point);
	// shows where hero pedestrians are expected to drop sandbags
	fprintf(fp, "  <drop_point>%d</drop_point>\n\n", drop_point);

	fprintf(fp, " </environment>\n\n");


	// NOTE: array index starts from zero. For more info on why, read this --> http://developeronline.blogspot.co.uk/2008/04/why-array-index-should-start-from-0.html
	for (i = 0; i < SIZE + 1; i++){
		for (j = 0; j < SIZE + 1; j++){


			x_int[i] = xmin + i  * dx;
			y_int[j] = ymin + j  * dy;

			z0_int[i][j] = bed_data((double)x_int[i], (double)y_int[j]);

		}
	}

	for (i = 0; i < SIZE; i++)	// changed by MS17Nov2017  			
	for (j = 0; j< SIZE; j++)

	{
		{
			x[i] = 0.5 * (x_int[i] + x_int[i + 1]); //
			y[j] = 0.5 * (y_int[j] + y_int[j + 1]);

			z0[i][j] = (z0_int[i][j] + z0_int[i + 1][j] + z0_int[i][j + 1] + z0_int[i + 1][j + 1]) / 4;

			fprintf(fp2, "%d\t\t %d\t\t %f \n", i, j, z0[i][j]);


			fprintf(fp, " <xagent>\n");
			fprintf(fp, "\t<name>FloodCell</name>\n");

			fprintf(fp, "\t<inDomain>%d</inDomain>\n", inDomain);
			fprintf(fp, "\t<x>%d</x>\n", i); // +1 can be added to to output 128 * 128, not 127 * 127 / since FLAME-GPU can read x = 0 that is completely alright
			fprintf(fp, "\t<y>%d</y>\n", j);
			fprintf(fp, "\t<z0>%f</z0>\n", z0[i][j]);

			fprintf(fp, " </xagent>\n");
		}
	}

	fprintf(fp, "</states>");
	fclose(fp);
	return 0;

}
 

/* Function to generate the terrain detail - Three Humps*/
double bed_data(double x_int, double y_int)
{
	// This function generates Three Humps terrain detail in the model

	double zz;
	double flume_length = 1; // in meters

	// defining an open channel in the centre of x-axis direction
	
	double x1 = (xmax - flume_length) / 2 ;
	double x2 = (xmax + flume_length) / 2 ; 

	if ((x_int <= x1) || (x_int >= x2))
		zz = 10;
	else
		zz = 0;

	

	return zz;
}