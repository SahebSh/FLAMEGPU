
#ifndef _FLAMEGPU_FUNCTIONS
#define _FLAMEGPU_FUNCTIONS

#include "header.h"
#include "cutil_math.h"

// This is to output the computational time for each message function within each iteration (added by MS22May2018) 
//#define INSTRUMENT_ITERATIONS 1
//#define INSTRUMENT_AGENT_FUNCTIONS 1
//#define OUTPUT_POPULATION_PER_ITERATION 1

//
#if _DEBUG
#define DEBUG_LOG( x, ... ) printf( x, __VA_ARGS__ );
#define DEBUG_LOG_POSITION( name, position ) printf( name ": %f, %f, %f\r\n", position.x, position.y, position.z )
#else
#define DEBUG_LOG( x, ... )
#define DEBUG_LOG_POSITION( name, position )
#endif

// global constant of the flood model
#define epsilon 1.0e-3
#define emsmall 1.0e-12 
#define GRAVITY 9.80665 
#define GLOBAL_MANNING 0.018000 // has been set to the three humps test case, originally 0.0185
#define CFL 0.5
#define TOL_H 10.0e-4
#define BIG_NUMBER 800000             // Used in WetDryMessage to skip extra calculations MS05Sep2017


// global constant of the pedestrian model

//removing the need of including CustomVisualisation of pedestrian model
#define ENV_MAX 1.0f
#define ENV_MIN -ENV_MAX
#define ENV_WIDTH (2*ENV_MAX)

#define SCALE_FACTOR 0.03125
#define I_SCALER (SCALE_FACTOR*0.35f)
#define MESSAGE_RADIUS d_message_pedestrian_location_radius
#define MIN_DISTANCE 0.0001f

#define PI 3.1415f
#define RADIANS(x) (PI / 180.0f) * x


__FLAME_GPU_INIT_FUNC__ void initConstants()
{
	// This function assign initial values to DXL, DYL, and dt

	double dt_init;

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


	//// for each flood agent in the state default
	for (int index = 0; index < get_agent_FloodCell_Default_count(); index++)
	{

			//Loading data from the agents
			double hp = get_FloodCell_Default_variable_h(index);

			if (hp > TOL_H)
			{

				double qx = get_FloodCell_Default_variable_qx(index);
				double qy = get_FloodCell_Default_variable_qy(index);

				double up = qx / hp;
				double vp = qy / hp;

				dt_init = fminf(CFL * dxl / (fabs(up) + sqrt(GRAVITY * hp)), CFL * dyl / (fabs(vp) + sqrt(GRAVITY * hp)));
			}

			//store for timestep calc
			//dt_init = fminf(dt_init, dt_xy);

	}
	
	set_dt(&dt_init);
	set_DXL(&dxl);
	set_DYL(&dyl);

	//printf("dx = %f and dy = %f \n", dxl,dyl); //prints assigned dx dy

	//printf("dt = %f \n", dt);
	//printf("xmin = %d xmax = %d ymin = %d ymin = %d\n", x_min,x_max,y_min,y_max);

}

// assigning dt for the next iteration
__FLAME_GPU_STEP_FUNC__ void DELTA_T_func()
{
	// This function takes the minimum dt between all the agents in the domain 
	// and assign it to be the time-step of the simulation for each iteration
	// This function is executed after each iteration

	//double minTimeStep = min_FloodCell_Default_timeStep_variable();


	double minTimeStep = 0.005; // for static time-stepping and measuring computation time

	set_dt(&minTimeStep);


	//printf("dt = %f \n", minTimeStep); //prints assigned dt 

}


inline __device__ double3 hll_x(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R);
inline __device__ double3 hll_y(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R);

//inline __device__ double3 Flux_F(double hh, double qx, double qy);
//inline __device__ double3 Flux_G(double hh, double qx, double qy);

inline __device__ double3 F_SWE(double hh, double qx, double qy);
inline __device__ double3 G_SWE(double hh, double qx, double qy);

