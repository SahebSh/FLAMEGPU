
#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include "header.h"
#include "cutil_math.h"



#define PI 3.1415f
#define RADIANS(x) (PI / 180.0f) * x

//
#if _DEBUG
#define DEBUG_LOG( x, ... ) printf( x, __VA_ARGS__ );
#define DEBUG_LOG_POSITION( name, position ) printf( name ": %f, %f, %f\r\n", position.x, position.y, position.z )
#else
#define DEBUG_LOG( x, ... )
#define DEBUG_LOG_POSITION( name, position )
#endif

//#define SIZE 64 // added by MS20NOV2017
#define epsilon 1.0e-3
#define emsmall 1.0e-12 //0.000000000001 //1.0e-12
#define GRAVITY 9.80665 
#define GLOBAL_MANNING 0.018500
#define CFL 0.5
#define TOL_H 10.0e-4
#define TIMESTEP 0.01 //Radial Dam-break test 
//#define DXL 0.390625  // Based on L=50 and N=128 
//#define DYL 0.390625  // Based on L=50 and N=128
#define DXL 0.3125  // Based on L=40 and N=128 Radial Test-case
#define DYL 0.3125  // Based on L=40 and N=128 Radial Test-case
//#define DXL 1.171875 // Domain 0:75 and N=64 1D dambreak test cases
//#define DYL 0.46875 // Domain 0:30 and N=64
//#define DXL 0.625 // Domain -20:20 and N=64 Radial dambreak test cases
//#define DYL 0.625 // Domain -20:20 and N=64


#define BIG_NUMBER 800000             // Used in WetDryMessage to skip extra calculations MS05Sep2017


inline __device__ double3 hll_x(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R);
inline __device__ double3 hll_y(double h_L, double h_R, double qx_L, double qx_R, double qy_L, double qy_R);

inline __device__ double3 Flux_F(double hh, double qx, double qy);
inline __device__ double3 Flux_G(double hh, double qx, double qy);

