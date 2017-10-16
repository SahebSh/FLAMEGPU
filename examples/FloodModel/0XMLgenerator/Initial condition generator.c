#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// This function generate 0.XML file based on the data of three hump sulution (FV)
// The output is the initial file to be employed in the flood modelling in FLAME-GPU
// The data which is used in this function is exactly the same as MATLAB solution for SWEs (FV)

double bed_data(double x_int , double y_int);

double initial_flow(double x_int , double y_int , double z0_int);

double maxi(double num1, double num2);

int main()
{
    FILE *fp = fopen("output.txt", "w");
    if (fp == NULL)
    {
    printf("Error opening file!\n");
    exit(1);
    }
    
         // Model constant :  they are imported to the output file          

         double       TIMESTEP = 0.5; // assigned Temporary 
         double       DXL;
         double       DYL;
         int         inDomain = 1 ;
           
        int xmin = 0;
        int xmax = 75;
        int ymin = 0;
        int ymax = 30;
        
        int nx = 64;
        int ny = 64;
        
        double Lx , Ly;
        double dx , dy;
        int i , j ;

          /*Initial variables*/         
          double qx_int[nx+1][ny+1], qy_int[nx+1][ny+1], h_int[nx+1][ny+1], z0_int[nx+1][ny+1];
          double x_int[nx+1],y_int[ny+1];
        
          double qx[nx+2][ny+2], qy[nx+2][ny+2], h[nx+2][ny+2], z0[nx+2][ny+2];
    
          double x[nx+2],y[ny+2];
          double xi[nx],yj[ny]; //In cse a plot is needed
          
          double hFace_E = 0.0 , etFace_E= 0.0, qxFace_E= 0.0, qyFace_E= 0.0;
          double hFace_W = 0.0 , etFace_W= 0.0, qxFace_W= 0.0, qyFace_W= 0.0;
          double hFace_N = 0.0 , etFace_N= 0.0, qxFace_N= 0.0, qyFace_N= 0.0;
          double hFace_S = 0.0 , etFace_S= 0.0, qxFace_S= 0.0, qyFace_S= 0.0;

    
    // initial flow rate
    double qx_initial = 0.00;
    double qy_initial = 0.00;
    
    // Mesh-grid propertise
    Lx = xmax - xmin;
    Ly = ymax - ymin;
    dx = (double)Lx/(double)nx;
    dy = (double)Ly/(double)ny;
    
    DXL = (double)dx; // Temporary
    DYL = (double)dy; // Temporary
    
    
    fprintf(fp,"<states>\n");
    fprintf(fp,"<itno>0</itno>\n");
//    fprintf(fp," <environment>\n"); 
////    fprintf(fp,"  <TIMESTEP>%f</TIMESTEP>\n",TIMESTEP);
////    fprintf(fp,"  <DXL>%f</DXL>\n",DXL);
////    fprintf(fp,"  <DYL>%f</DYL>\n",DYL);
//    fprintf(fp," </environment>\n");
    

      for ( i=1 ; i <= nx+1 ; i++){  
          for ( j=1 ; j <= ny+1 ; j++){
            
            x_int[i] = xmin + (i-1) * dx; 
            y_int[j] = ymin + (j-1) * dy;
            
            z0_int[i][j] = bed_data    ((double)x_int[i],(double)y_int[j]);
            h_int[i][j]  = initial_flow((double)x_int[i],(double)y_int[j],(double)z0_int[i][j]);
            qx_int[i][j] = qx_initial; // Temporary assigned value
            qy_int[i][j] = qy_initial; // Temporary assigned value (However it should be 0 )
            
//            printf("The value of h_initial in x_interface[%f] y_interface[%f] %3f\n",x_int[i],y_int[j], h_int[i][j] );
            //printf("The value of z0_initial in x_interface[%f] y_interface[%f] %3f\n", x_int[1], y_int[1] , z0_int[1][1] );
                     }
            }
            
            for ( i=1 ; i <= nx ; i++){  
                for ( j=1 ; j <= ny ; j++){
                    
                    x[i] = 0.5*(x_int[i] + x_int[i+1]);
                    y[j] = 0.5*(y_int[j] + y_int[j+1]);
                    
                    z0[i][j] = 0.25*(z0_int[i][j+1] + z0_int[i][j] + z0_int[i+1][j] + z0_int[i+1][j+1]);
                    h[i][j]  = 0.25*(h_int [i][j+1] +  h_int[i][j] +  h_int[i+1][j] +  h_int[i+1][j+1]);
                    
                    qx[i][j] = 0.25*(qx_int[i][j+1] + qx_int[i][j] + qx_int[i+1][j] + qx_int[i+1][j+1]);
                    qy[i][j] = 0.25*(qy_int[i][j+1] + qy_int[i][j] + qy_int[i+1][j] + qy_int[i+1][j+1]);
                    
                    
//                    // Missing data at the first iteration  // For Dam break solutions, it needs to be compatible with the problem
//                    if (i == 1)
//                    {
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
                    	
//					} 
//					else {
//						
//						hFace_E = 0;
//                    	hFace_W = 0;
//                    	hFace_N = 0;
//                    	hFace_S = 0;
//                    	
//                    	qxFace_E = 0;
//                    	qxFace_W = 0;
//                    	qxFace_N = 0;
//                    	qxFace_S = 0;
//                    	
//                    	qyFace_E = 0;
//                    	qyFace_W = 0;
//                    	qyFace_N = 0;
//                    	qyFace_S = 0;
//					}

//                   printf("The value of z0 in x[%f] y[%f] %3f\n", x[i], y[j] , z0[i][j] );
//                   *To test the results : 
                    // fprintf(fp," x[%d] = %.3f\ty[%d] = %.3f\tz0 = %.3f \n ", i, x[i], j , y[j] , z0[i][j] );
                      
                    fprintf(fp, " <xagent>\n");
	                fprintf(fp, "\t<name>FloodCell</name>\n");
	                
	                fprintf(fp, "\t<inDomain>%d</inDomain>\n", inDomain);
//                    fprintf(fp, "\t<x>%f</x>\n", x[i]);
//                    fprintf(fp, "\t<y>%f</y>\n", y[j]);
                    fprintf(fp, "\t<x>%d</x>\n", i);
                    fprintf(fp, "\t<y>%d</y>\n", j);
                    fprintf(fp, "\t<z0>%f</z0>\n",z0[i][j]);
                    fprintf(fp, "\t<h>%f</h>\n",h[i][j]);
                    fprintf(fp, "\t<qx>%f</qx>\n",qx[i][j]);
                    fprintf(fp, "\t<qy>%f</qy>\n",qy[i][j]); 
                    
					fprintf(fp, "\t<hFace_E>%f</hFace_E>\n",hFace_E);
					fprintf(fp, "\t<hFace_W>%f</hFace_W>\n",hFace_W);
					fprintf(fp, "\t<hFace_N>%f</hFace_N>\n",hFace_N);
					fprintf(fp, "\t<hFace_S>%f</hFace_S>\n",hFace_S);
					
                    fprintf(fp, "\t<qxFace_E>%f</qxFace_E>\n",qxFace_E);
                    fprintf(fp, "\t<qxFace_W>%f</qxFace_W>\n",qxFace_W);
                    fprintf(fp, "\t<qxFace_N>%f</qxFace_N>\n",qxFace_N);
                    fprintf(fp, "\t<qxFace_S>%f</qxFace_S>\n",qxFace_S);
                    
                    fprintf(fp, "\t<qyFace_E>%f</qyFace_E>\n",qyFace_E);
                    fprintf(fp, "\t<qyFace_W>%f</qyFace_W>\n",qyFace_W);
                    fprintf(fp, "\t<qyFace_N>%f</qyFace_N>\n",qyFace_N);
                    fprintf(fp, "\t<qyFace_S>%f</qyFace_S>\n",qyFace_S);
					        
                    fprintf(fp, " </xagent>\n");
                   
                    
                                    }
                }
                
                  fprintf(fp, "</states>");          
                  fclose(fp);
                  return 0;
    
}