//inline __device__ double3 timeStep(double hh, double qx, double qy);

// Approximating the minimum dt from all the agents
//__global__ void timestep(double *timeStep);


//inline __device__ double3 Sb(double hx_mod, double hy_mod, double zprime_x, double zprime_y);

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
	AgentFlowData result;

	result.z0 = agent->z0;
	result.h  = agent->h;
	result.et = agent->z0 + agent->h; // added by MS27Sep2017
	result.qx = agent->qx;
	result.qy = agent->qy;

	return result;

}


// Boundary condition in ghost cells // 
inline __device__ void centbound(xmachine_memory_FloodCell* agent, const AgentFlowData& FlowData, AgentFlowData& centBoundData)
{
	// NB. It is assumed that the ghost cell is at the same level as the present cell.
	//** Assign the the same topography data for the ghost cell **!

	//Default is a reflective boundary
	centBoundData.z0 = FlowData.z0;

	centBoundData.h = FlowData.h;

	centBoundData.et = FlowData.et; // added by MS27Sep2017 // 

	centBoundData.qx = -FlowData.qx; //

	centBoundData.qy = -FlowData.qy; //

}

//
//// Conditional function to check whether the Height is less than TOL_H or not
////inline __device__ bool IsDry(double waterHeight)
////{
////	return waterHeight <= TOL_H;
////}

// This function should be called when minh_loc is greater than TOL_H
inline __device__ double2 friction_2D(double dt_loc, double h_loc, double qx_loc, double qy_loc)
{
	//This function add the friction contribution to wet cells.
	//This fucntion should not be called when: minh_loc.LE.TOL_H << check if the function has the criterion to be taken into accont MS05Sep2017


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


		//if (maxHeight > TOL_H) // if the Height of water is not less than TOL_H => the friction is needed to be taken into account MS05Sep2017
		//{

		//		// Contribution of friction term
		//		//Friction_Implicit(agent, TIMESTEP); //

		//	//Friction term has been disabled for radial deam-break
		//	//	Friction_Implicit(agent,TIMESTEP); //

		//}
		//else
		//{

		//	//need to go high, so that it won't affect min calculation when it is tested again . Needed to be tested MS05Sep2017 which is now temporary. needs to be corrected somehow
			agent->minh_loc = BIG_NUMBER;

			// taking agent->timeStep to a non-zero value execuled zero from taking the minimum time-step of all the agents
			//agent->timeStep = BIG_NUMBER;

		//}


		// Approximation of time step at the first iteration
		// Initialisation of timestep to a non-zero value, so that it can be approximated for the min dt 'MS23May2018'
		//agent->timeStep = BIG_NUMBER;

		//double hp = agent->h;
		//double dt = agent->timeStep;

		////printf("dt = %f" , dt);

		//double up = agent->qx / hp;
		//double vp = agent->qy / hp;

		////store for timestep calc
		//double xStep = CFL * DXL / (fabs(up) + sqrt(GRAVITY * hp));
		//double yStep = CFL * DYL / (fabs(vp) + sqrt(GRAVITY * hp));

		//double dt_xy = fminf(xStep, yStep);

		//agent->timeStep = fminf(dt, dt_xy);

	}

	//printf("qx = %f \t qy = %f \n", agent->qx, agent->qy);

	return 0;
}


inline __device__ LFVResult LFV(const AgentFlowData& FlowData)
{

	LFVResult result;


	result.h_face = FlowData.h;

	result.et_face = FlowData.et;

	result.qFace.x = FlowData.qx;

	result.qFace.y = FlowData.qy;

	return result;
}

