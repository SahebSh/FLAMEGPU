
----------------- MS07Sep2017 ------------------ (The code is not built yet due to the error code : error MSB3721)

Done by the date: 

- 0.XML generator has been coded by MS to generate 'Three Humps' model(test case) initial condition based on mesh-grid model (Matlab/C code)
	* This code is accessible through 0XMLgenerator folder - number of agents can be changed by decreasing the mesh size.
	* The content is then copied and pasted to ~/itterations/0.xml

- Function.c and XMLModelFile are coded and specified based on the case study model (Three Humps - flood modeling)
	* Finite Volume (FV) method has been used to solve Shallow Water Equation (SWE) in this Flood modelling.


----------------- MS08Sep2017 ------------------ (The project has not been built yet)

- The project is set to CUDA 8.0 and is compatible with VS15
- A few modifications in XMLModelFile and corrections in Functions.c (device function corrections and declaration of some variables) has been taken.

----------------- MS13Sep2017 ------------------ (The project builds now)

- 0.xml has been modified to the correct number of agents and array indicators.
- The project builds, but no result is achieved.

----------------- MS25Sep2017 ------------------ 

- 0.xml - the correct i and j is assigned to the variables (all in accordance with the same 'C' project).
- Roe solver is replaced by hll and SWE flux is modified to the correct varibales.
- Still struggling with outputing the results.

----------------- MS03Oct2017 ------------------ still under debugging 
- Corrections made in LFVs
- Reimann Solver is refined with correct values 
- Times step is introduced as global variable (instead of agent environment variable)

----------------- MS16Oct2017 ------------------ ( Model Runs Properly)
- All the bugs of the model is corrected (Many of which caused by boundary condition issues). 

- Boundary condition issue is tackled by using a flux calculations before incorporating with neighbours. so that the input data is restricted to defined values in boundaries. see Ln 782

- Wet/Dry agent message is modified to the correct form of condition. agent->isDry changed to agent->minh_loc <= TOL_H and etc.

- TIMESTEP , DXL, DYL are introduced within the environment of the model. They seemed to be not accessible to the model within the 0.xml file.

- The results are compared (visually checked) with that of C and MATLAB project with the same specifications and domain size. The model shows satisfying results.

* Further steps :
 
	- Introducing adaptive dt to the model, using step functions. 
	- Make DXL and DYL an agent constant (which is automaticlly calculated by initial condition generator), rather than being a global variable. 
	- Transferring the results into a readable text/dot file to be able to visualise/plot the outputs.
	- Build a execuateble visualisation file within the flamegpu framework (manipulation of arguments and commands)

- The case is set to 64*64 Grid and DamBreak of 1.875 case study.


----------------- MS14Nov2017 ------------------ ( Model Runs Properly)
- 0XML generatore modified to 128 agents and checked for correct initial values on 25OCT2017
- XMl converter is added to the example. 
- 0.xml generator is modified to generate desirable number of agents.
- Both are in the 'Codes' folder.
- Not many changes in functions.c and XMLmodelfile is taken in this upload. 

----------------- MS13Dec2017 ------------------ 
- CUDA 8.0 replaced by CUDA 7.0 (personal computer issue)
- Code has been refined in some terms. 
- This update is a backup, as it has been validated in some test cases and the results are satisfying enough. 

----------------- MS15Dec2017 ------------------ 
- Backup after solving QX issue. The model works completely perfect.
 