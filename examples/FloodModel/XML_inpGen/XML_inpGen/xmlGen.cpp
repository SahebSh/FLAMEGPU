#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>    // <-- NOTE: to use with std::max and std:min

// NOTE: run it with visual studio. To compile with g++, run "g++ -std=c++11 xmlGen.cpp -o xmlGen"

// This function generate 0.XML file based on the data of three humps sulution (FV)
// The output of this code is the input to the flood modelling in FLAME-GPU


double bed_data(double x_int, double y_int);
double initial_flow(double x_int, double y_int, double z0_int);

#define SIZE 128 

int main()
{
	FILE *fp = fopen("0.xml", "w"); // <-- NOTE: changed output.txt to 0.xml
	if (fp == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	// Model constant :  they are imported to the output file          

	//double       timestep = 0.5; // assigned Temporary 
	int          inDomain = 1;

	// Specifying the size of domain  
	 
	// Case 1: Radial
//	int xmin = 0;// Radial = 0
//	int xmax = 40;//;Radial = 40
//	int ymin = 0;//Radial = 0
//	int ymax = 40;//Radial = 40
	
	// Case 2: Three Humps
	int xmin = 0;// Radial = 0
	int xmax = 75;//;Radial = 40
	int ymin = 0;//Radial = 0
	int ymax = 30;//Radial = 40

	//********************** This is to specify the number of agents, which is supposed to be square ********************* 
	//int nx = 128;//256;
	//int ny = 128;//256;
	//********************************************************************************************************************   

	// The length of the domain
	double lx, ly; // <-- NOTE: variable names start with small letters, read more on https://www.programiz.com/c-programming/c-variables-constants

	// size of the domain
	double dx, dy;

	// iteration integers
	int i, j;

	/*Initial variables, to be printed*/
	// NOTE: allocating/ initialising dynamic arrays. auto is permitted in C++11.
	auto qx_int = new double[SIZE + 1][SIZE + 1]();
	auto qy_int = new double[SIZE + 1][SIZE + 1]();
	auto h_int 	= new double[SIZE + 1][SIZE + 1]();
	auto z0_int = new double[SIZE + 1][SIZE + 1]();
	auto x_int 	= new double[SIZE + 1]();
	auto y_int 	= new double[SIZE + 1]();

	auto qx = new double[SIZE][SIZE]();
	auto qy = new double[SIZE][SIZE]();
	auto h 	= new double[SIZE][SIZE]();
	auto z0 = new double[SIZE][SIZE]();

	auto x = new double[SIZE]();
	auto y = new double[SIZE]();

	double hFace_E = 0.0, etFace_E = 0.0, qxFace_E = 0.0, qyFace_E = 0.0;
	double hFace_W = 0.0, etFace_W = 0.0, qxFace_W = 0.0, qyFace_W = 0.0;
	double hFace_N = 0.0, etFace_N = 0.0, qxFace_N = 0.0, qyFace_N = 0.0;
	double hFace_S = 0.0, etFace_S = 0.0, qxFace_S = 0.0, qyFace_S = 0.0;


	// initial flow rate
	double qx_initial = 0.0;
	double qy_initial = 0.0;

	// Mesh-grid propertise
	lx = xmax - xmin;
	ly = ymax - ymin;
	dx = (double)lx / (double)SIZE;
	dy = (double)ly / (double)SIZE;
	
	FILE *fp2 = fopen("InitialData.txt", "w");
    if (fp2 == NULL){
       printf("Error opening file!\n");
    exit(1);
    }

	fprintf(fp, "<states>\n");
	fprintf(fp, "<itno>0</itno>\n");
	//    fprintf(fp," <environment>\n"); 
	////    fprintf(fp,"  <timestep>%f</timestep>\n",timestep);
	////    fprintf(fp,"  <dxl>%f</dxl>\n",dxl);
	////    fprintf(fp,"  <dyl>%f</dyl>\n",dyl);
	//    fprintf(fp," </environment>\n");


	// NOTE: array index starts from zero. For more info on why, read this --> http://developeronline.blogspot.co.uk/2008/04/why-array-index-should-start-from-0.html
	for (i = 0; i < SIZE + 1; i++){
		for (j = 0; j < SIZE + 1; j++){
		

			x_int[i] = xmin + i  * dx;
			y_int[j] = ymin + j  * dy;

			z0_int[i][j] = bed_data((double)x_int[i], (double)y_int[j]);
			h_int[i][j]  = initial_flow((double)x_int[i], (double)y_int[j], (double)z0_int[i][j]);
			
			qx_int[i][j] = 0;//qx_initial; // Temporary assigned value
			qy_int[i][j] = 0;//qy_initial; // Temporary assigned value (However it should be 0 )
		}
	}

//	for (i = 0; i < SIZE; i++){
//		for (j = 0; j < SIZE; j++){

//			x[i] = 0.5*(x_int[i] + x_int[i + 1]);
//			y[j] = 0.5*(y_int[j] + y_int[j + 1]);

//			z0[i][j] = 0.25*(z0_int[i][j + 1] + z0_int[i][j] + z0_int[i + 1][j] + z0_int[i + 1][j + 1]);
	//		h[i][j] = 0.25*(h_int[i][j + 1] + h_int[i][j] + h_int[i + 1][j] + h_int[i + 1][j + 1]);

	//		qx[i][j] = 0.25*(qx_int[i][j + 1] + qx_int[i][j] + qx_int[i + 1][j] + qx_int[i + 1][j + 1]);
	//		qy[i][j] = 0.25*(qy_int[i][j + 1] + qy_int[i][j] + qy_int[i + 1][j] + qy_int[i + 1][j + 1]);
			
//			for ( i=1 ; i < SIZE + 1 ; i++)	// changed by MS17Nov2017  			
//				for (j=1 ; j< SIZE + 1; j++)
			for ( i=0 ; i < SIZE  ; i++)	// changed by MS17Nov2017  			
				for (j=0 ; j< SIZE ; j++)
				
					{
						{
							x[i]  = 0.5 * ( x_int[i] +  x_int[i+1]); //
							y[j]  = 0.5 * ( y_int[j] +  y_int[j+1]);
							
//										z0[i][j] = bed_data((double)x[i], (double)y[j]);
//										h[i][j]  = initial_flow((double)x[i], (double)y[j], (double)z0[i][j]);
//										qx[i][j] = qx_initial;
//										qy[i][j] = qy_initial;
    
			                z0[i][j] =  (z0_int[i][j] + z0_int[i+1][j] + z0_int[i][j+1] + z0_int[i+1][j+1])/4;
			                h[i][j]  =  ( h_int[i][j] +  h_int[i+1][j] +  h_int[i][j+1] +  h_int[i+1][j+1])/4;
			                qx[i][j] =  (qx_int[i][j] + qx_int[i+1][j] + qx_int[i][j+1] + qx_int[i+1][j+1])/4;
			                qy[i][j] =  (qy_int[i][j] + qy_int[i+1][j] + qy_int[i][j+1] + qy_int[i+1][j+1])/4;

							fprintf(fp2,"%d\t\t %d\t\t %f \t\t %f \t\t %f \t\t %f \t\t\n", i,j,z0[i][j],h[i][j],qx[i][j],qy[i][j]);

			//                   
			hFace_E = h[i][j];
			hFace_W = h[i][j];
			hFace_N = h[i][j];
			hFace_S = h[i][j];

			qxFace_E = qx[i][j];
			qxFace_W = qx[i][j];
			qxFace_N = qx[i][j];
			qxFace_S = qx[i][j];

			qyFace_E = qy[i][j];
			qyFace_W = qy[i][j];
			qyFace_N = qy[i][j];
			qyFace_S = qy[i][j];

		
			fprintf(fp, " <xagent>\n");
			fprintf(fp, "\t<name>FloodCell</name>\n");

			fprintf(fp, "\t<inDomain>%d</inDomain>\n", inDomain);
			//                    fprintf(fp, "\t<x>%f</x>\n", x[i]);
			//                    fprintf(fp, "\t<y>%f</y>\n", y[j]);
			fprintf(fp, "\t<x>%d</x>\n", i); // +1 can be added to to output 128 * 128, not 127 * 127 / since FLAME-GPU can read x = 0 that is completely alright
			fprintf(fp, "\t<y>%d</y>\n", j);
			fprintf(fp, "\t<z0>%f</z0>\n", z0[i][j]);
			fprintf(fp, "\t<h>%f</h>\n", h[i][j]);
			fprintf(fp, "\t<qx>%f</qx>\n", qx[i][j]);
			fprintf(fp, "\t<qy>%f</qy>\n", qy[i][j]);

			fprintf(fp, "\t<hFace_E>%f</hFace_E>\n", hFace_E);
			fprintf(fp, "\t<hFace_W>%f</hFace_W>\n", hFace_W);
			fprintf(fp, "\t<hFace_N>%f</hFace_N>\n", hFace_N);
			fprintf(fp, "\t<hFace_S>%f</hFace_S>\n", hFace_S);

			fprintf(fp, "\t<qxFace_E>%f</qxFace_E>\n", qxFace_E);
			fprintf(fp, "\t<qxFace_W>%f</qxFace_W>\n", qxFace_W);
			fprintf(fp, "\t<qxFace_N>%f</qxFace_N>\n", qxFace_N);
			fprintf(fp, "\t<qxFace_S>%f</qxFace_S>\n", qxFace_S);

			fprintf(fp, "\t<qyFace_E>%f</qyFace_E>\n", qyFace_E);
			fprintf(fp, "\t<qyFace_W>%f</qyFace_W>\n", qyFace_W);
			fprintf(fp, "\t<qyFace_N>%f</qyFace_N>\n", qyFace_N);
			fprintf(fp, "\t<qyFace_S>%f</qyFace_S>\n", qyFace_S);

			fprintf(fp, " </xagent>\n");


		}
	}

	fprintf(fp, "</states>");
	fclose(fp);
	return 0;

}


double initial_flow(double x_int, double y_int, double z0_int)
{
	 //1D-Fully wet with no topography
//	double etta = 1.0;//1.875;//1.875; // 
//	double h;

//	if (x_int < 25) 
//	{
//////	if (y_int < 16) 
//////	{
//		
////		h = max2(0.0, etta - z0_int); <-- NOTE: you do not need max2. You can use std::max from algorithm library
//			h = etta - z0_int;//std::max(0.0, etta - z0_int); 
//	}
//	else
//	{
////		h = 0.5;//etta - z0_int; //0.0*max2(0.0, etta - z0_int); <-- NOTE: this is always zero? // MS " Different in test cases "
//		h = 0.1;
//	}
//	
	// case 1 - Radial Dam break , wet
//	double etta = 2.5;
//    double x_o = 20;
//    double y_o = 20;
//    
//	double radius = 2.5;
//    double h;
//       
//       if (sqrt(pow((x_int - x_o),2) + pow((y_int - y_o),2)) <= radius)
//       {
//       	h = etta - z0_int;
//	   }
//	   else
//	   {
//	   	h = .5;
//	   }
	   
	   // Case 2: Three Humps
	   double etta = 1.875;
	   double h;
	   
	   if (x_int <= 16)
	   {
	   	h = etta - z0_int;
	   }
	   else
	   {
	   	h = 0.0;
	   }
   
	return h;
}
//    

/* Function to generate the terrain detail - Three Humps*/
double bed_data(double x_int, double y_int)
{
	// This function generates Three Humps terrain detail in the model

	double zz;

	double x1 = 30.000;
	double y1 = 6.000;

	double x2 = 30.000;
	double y2 = 24.000;
	////
	double x3 = 47.500;
	double y3 = 15.000;
	//  
	//       
	double rm1 = 8.000;
	double rm2 = 8.000;
	double rm3 = 10.000;
	//       

	//       
	//
	//
	double x01 = x_int - x1;
	double x02 = x_int - x2;
	double x03 = x_int - x3;

	double y01 = y_int - y1;
	double y02 = y_int - y2;
	double y03 = y_int - y3;
	//
	//       
	double r1 = sqrt(pow(x01, 2.0) + pow(y01, 2.0));
	double r2 = sqrt(pow(x02, 2.0) + pow(y02, 2.0));
	double r3 = sqrt(pow(x03, 2.0) + pow(y03, 2.0));
	//
	//       
	double zb1 = (rm1 - r1) / 8.0;
	double zb2 = (rm2 - r2) / 8.0;
	double zb3 = 3 * (rm3 - r3) / 10.0;
	double zb4 = 0.0; /*This is the minimum height of the topography*/
		
	zz = 1.0 * std::max( std::max((double)zb1, (double)zb2), std::max((double)zb3, (double)zb4) );

	return zz;
}