__FLAME_GPU_FUNC__ int PrepareSpaceOperator(xmachine_memory_FloodCell* agent, xmachine_message_SpaceOperatorMessage_list* SpaceOperatorMessage_messages)
{
	AgentFlowData FlowData = GetFlowDataFromAgent(agent);

	//AgentFlowData EastBound;
	//AgentFlowData WestBound;
	//AgentFlowData NorthBound;
	//AgentFlowData SouthBound;


	//centbound(agent, FlowData, EastBound);
	//centbound(agent, FlowData, WestBound);
	//centbound(agent, FlowData, NorthBound);
	//centbound(agent, FlowData, SouthBound);


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
	double3 FPlus = make_double3(0.0, 0.0, 0.0);
	double3 FMinus = make_double3(0.0, 0.0, 0.0);
	double3 GPlus = make_double3(0.0, 0.0, 0.0);
	double3 GMinus = make_double3(0.0, 0.0, 0.0);


	// Initialising EASTER face

	double zbF_E = agent->z0;
	double hf_E = 0.0;
	//double qxf_E = 0.0;
	//double qyf_E = 0.0;

	double h_L = agent->hFace_E;
	double et_L = agent->etFace_E;

	double qx_L = agent->qxFace_E; // added MS15Nov2017
	double qy_L = agent->qyFace_E;// MS15Nov2017
	//double2 q_L = make_double2(agent->qxFace_E, agent->qyFace_E);  commented MS15Nov2017


	double h_R = h_L;
	double et_R = et_L;
	
	//Case three humps : reflective
	//double qx_R = -qx_L;//
	//double qy_R = qy_L;

	//Case Radial dam : imposed
	double qx_R = qx_L;//
	double qy_R = qy_L;



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
	//qxf_E = qx_F_L;
	//qyf_E = qy_F_L;


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
	//q_R  = make_double2(agent->qxFace_W, agent->qyFace_W);


	h_L = h_R;
	et_L = et_R;

	//Case Radial dam : imposed
	qx_L = qx_R;//-
	qy_L = qy_R;
	
	//Case Three Humps : reflective
	//qx_L = -qx_R;//-
	//qy_L = qy_R;


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
	//q_L  = make_double2(agent->qxFace_N, agent->qyFace_N);


	h_R = h_L;
	et_R = et_L;


	//Case Radial dam : imposed
	qx_R = qx_L;
	qy_R = qy_L; //-

	//Case Three Humps : reflective
	//qx_R = qx_L;
	//qy_R = -qy_L; //-


	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, NORTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	GPlus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_N = z_F;
	hf_N = h_F_L;
	//qxf_N = qx_F_L;
	//qyf_N = qy_F_L;


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
	//q_R  = make_double2(agent->qxFace_S, agent->qyFace_S);


	h_L = h_R;
	et_L = et_R;

	//Case Radial dam : imposed
	qx_L = qx_R;
	qy_L = qy_R; //-

	//Case Three Humps : Reflective
	//qx_L = qx_R;
	//qy_L = -qy_R; //-



	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, SOUTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	GMinus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_S = z_F;
	hf_S = h_F_R;
	//qxf_S = qx_F_R;
	//qyf_S = qy_F_R;


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
	agent->h  = agent->h  - (dt / DXL) * (FPlus.x - FMinus.x) - (dt / DYL) * (GPlus.x - GMinus.x) + dt * SS_1;
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

__FLAME_GPU_FUNC__ int getNewExitLocation(RNG_rand48* rand48){

	int exit1_compare = EXIT1_PROBABILITY;
	int exit2_compare = EXIT2_PROBABILITY + exit1_compare;
	int exit3_compare = EXIT3_PROBABILITY + exit2_compare;
	int exit4_compare = EXIT4_PROBABILITY + exit3_compare;
	int exit5_compare = EXIT5_PROBABILITY + exit4_compare;
	int exit6_compare = EXIT6_PROBABILITY + exit5_compare;

	float range = (float)(EXIT1_PROBABILITY +
		EXIT2_PROBABILITY +
		EXIT3_PROBABILITY +
		EXIT4_PROBABILITY +
		EXIT5_PROBABILITY +
		EXIT6_PROBABILITY +
		EXIT7_PROBABILITY);

	float rand = rnd<DISCRETE_2D>(rand48)*range;

	if (rand<exit1_compare)
		return 1;
	else if (rand<exit2_compare)
		return 2;
	else if (rand<exit3_compare)
		return 3;
	else if (rand<exit4_compare)
		return 4;
	else if (rand<exit5_compare)
		return 5;
	else if (rand<exit6_compare)
		return 6;
	else
		return 7;

}

__FLAME_GPU_FUNC__ int output_pedestrian_location(xmachine_memory_agent* agent, xmachine_message_pedestrian_location_list* pedestrian_location_messages){


	add_pedestrian_location_message(pedestrian_location_messages, agent->x, agent->y, 0.0);

	return 0;
}

__FLAME_GPU_FUNC__ int output_navmap_cells(xmachine_memory_navmap* agent, xmachine_message_navmap_cell_list* navmap_cell_messages){

	add_navmap_cell_message<DISCRETE_2D>(navmap_cell_messages,
		agent->x, agent->y,
		agent->exit_no,
		agent->height,
		agent->collision_x, agent->collision_y,
		agent->exit0_x, agent->exit0_y,
		agent->exit1_x, agent->exit1_y,
		agent->exit2_x, agent->exit2_y,
		agent->exit3_x, agent->exit3_y,
		agent->exit4_x, agent->exit4_y,
		agent->exit5_x, agent->exit5_y,
		agent->exit6_x, agent->exit6_y);

	return 0;
}

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
		if ((separation < MESSAGE_RADIUS) && (separation>MIN_DISTANCE)){
			glm::vec2 to_agent = normalize(agent_pos - message_pos);
			float ang = acosf(dot(agent_vel, to_agent));
			float perception = 45.0f;

			//STEER
			if ((ang < RADIANS(perception)) || (ang > 3.14159265f - RADIANS(perception))){
				glm::vec2 s_velocity = to_agent;
				s_velocity *= powf(I_SCALER / separation, 1.25f)*STEER_WEIGHT;
				navigate_velocity += s_velocity;
			}

			//AVOID
			glm::vec2 a_velocity = to_agent;
			a_velocity *= powf(I_SCALER / separation, 2.00f)*AVOID_WEIGHT;
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

__FLAME_GPU_FUNC__ int force_flow(xmachine_memory_agent* agent, xmachine_message_navmap_cell_list* navmap_cell_messages, RNG_rand48* rand48){

	//map agent position into 2d grid
	int x = floor(((agent->x + ENV_MAX) / ENV_WIDTH)*d_message_navmap_cell_width);
	int y = floor(((agent->y + ENV_MAX) / ENV_WIDTH)*d_message_navmap_cell_width);

	//lookup single message
	xmachine_message_navmap_cell* current_message = get_first_navmap_cell_message<CONTINUOUS>(navmap_cell_messages, x, y);

	glm::vec2 collision_force = glm::vec2(current_message->collision_x, current_message->collision_y);
	collision_force *= COLLISION_WEIGHT;

	//exit location of cell
	int exit_location = current_message->exit_no;

	//agent death flag
	int kill_agent = 0;

	//goal force
	glm::vec2 goal_force;
	if (agent->exit_no == 1)
	{
		goal_force = glm::vec2(current_message->exit0_x, current_message->exit0_y);
		if (exit_location == 1)
		{
			if (EXIT1_STATE)
				kill_agent = 1;
			else
				agent->exit_no = getNewExitLocation(rand48);
		}
	}
	else if (agent->exit_no == 2)
	{
		goal_force = glm::vec2(current_message->exit1_x, current_message->exit1_y);
		if (exit_location == 2)
		if (EXIT2_STATE)
			kill_agent = 1;
		else
			agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 3)
	{
		goal_force = glm::vec2(current_message->exit2_x, current_message->exit2_y);
		if (exit_location == 3)
		if (EXIT3_STATE)
			kill_agent = 1;
		else
			agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 4)
	{
		goal_force = glm::vec2(current_message->exit3_x, current_message->exit3_y);
		if (exit_location == 4)
		if (EXIT4_STATE)
			kill_agent = 1;
		else
			agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 5)
	{
		goal_force = glm::vec2(current_message->exit4_x, current_message->exit4_y);
		if (exit_location == 5)
		if (EXIT5_STATE)
			kill_agent = 1;
		else
			agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 6)
	{
		goal_force = glm::vec2(current_message->exit5_x, current_message->exit5_y);
		if (exit_location == 6)
		if (EXIT6_STATE)
			kill_agent = 1;
		else
			agent->exit_no = getNewExitLocation(rand48);
	}
	else if (agent->exit_no == 7)
	{
		goal_force = glm::vec2(current_message->exit6_x, current_message->exit6_y);
		if (exit_location == 7)
		if (EXIT7_STATE)
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

	return kill_agent;
}

__FLAME_GPU_FUNC__ int move(xmachine_memory_agent* agent){

	glm::vec2 agent_pos = glm::vec2(agent->x, agent->y);
	glm::vec2 agent_vel = glm::vec2(agent->velx, agent->vely);
	glm::vec2 agent_steer = glm::vec2(agent->steer_x, agent->steer_y);

	float current_speed = length(agent_vel) + 0.025f;//(powf(length(agent_vel), 1.75f)*0.01f)+0.025f;

	//apply more steer if speed is greater
	agent_vel += current_speed*agent_steer;
	float speed = length(agent_vel);
	//limit speed
	if (speed >= agent->speed){
		agent_vel = normalize(agent_vel)*agent->speed;
		speed = agent->speed;
	}

	//update position x=v.dt
	agent_pos += agent_vel*TIME_SCALER;


	//animation
	agent->animate += (agent->animate_dir * powf(speed, 2.0f)*TIME_SCALER*100.0f);
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
		agent->x += 2.0f;
	if (agent->x > 1.0f)
		agent->x -= 2.0f;
	if (agent->y < -1.0f)
		agent->y += 2.0f;
	if (agent->y > 1.0f)
		agent->y -= 2.0f;

	return 0;
}

__FLAME_GPU_FUNC__ int generate_pedestrians(xmachine_memory_navmap* agent, xmachine_memory_agent_list* agent_agents, RNG_rand48* rand48){

	if (agent->exit_no > 0)
	{
		float random = rnd<DISCRETE_2D>(rand48);
		bool emit_agent = false;

		if ((agent->exit_no == 1) && ((random < EMMISION_RATE_EXIT1*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 2) && ((random <EMMISION_RATE_EXIT2*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 3) && ((random <EMMISION_RATE_EXIT3*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 4) && ((random <EMMISION_RATE_EXIT4*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 5) && ((random <EMMISION_RATE_EXIT5*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 6) && ((random <EMMISION_RATE_EXIT6*TIME_SCALER)))
			emit_agent = true;
		if ((agent->exit_no == 7) && ((random <EMMISION_RATE_EXIT7*TIME_SCALER)))
			emit_agent = true;

		if (emit_agent){
			float x = ((agent->x + 0.5f) / (d_message_navmap_cell_width / ENV_WIDTH)) - ENV_MAX;
			float y = ((agent->y + 0.5f) / (d_message_navmap_cell_width / ENV_WIDTH)) - ENV_MAX;
			int exit = getNewExitLocation(rand48);
			float animate = rnd<DISCRETE_2D>(rand48);
			float speed = (rnd<DISCRETE_2D>(rand48))*0.5f + 1.0f;
			add_agent_agent(agent_agents, x, y, 0.0f, 0.0f, 0.0f, 0.0f, agent->height, exit, speed, 1, animate, 1);
		}
	}


	return 0;
}


#endif 