inline __device__ double3 F_SWE(double hh, double qx, double qy);
inline __device__ double3 G_SWE(double hh, double qx, double qy);


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
	result.h = agent->h;
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

	//// added to test the case with no friction MS01Dec2017
	//result.x = qx_loc;
	//result.y = qy_loc;

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
	if (agent->inDomain == 1)
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

	if (agent->inDomain == 1)
	{

		//looking up neighbours values for wet/dry tracking
		xmachine_message_WetDryMessage* msg = get_first_WetDryMessage_message<DISCRETE_2D>(WetDryMessage_messages, agent->x, agent->y);

		double maxHeight = agent->minh_loc;

		// MS COMMENTS : WHICH HEIGH OF NEIGHBOUR IS BEING CHECKED HERE? ONE AGAINST OTHER AGENTS ? POSSIBLE ?
		while (msg)
		{
			if (msg->inDomain == 1)
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

		// Commented by MS16Apr due to compare with exact solutions

		//if (maxHeight > TOL_H) // if the Height of water is not less than TOL_H => the friction is needed to be taken into account MS05Sep2017
		//{
		//	Friction_Implicit(agent, TIMESTEP); //
		//}
		//else
		//{

		//	//need to go high, so that it won't affect min calculation when it is tested again . Needed to be tested MS05Sep2017 which is now temporary. needs to be corrected somehow
			agent->minh_loc = BIG_NUMBER;
		//}

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
	////EAST FACE
	//agent->etFace_E = faceLFV.et_face;
	//agent->hFace_E  = faceLFV.h_face;
	//agent->qxFace_E = faceLFV.qFace.x;
	//agent->qyFace_E = faceLFV.qFace.y;

	////WEST FACE
	//agent->etFace_W = faceLFV.et_face;
	//agent->hFace_W  = faceLFV.h_face;
	//agent->qxFace_W = faceLFV.qFace.x;
	//agent->qyFace_W = faceLFV.qFace.y;

	////NORTH FACE
	//agent->etFace_N = faceLFV.et_face;
	//agent->hFace_N  = faceLFV.h_face;
	//agent->qxFace_N = faceLFV.qFace.x;
	//agent->qyFace_N = faceLFV.qFace.y;

	////SOUTH FACE
	//agent->etFace_S = faceLFV.et_face;
	//agent->hFace_S  = faceLFV.h_face;
	//agent->qxFace_S = faceLFV.qFace.x;
	//agent->qyFace_S = faceLFV.qFace.y;
	//
	//if (agent->x == SIZE)
	//{
	//	agent->etFace_E = EastBound.et;
	//	agent->hFace_E = EastBound.h;
	//	agent->qxFace_E = EastBound.qx;
	//	agent->qyFace_E = EastBound.qy;
	//}
	//else
	//{
	//	agent->etFace_E = faceLFV.et_face;
	//	agent->hFace_E = faceLFV.h_face;
	//	agent->qxFace_E = faceLFV.qFace.x;
	//	agent->qyFace_E = faceLFV.qFace.y;
	//}

	//if (agent->x == 1)
	//{
	//	agent->etFace_W = WestBound.et;
	//	agent->hFace_W = WestBound.h;
	//	agent->qxFace_W = WestBound.qx;
	//	agent->qyFace_W = WestBound.qy;
	//}
	//else
	//{
	//	agent->etFace_W = faceLFV.et_face;
	//	agent->hFace_W  = faceLFV.h_face;
	//	agent->qxFace_W = faceLFV.qFace.x;
	//	agent->qyFace_W = faceLFV.qFace.y;
	//}

	//if (agent->y == SIZE)
	//{
	//	agent->etFace_N = NorthBound.et;
	//	agent->hFace_N = NorthBound.h;
	//	agent->qxFace_N = NorthBound.qx;
	//	agent->qyFace_N = NorthBound.qy;
	//}
	//else
	//{
	//	agent->etFace_N = faceLFV.et_face;
	//	agent->hFace_N  = faceLFV.h_face;
	//	agent->qxFace_N = faceLFV.qFace.x;
	//	agent->qyFace_N = faceLFV.qFace.y;
	//}

	//if (agent->y == 1)
	//{
	//	agent->etFace_S = SouthBound.et;
	//	agent->hFace_S  = SouthBound.h;
	//	agent->qxFace_S = SouthBound.qx;
	//	agent->qyFace_S = SouthBound.qy;
	//}
	//else
	//{
	//	agent->etFace_S = faceLFV.et_face;
	//	agent->hFace_S  = faceLFV.h_face;
	//	agent->qxFace_S = faceLFV.qFace.x;
	//	agent->qyFace_S = faceLFV.qFace.y;
	//}


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


	if (agent->inDomain == 1
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
	double qxf_E = 0.0;
	double qyf_E = 0.0;

	double h_L = agent->hFace_E;
	double et_L = agent->etFace_E;

	double qx_L = agent->qxFace_E; // added MS15Nov2017
	double qy_L = agent->qyFace_E;// MS15Nov2017
	//double2 q_L = make_double2(agent->qxFace_E, agent->qyFace_E);  commented MS15Nov2017


	double h_R = h_L;//-
	double et_R = et_L;

	double qx_R = qx_L;
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
	qxf_E = qx_F_L;
	qyf_E = qy_F_L;


	// Initialising WESTERN face

	double zbF_W = agent->z0;
	double hf_W = 0.0;
	double qxf_W = 0.0;
	double qyf_W = 0.0;

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

	qx_L = qx_R;//-
	qy_L = qy_R;


	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, WEST, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	FMinus = hll_x(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_W = z_F;
	hf_W = h_F_R;
	qxf_W = qx_F_R;
	qyf_W = qy_F_R;


	// Initialising NORTHERN face

	double zbF_N = agent->z0;
	double hf_N = 0.0;
	double qxf_N = 0.0;
	double qyf_N = 0.0;

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

	qx_R = qx_L;
	qy_R = qy_L; //-


	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, NORTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	GPlus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_N = z_F;
	hf_N = h_F_L;
	qxf_N = qx_F_L;
	qyf_N = qy_F_L;


	// Initialising SOUTHERN face

	double zbF_S = agent->z0;
	double hf_S = 0.0;
	double qxf_S = 0.0;
	double qyf_S = 0.0;

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

	qx_L = qx_R;
	qy_L = qy_R; //-
	//q_L  = q_R; // -q_L changed to q_L MS15Nov2017


	//Wetting and drying "depth-positivity-preserving" reconstructions
	WD(h_L, h_R, et_L, et_R, qx_L, qx_R, qy_L, qy_R, SOUTH, z_F, h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Flux accross the cell
	GMinus = hll_y(h_F_L, h_F_R, qx_F_L, qx_F_R, qy_F_L, qy_F_R);

	// Local flow data restrictions at the EASTERN face
	zbF_S = z_F;
	hf_S = h_F_R;
	qxf_S = qx_F_R;
	qyf_S = qy_F_R;


	xmachine_message_SpaceOperatorMessage* msg = get_first_SpaceOperatorMessage_message<DISCRETE_2D>(SpaceOperatorMessage_messages, agent->x, agent->y);

	while (msg)

	{
		if (msg->inDomain == 1)
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
				qxf_E = qx_F_L;
				qyf_E = qy_F_L;

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
				qxf_W = qx_F_R;
				qyf_W = qy_F_R;

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
				qxf_N = qx_F_L;
				qyf_N = qy_F_L;


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
				qxf_S = qx_F_R;
				qyf_S = qy_F_R;


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


	agent->h = agent->h - (TIMESTEP / DXL) * (FPlus.x - FMinus.x) - (TIMESTEP / DYL) * (GPlus.x - GMinus.x) + TIMESTEP * SS_1;
	agent->qx = agent->qx - (TIMESTEP / DXL) * (FPlus.y - FMinus.y) - (TIMESTEP / DYL) * (GPlus.y - GMinus.y) + TIMESTEP * SS_2;
	agent->qy = agent->qy - (TIMESTEP / DXL) * (FPlus.z - FMinus.z) - (TIMESTEP / DYL) * (GPlus.z - GMinus.z) + TIMESTEP * SS_3;



	//printf("x =%d \t\t y=%d L0h=%f \t L0qx=%f  \t L0qy=%f  \n",agent->x, agent->y, L0h, L0qx, L0qy);


	// Secure zero velocities at the wet/dry front // Adaptive dt needs to be added to the model with dynamic dt (Using Set functions)
	//////double h0 = agent->h;

	//if (agent->h <= TOL_H) //not working
	//{
	//	agent->qx = 0.0;
	//	agent->qy = 0.0;
	//}
	//else
	//{
	//	agent->qx = agent->qx;
	//	agent->qy = agent->qy;
	//}

	//////	
	//////	//this needs to be set high, so it is ignored in the timestep reduction stage
	//////	agent->timeStep = BIG_NUMBER;
	//////}
	//////else
	//////{
	//////	double up = agent->qx / h0;
	//////	double vp = agent->qy / h0;

	//////	//store for timestep calc
	//////	double xStep = CFL * DXL / (fabs(up) + sqrt(GRAVITY * h0));
	//////	double yStep = CFL * DYL / (fabs(vp) + sqrt(GRAVITY * h0));

	//////	agent->timeStep = min(xStep, yStep);

	//////	//TIMESTEP = agent->timeStep;
	//////}

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

#endif 