/* Function to generate the terrain detail - Three Humps*/
 double bed_data(double x_int , double y_int)
 {
       double zz;
       
       double x1 = 30.000;
       double y1 = 6.000;
       double x2 = 30.000;
       double y2 = 24.000;
       double x3 = 47.500;
       double y3 = 15.000;
       
       double rm1 = 8.000;
       double rm2 = 8.000;
       double rm3 = 10.000; 
       
       double r1,r2,r3;
       double zb1,zb2,zb3,zb4;
       double zz1,zz2;
       
       double x01,x02,x03,y01,y02,y03;


       x01 = x_int - x1;
       x02 = x_int - x2;
       x03 = x_int - x3;
       y01 = y_int - y1;
       y02 = y_int - y2;
       y03 = y_int - y3;
       
//       r1 = pow(((pow(x01,2))+(pow(y01,2))),0.5);
//       r2 = pow(((pow(x02,2))+(pow(y02,2))),0.5);
//       r2 = pow(((pow(x03,2))+(pow(y03,2))),0.5);
       
       r1 = sqrt((pow(x01,2.0)) + (pow(y01,2.0)));
       r2 = sqrt((pow(x02,2.0)) + (pow(y02,2.0)));
       r3 = sqrt((pow(x03,2.0)) + (pow(y03,2.0))); 

       
       zb1 = (rm1 - r1)/8 ;
       zb2 = (rm2 - r2)/8 ;
       zb3 = 3 * (rm1 - r3)/10 ;
       zb4 = 0; /*This is the max surface*/
       
       zz1 = maxi((double)zb1,(double)zb2 );
       zz2 = maxi((double)zb3,(double)zb4 );
       
       zz = 1.0 * maxi((double)zz1 , (double)zz2);
       
       return zz;
       }
       
 /* function returning the max between two numbers */
double maxi(double num1, double num2) {
      
        double result;
    
      if (num1 > num2)
       result = num1;
       else
           result = num2;
 
   return result; 
}  
 
 double initial_flow(double x_int , double y_int , double z0_int)
 {

       double etta = 1.875;
       double h;
       
       if ( x_int <= 16 ) {
            h = maxi(0.0,etta - z0_int);
            }
            else{
                 h = 0.0 *maxi(0.0,etta - z0_int);
                 }
                 
          return h;
          }
                  
       
