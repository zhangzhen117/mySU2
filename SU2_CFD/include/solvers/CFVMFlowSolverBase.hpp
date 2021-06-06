/*!
 * \file CFVMFlowSolverBase.hpp
 * \brief Base class template for all FVM flow solvers.
 * \version 7.1.1 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2020, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../../../Common/include/parallelization/omp_structure.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "CSolver.hpp"

class CNumericsSIMD;

template <class VariableType, ENUM_REGIME FlowRegime>
class CFVMFlowSolverBase : public CSolver {
 protected:
  static constexpr size_t MAXNDIM = 3; /*!< \brief Max number of space dimensions, used in some static arrays. */
  static constexpr size_t MAXNVAR = VariableType::MAXNVAR; /*!< \brief Max number of variables, for static arrays. */

  static constexpr size_t OMP_MAX_SIZE = 512; /*!< \brief Max chunk size for light point loops. */
  static constexpr size_t OMP_MIN_SIZE = 32;  /*!< \brief Min chunk size for edge loops (max is color group size). */

  unsigned long omp_chunk_size; /*!< \brief Chunk size used in light point loops. */

  su2double Mach_Inf = 0.0;          /*!< \brief Mach number at the infinity. */
  su2double Density_Inf = 0.0;       /*!< \brief Density at the infinity. */
  su2double Energy_Inf = 0.0;        /*!< \brief Energy at the infinity. */
  su2double Temperature_Inf = 0.0;   /*!< \brief Energy at the infinity. */
  su2double Pressure_Inf = 0.0;      /*!< \brief Pressure at the infinity. */
  su2double* Velocity_Inf = nullptr; /*!< \brief Flow Velocity vector at the infinity. */

  su2double Viscosity_Inf; /*!< \brief Viscosity at the infinity. */
  su2double Tke_Inf;       /*!< \brief Turbulent kinetic energy at the infinity. */

  su2double StrainMag_Max; /*!< \brief Maximum Strain Rate magnitude. */
  su2double Omega_Max;     /*!< \brief Maximum Omega. */

  su2double Global_Delta_Time = 0.0, /*!< \brief Time-step for TIME_STEPPING time marching strategy. */
  Global_Delta_UnstTimeND = 0.0;     /*!< \brief Unsteady time step for the dual time strategy. */

  unsigned long ErrorCounter = 0;    /*!< \brief Counter for number of un-physical states. */

  /*!
   * \brief Auxilary types to store common aero coefficients (avoids repeating oneself so much).
   */
  struct AeroCoeffsArray {
    su2double* CD = nullptr;     /*!< \brief Drag coefficient. */
    su2double* CL = nullptr;     /*!< \brief Lift coefficient. */
    su2double* CSF = nullptr;    /*!< \brief Sideforce coefficient. */
    su2double* CEff = nullptr;   /*!< \brief Efficiency (Cl/Cd). */
    su2double* CFx = nullptr;    /*!< \brief x Force coefficient. */
    su2double* CFy = nullptr;    /*!< \brief y Force coefficient. */
    su2double* CFz = nullptr;    /*!< \brief z Force coefficient. */
    su2double* CMx = nullptr;    /*!< \brief x Moment coefficient. */
    su2double* CMy = nullptr;    /*!< \brief y Moment coefficient. */
    su2double* CMz = nullptr;    /*!< \brief z Moment coefficient. */
    su2double* CoPx = nullptr;   /*!< \brief x Moment coefficient. */
    su2double* CoPy = nullptr;   /*!< \brief y Moment coefficient. */
    su2double* CoPz = nullptr;   /*!< \brief z Moment coefficient. */
    su2double* CT = nullptr;     /*!< \brief Thrust coefficient. */
    su2double* CQ = nullptr;     /*!< \brief Torque coefficient. */
    su2double* CMerit = nullptr; /*!< \brief Rotor Figure of Merit. */
    int _size = 0;               /*!< \brief Array size. */

    void allocate(int size); /*!< \brief Allocates arrays. */

    void setZero(int i); /*!< \brief Sets all values to zero at a particular index. */
    void setZero() {     /*!< \brief Sets all values to zero for all indices. */
      for (int i = 0; i < _size; ++i) setZero(i);
    }

    AeroCoeffsArray(int size = 0) : _size(size) {
      if (size) allocate(size);
    }

    ~AeroCoeffsArray();
  };

  /*!
   * \brief Scalar version of the coefficients type.
   */
  struct AeroCoeffs {
    su2double CD, CL, CSF, CEff, CFx, CFy, CFz, CMx, CMy, CMz, CoPx, CoPy, CoPz, CT, CQ, CMerit;

    void setZero() {
      CD = CL = CSF = CEff = CFx = CFy = CFz = CMx = CMy = CMz = CoPx = CoPy = CoPz = CT = CQ = CMerit = 0.0;
    }

    AeroCoeffs() { setZero(); }
  };

  AeroCoeffsArray InvCoeff;        /*!< \brief Inviscid pressure contributions for each boundary. */
  AeroCoeffsArray SurfaceInvCoeff; /*!< \brief Inviscid pressure contributions for each monitoring boundary. */
  AeroCoeffs AllBoundInvCoeff;     /*!< \brief Total pressure contribution for all the boundaries. */

  AeroCoeffsArray MntCoeff;        /*!< \brief Inviscid momentum contributions for each boundary. */
  AeroCoeffsArray SurfaceMntCoeff; /*!< \brief Inviscid momentum contributions for each monitoring boundary. */
  AeroCoeffs AllBoundMntCoeff;     /*!< \brief Total momentum contribution for all the boundaries. */

  AeroCoeffsArray ViscCoeff;        /*!< \brief Viscous contributions for each boundary. */
  AeroCoeffsArray SurfaceViscCoeff; /*!< \brief Viscous contributions for each monitoring boundary. */
  AeroCoeffs AllBoundViscCoeff;     /*!< \brief Total pressure contribution for all the boundaries. */

  AeroCoeffsArray SurfaceCoeff; /*!< \brief Totals for each monitoring surface. */
  AeroCoeffs TotalCoeff;        /*!< \brief Totals for all boundaries. */

  su2double AeroCoeffForceRef = 1.0;    /*!< \brief Reference force for aerodynamic coefficients. */

  su2double InverseDesign = 0.0;        /*!< \brief Inverse design functional for each boundary. */
  su2double Total_ComboObj = 0.0;       /*!< \brief Total 'combo' objective for all monitored boundaries */
  su2double Total_Custom_ObjFunc = 0.0; /*!< \brief Total custom objective function for all the boundaries. */
  su2double Total_CpDiff = 0.0;         /*!< \brief Total Equivalent Area coefficient for all the boundaries. */
  su2double Total_TemDiff = 0.0;        /*!< \brief zhen: for temperature inverse design  */
  su2double Total_HeatFluxDiff = 0.0;   /*!< \brief Total Equivalent Area coefficient for all the boundaries. */
  su2double Total_MassFlowRate = 0.0;   /*!< \brief Total Mass Flow Rate on monitored boundaries. */
  su2double Total_CNearFieldOF = 0.0;   /*!< \brief Total Near-Field Pressure coefficient for all the boundaries. */
  su2double Total_Heat = 0.0;           /*!< \brief Total heat load for all the boundaries. */
  su2double Total_MaxHeat = 0.0;        /*!< \brief Maximum heat flux on all boundaries. */
  su2double AllBound_CNearFieldOF_Inv = 0.0; /*!< \brief Near-Field press coeff (inviscid) for all the boundaries. */
  vector<su2double> CNearFieldOF_Inv;    /*!< \brief Near field pressure (inviscid) for each boundary. */
  vector<su2double> Surface_HF_Visc;     /*!< \brief Total (integrated) heat flux for each monitored surface. */
  vector<su2double> Surface_MaxHF_Visc;  /*!< \brief Maximum heat flux for each monitored surface. */
  vector<su2double> HF_Visc;             /*!< \brief Heat load (viscous contribution) for each boundary. */
  vector<su2double> MaxHF_Visc;          /*!< \brief Maximum heat flux (viscous contribution) for each boundary. */
  su2double AllBound_HF_Visc = 0.0;      /*!< \brief Heat load (viscous contribution) for all the boundaries. */
  su2double AllBound_MaxHF_Visc = 0.0;   /*!< \brief Maximum heat flux (viscous contribution) for all boundaries. */

  vector<vector<su2double> > Inlet_Ptotal;      /*!< \brief Value of the Total P. */
  vector<vector<su2double> > Inlet_Ttotal;      /*!< \brief Value of the Total T. */
  vector<su2activematrix> Inlet_FlowDir;        /*!< \brief Value of the Flow Direction. */
  vector<vector<su2double> > HeatFlux;          /*!< \brief Heat transfer coefficient for each boundary and vertex. */
  vector<vector<su2double> > HeatFluxTarget;    /*!< \brief Heat transfer coefficient for each boundary and vertex. */
  vector<su2activematrix> CharacPrimVar;        /*!< \brief Value of the characteristic variables at each boundary. */
  vector<su2activematrix> CSkinFriction;        /*!< \brief Skin friction coefficient for each boundary and vertex. */
  vector<vector<su2double> > WallShearStress;   /*!< \brief Wall Shear Stress for each boundary and vertex. */
  vector<su2activematrix> HeatConjugateVar;     /*!< \brief CHT variables for each boundary and vertex. */
  vector<vector<su2double> > CPressure;         /*!< \brief Pressure coefficient for each boundary and vertex. */
  vector<vector<su2double> > CPressureTarget;   /*!< \brief Target Pressure coefficient for each boundary and vertex. */
  vector<vector<su2double> > Tem;               /*!< \brief zhen: for temperature inverse design */
  vector<vector<su2double> > TemTarget;         /*!< \brief zhen: for temperature inverse design. */
  vector<vector<su2double> > YPlus;             /*!< \brief Yplus for each boundary and vertex. */

  bool space_centered;       /*!< \brief True if space centered scheme used. */
  bool euler_implicit;       /*!< \brief True if euler implicit scheme used. */
  bool least_squares;        /*!< \brief True if computing gradients by least squares. */
  su2double Gamma;           /*!< \brief Fluid's Gamma constant (ratio of specific heats). */
  su2double Gamma_Minus_One; /*!< \brief Fluids's Gamma - 1.0  . */

  /*--- Sliding meshes variables ---*/

  vector<su2matrix<su2double*> > SlidingState; // vector of matrix of pointers... inner dim alloc'd elsewhere (welcome, to the twilight zone)
  vector<vector<int> > SlidingStateNodes;

  /*--- Shallow copy of grid coloring for OpenMP parallelization. ---*/

#ifdef HAVE_OMP
  vector<GridColor<> > EdgeColoring; /*!< \brief Edge colors. */
  bool ReducerStrategy = false;      /*!< \brief If the reducer strategy is in use. */
#else
  array<DummyGridColor<>, 1> EdgeColoring;
  /*--- Never use the reducer strategy if compiling for MPI-only. ---*/
  static constexpr bool ReducerStrategy = false;
#endif

  /*--- Edge fluxes, for OpenMP parallelization of difficult-to-color grids.
   * We first store the fluxes and then compute the sum for each cell.
   * This strategy is thread-safe but lower performance than writting to both
   * end points of each edge, so we only use it when necessary, i.e. when the
   * coloring does not allow "enough" parallelism. ---*/

  CSysVector<su2double> EdgeFluxes; /*!< \brief Flux across each edge. */

  CNumericsSIMD* edgeNumerics = nullptr; /*!< \brief Object for edge flux computation. */

  /*!
   * \brief The highest level in the variable hierarchy the DERIVED solver can safely use.
   */
  VariableType* nodes = nullptr;

  /*!
   * \brief Return nodes to allow CSolver::base_nodes to be set.
   */
  inline CVariable* GetBaseClassPointerToNodes() final { return nodes; }

  /*!
   * \brief Default constructor, this class is not directly instantiable.
   */
  CFVMFlowSolverBase() : CSolver() {}

  /*!
   * \brief Allocate member variables.
   */
  void Allocate(const CConfig& config);

  /*!
   * \brief Allocate small member variables that ideally should not be used.
   */
  void AllocateTerribleLegacyTemporaryVariables();

  /*!
   * \brief Communicate the initial solver state.
   */
  void CommunicateInitialState(CGeometry* geometry, const CConfig* config);

  /*!
   * \brief Initialize thread parallel variables.
   */
  void HybridParallelInitialization(const CConfig& config, CGeometry& geometry);

  /*!
   * \brief Move solution to previous time levels (for restarts).
   */
  void PushSolutionBackInTime(unsigned long TimeIter, bool restart, bool rans, CSolver*** solver_container,
                              CGeometry** geometry, CConfig* config);

  /*!
   * \brief Evaluate common part of objective function to all solvers.
   */
  su2double EvaluateCommonObjFunc(const CConfig& config) const;

  /*!
   * \brief Method to compute convective and viscous residual contribution using vectorized numerics.
   */
  void EdgeFluxResidual(const CGeometry *geometry, const CSolver* const* solvers, const CConfig *config);

  /*!
   * \brief Sum the edge fluxes for each cell to populate the residual vector, only used on coarse grids.
   */
  void SumEdgeFluxes(const CGeometry* geometry);

  /*!
   * \brief Instantiate a SIMD numerics object.
   */
  inline virtual void InstantiateEdgeNumerics(const CSolver* const* solvers, const CConfig* config) {}

  /*!
   * \brief Compute the viscous contribution for a particular edge.
   * \note The convective residual methods include a call to this for each edge,
   *       this allows convective and viscous loops to be "fused".
   * \param[in] iEdge - Edge for which the flux and Jacobians are to be computed.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   */
  inline virtual void Viscous_Residual(unsigned long iEdge, CGeometry *geometry, CSolver **solver_container,
                                       CNumerics *numerics, CConfig *config) { }
  void Viscous_Residual_impl(unsigned long iEdge, CGeometry *geometry, CSolver **solver_container,
                             CNumerics *numerics, CConfig *config);
  using CSolver::Viscous_Residual; /*--- Silence warning ---*/

  /*!
   * \brief Compute a suitable under-relaxation parameter to limit the change in the solution variables over a nonlinear
   * iteration for stability.
   */
  void ComputeUnderRelaxationFactor(const CConfig* config);

  /*!
   * \brief General implementation to load a flow solution from a restart file.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver - Container vector with all of the solvers.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iter - Current external iteration number.
   * \param[in] update_geo - Flag for updating coords and grid velocity.
   * \param[in] RestartSolution - Optional buffer to load restart vars into,
   *            this allows default values to be given when nVar > nVar_Restart.
   * \param[in] nVar_Restart - Number of restart variables, if 0 defaults to nVar.
   */
  void LoadRestart_impl(CGeometry **geometry, CSolver ***solver, CConfig *config, int iter, bool update_geo,
                        su2double* RestartSolution = nullptr, unsigned short nVar_Restart = 0);

  /*!
   * \brief Generic implementation to compute the time step based on CFL and conv/visc eigenvalues.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   * \param[in] Iteration - Value of the current iteration.
   * \tparam SoundSpeedFunc - Function object to compute speed of sound.
   * \tparam LambdaViscFunc - Function object to compute the viscous lambda.
   * \note Both functors need to implement (nodes,iPoint,jPoint) for edges, and (nodes,iPoint) for vertices.
   */
  template<class SoundSpeedFunc, class LambdaViscFunc>
  FORCEINLINE void SetTime_Step_impl(const SoundSpeedFunc& soundSpeed,
                                     const LambdaViscFunc& lambdaVisc,
                                     CGeometry *geometry,
                                     CSolver **solver_container,
                                     CConfig *config,
                                     unsigned short iMesh,
                                     unsigned long Iteration) {

    const bool viscous       = config->GetViscous();
    const bool implicit      = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
    const bool time_stepping = (config->GetTime_Marching() == TIME_STEPPING);
    const bool dual_time     = (config->GetTime_Marching() == DT_STEPPING_1ST) ||
                               (config->GetTime_Marching() == DT_STEPPING_2ND);
    const su2double K_v = 0.25;

    /*--- Init thread-shared variables to compute min/max values.
     *    Critical sections are used for this instead of reduction
     *    clauses for compatibility with OpenMP 2.0 (Windows...). ---*/

    SU2_OMP_MASTER
    {
      Min_Delta_Time = 1e30;
      Max_Delta_Time = 0.0;
      Global_Delta_UnstTimeND = 1e30;
    }
    SU2_OMP_BARRIER

    /*--- Loop domain points. ---*/

    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {

      /*--- Set maximum eigenvalues to zero. ---*/

      nodes->SetMax_Lambda_Inv(iPoint,0.0);

      if (viscous)
        nodes->SetMax_Lambda_Visc(iPoint,0.0);

      /*--- Loop over the neighbors of point i. ---*/

      for (unsigned short iNeigh = 0; iNeigh < geometry->nodes->GetnPoint(iPoint); ++iNeigh)
      {
        auto jPoint = geometry->nodes->GetPoint(iPoint,iNeigh);

        auto iEdge = geometry->nodes->GetEdge(iPoint,iNeigh);
        auto Normal = geometry->edges->GetNormal(iEdge);
        auto Area2 = GeometryToolbox::SquaredNorm(nDim, Normal);

        /*--- Mean Values ---*/

        su2double Mean_ProjVel = 0.5 * (nodes->GetProjVel(iPoint,Normal) + nodes->GetProjVel(jPoint,Normal));
        su2double Mean_SoundSpeed = soundSpeed(*nodes, iPoint, jPoint) * sqrt(Area2);

        /*--- Adjustment for grid movement ---*/

        if (dynamic_grid) {
          const su2double *GridVel_i = geometry->nodes->GetGridVel(iPoint);
          const su2double *GridVel_j = geometry->nodes->GetGridVel(jPoint);

          for (unsigned short iDim = 0; iDim < nDim; iDim++)
            Mean_ProjVel -= 0.5 * (GridVel_i[iDim] + GridVel_j[iDim]) * Normal[iDim];
        }

        /*--- Inviscid contribution ---*/

        su2double Lambda = fabs(Mean_ProjVel) + Mean_SoundSpeed;
        nodes->AddMax_Lambda_Inv(iPoint,Lambda);

        /*--- Viscous contribution ---*/

        if (!viscous) continue;

        Lambda = lambdaVisc(*nodes, iPoint, jPoint) * Area2;
        nodes->AddMax_Lambda_Visc(iPoint, Lambda);
      }

    }

    /*--- Loop boundary edges ---*/

    for (unsigned short iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
      if ((config->GetMarker_All_KindBC(iMarker) != INTERNAL_BOUNDARY) &&
          (config->GetMarker_All_KindBC(iMarker) != PERIODIC_BOUNDARY)) {

        SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
        for (auto iVertex = 0ul; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

          /*--- Point identification, Normal vector and area ---*/

          auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

          if (!geometry->nodes->GetDomain(iPoint)) continue;

          auto Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
          auto Area2 = GeometryToolbox::SquaredNorm(nDim, Normal);

          /*--- Mean Values ---*/

          su2double ProjVel = nodes->GetProjVel(iPoint,Normal);
          su2double SoundSpeed = soundSpeed(*nodes, iPoint) * sqrt(Area2);

          /*--- Adjustment for grid movement ---*/

          if (dynamic_grid) {
            ProjVel -= GeometryToolbox::DotProduct(nDim, Normal, geometry->nodes->GetGridVel(iPoint));
          }

          /*--- Inviscid contribution ---*/

          su2double Lambda = fabs(ProjVel) + SoundSpeed;
          nodes->AddMax_Lambda_Inv(iPoint, Lambda);

          /*--- Viscous contribution ---*/

          if (!viscous) continue;

          Lambda = lambdaVisc(*nodes,iPoint) * Area2;
          nodes->AddMax_Lambda_Visc(iPoint, Lambda);
        }
      }
    }

    /*--- Each element uses their own speed, steady state simulation. ---*/
    {
      /*--- Thread-local variables for min/max reduction. ---*/
      su2double minDt = 1e30, maxDt = 0.0;

      SU2_OMP(for schedule(static,omp_chunk_size) nowait)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

        su2double Vol = geometry->nodes->GetVolume(iPoint);

        if (Vol != 0.0) {
          su2double Local_Delta_Time = nodes->GetLocalCFL(iPoint)*Vol / nodes->GetMax_Lambda_Inv(iPoint);

          if(viscous) {
            su2double dt_visc = nodes->GetLocalCFL(iPoint)*K_v*Vol*Vol / nodes->GetMax_Lambda_Visc(iPoint);
            Local_Delta_Time = min(Local_Delta_Time, dt_visc);
          }

          minDt = min(minDt, Local_Delta_Time);
          maxDt = max(maxDt, Local_Delta_Time);

          nodes->SetDelta_Time(iPoint, min(Local_Delta_Time, config->GetMax_DeltaTime()));
        }
        else {
          nodes->SetDelta_Time(iPoint,0.0);
        }
      }
      /*--- Min/max over threads. ---*/
      SU2_OMP_CRITICAL
      {
        Min_Delta_Time = min(Min_Delta_Time, minDt);
        Max_Delta_Time = max(Max_Delta_Time, maxDt);
        Global_Delta_Time = Min_Delta_Time;
      }
      SU2_OMP_BARRIER
    }

    /*--- Compute the min/max dt (in parallel, now over mpi ranks). ---*/

    SU2_OMP_MASTER
    if (config->GetComm_Level() == COMM_FULL) {
      su2double rbuf_time;
      SU2_MPI::Allreduce(&Min_Delta_Time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
      Min_Delta_Time = rbuf_time;

      SU2_MPI::Allreduce(&Max_Delta_Time, &rbuf_time, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      Max_Delta_Time = rbuf_time;
    }
    SU2_OMP_BARRIER

    /*--- For exact time solution use the minimum delta time of the whole mesh. ---*/
    if (time_stepping) {

      /*--- If the unsteady CFL is set to zero, it uses the defined unsteady time step,
       *    otherwise it computes the time step based on the unsteady CFL. ---*/

      SU2_OMP_MASTER
      {
        if (config->GetUnst_CFL() == 0.0) {
          Global_Delta_Time = config->GetDelta_UnstTime();
        }
        else {
          Global_Delta_Time = Min_Delta_Time;
        }
        Max_Delta_Time = Global_Delta_Time;

        config->SetDelta_UnstTimeND(Global_Delta_Time);
      }
      SU2_OMP_BARRIER

      /*--- Sets the regular CFL equal to the unsteady CFL. ---*/

      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
        nodes->SetLocalCFL(iPoint, config->GetUnst_CFL());
        nodes->SetDelta_Time(iPoint, Global_Delta_Time);
      }

    }

    /*--- Recompute the unsteady time step for the dual time strategy if the unsteady CFL is diferent from 0. ---*/

    if ((dual_time) && (Iteration == 0) && (config->GetUnst_CFL() != 0.0) && (iMesh == MESH_0)) {

      /*--- Thread-local variable for reduction. ---*/
      su2double glbDtND = 1e30;

      SU2_OMP(for schedule(static,omp_chunk_size) nowait)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
        glbDtND = min(glbDtND, config->GetUnst_CFL()*Global_Delta_Time / nodes->GetLocalCFL(iPoint));
      }
      SU2_OMP_CRITICAL
      Global_Delta_UnstTimeND = min(Global_Delta_UnstTimeND, glbDtND);
      SU2_OMP_BARRIER

      SU2_OMP_MASTER
      {
        SU2_MPI::Allreduce(&Global_Delta_UnstTimeND, &glbDtND, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
        Global_Delta_UnstTimeND = glbDtND;

        config->SetDelta_UnstTimeND(Global_Delta_UnstTimeND);
      }
      SU2_OMP_BARRIER
    }

    /*--- The pseudo local time (explicit integration) cannot be greater than the physical time ---*/

    if (dual_time && !implicit) {
      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
        su2double dt = min((2.0/3.0)*config->GetDelta_UnstTimeND(), nodes->GetDelta_Time(iPoint));
        nodes->SetDelta_Time(iPoint, dt);
      }
    }
  }

  /*!
   * \brief Compute the max eigenvalue, gemeric implementation.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \tparam SoundSpeedFunc - Function object to compute speed of sound.
   * \note Functor needs to implement (nodes,iPoint,jPoint) for edges, and (nodes,iPoint) for vertices.
   */
  template<class SoundSpeedFunc>
  FORCEINLINE void SetMax_Eigenvalue_impl(const SoundSpeedFunc& soundSpeed, CGeometry *geometry, const CConfig *config) {

    /*--- Loop domain points. ---*/

    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {

      /*--- Set eigenvalues to zero. ---*/
      nodes->SetLambda(iPoint,0.0);

      /*--- Loop over the neighbors of point i. ---*/
      for (unsigned short iNeigh = 0; iNeigh < geometry->nodes->GetnPoint(iPoint); ++iNeigh)
      {
        auto jPoint = geometry->nodes->GetPoint(iPoint, iNeigh);

        auto iEdge = geometry->nodes->GetEdge(iPoint, iNeigh);
        auto Normal = geometry->edges->GetNormal(iEdge);
        su2double Area = GeometryToolbox::Norm(nDim, Normal);

        /*--- Mean Values ---*/

        su2double Mean_ProjVel = 0.5 * (nodes->GetProjVel(iPoint,Normal) + nodes->GetProjVel(jPoint,Normal));
        su2double Mean_SoundSpeed = soundSpeed(*nodes, iPoint, jPoint) * Area;

        /*--- Adjustment for grid movement ---*/

        if (dynamic_grid) {
          const su2double *GridVel_i = geometry->nodes->GetGridVel(iPoint);
          const su2double *GridVel_j = geometry->nodes->GetGridVel(jPoint);

          for (unsigned short iDim = 0; iDim < nDim; iDim++)
            Mean_ProjVel -= 0.5 * (GridVel_i[iDim] + GridVel_j[iDim]) * Normal[iDim];
        }

        /*--- Inviscid contribution ---*/

        nodes->AddLambda(iPoint, fabs(Mean_ProjVel) + Mean_SoundSpeed);
      }
    }

    /*--- Loop boundary edges ---*/

    for (unsigned short iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
      if ((config->GetMarker_All_KindBC(iMarker) != INTERNAL_BOUNDARY) &&
          (config->GetMarker_All_KindBC(iMarker) != PERIODIC_BOUNDARY)) {

        SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
        for (unsigned long iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

          /*--- Point identification, Normal vector and area ---*/

          auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

          if (!geometry->nodes->GetDomain(iPoint)) continue;

          auto Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
          su2double Area = GeometryToolbox::Norm(nDim, Normal);

          /*--- Mean Values ---*/

          su2double Mean_ProjVel = nodes->GetProjVel(iPoint,Normal);
          su2double Mean_SoundSpeed = soundSpeed(*nodes, iPoint) * Area;

          /*--- Adjustment for grid movement ---*/

          if (dynamic_grid) {
            Mean_ProjVel -= GeometryToolbox::DotProduct(nDim, Normal, geometry->nodes->GetGridVel(iPoint));
          }

          /*--- Inviscid contribution ---*/

          nodes->AddLambda(iPoint, fabs(Mean_ProjVel) + Mean_SoundSpeed);
        }
      }
    }

    /*--- Correct the eigenvalue values across any periodic boundaries. ---*/

    for (unsigned short iPeriodic = 1; iPeriodic <= config->GetnMarker_Periodic()/2; iPeriodic++) {
      InitiatePeriodicComms(geometry, config, iPeriodic, PERIODIC_MAX_EIG);
      CompletePeriodicComms(geometry, config, iPeriodic, PERIODIC_MAX_EIG);
    }

    /*--- MPI parallelization ---*/

    InitiateComms(geometry, config, MAX_EIGENVALUE);
    CompleteComms(geometry, config, MAX_EIGENVALUE);
  }

  /*!
   * \brief Compute the dissipation sensor for centered schemes.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \tparam SensVarFunc - Function object implementing (nodes, iPoint) to return the sensor variable, e.g. pressure.
   */
  template<class SensVarFunc>
  FORCEINLINE void SetCentered_Dissipation_Sensor_impl(const SensVarFunc& sensVar,
                                                       CGeometry *geometry, const CConfig *config) {

    /*--- We can access memory more efficiently if there are no periodic boundaries. ---*/

    const bool isPeriodic = (config->GetnMarker_Periodic() > 0);

    /*--- Loop domain points. ---*/

    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {

      const bool boundary_i = geometry->nodes->GetPhysicalBoundary(iPoint);
      const su2double sensVar_i = sensVar(*nodes, iPoint);

      /*--- Initialize. ---*/
      iPoint_UndLapl[iPoint] = 0.0;
      jPoint_UndLapl[iPoint] = 0.0;

      /*--- Loop over the neighbors of point i. ---*/
      for (auto jPoint : geometry->nodes->GetPoints(iPoint))
      {
        bool boundary_j = geometry->nodes->GetPhysicalBoundary(jPoint);

        /*--- If iPoint is boundary it only takes contributions from other boundary points. ---*/
        if (boundary_i && !boundary_j) continue;

        su2double sensVar_j = sensVar(*nodes, jPoint);

        /*--- Dissipation sensor, add variable difference and variable sum. ---*/
        iPoint_UndLapl[iPoint] += sensVar_j - sensVar_i;
        jPoint_UndLapl[iPoint] += sensVar_j + sensVar_i;
      }

      if (!isPeriodic) {
        /*--- Every neighbor is accounted for, sensor can be computed. ---*/
        nodes->SetSensor(iPoint, fabs(iPoint_UndLapl[iPoint]) / jPoint_UndLapl[iPoint]);
      }
    }

    if (isPeriodic) {
      /*--- Correct the sensor values across any periodic boundaries. ---*/

      for (unsigned short iPeriodic = 1; iPeriodic <= config->GetnMarker_Periodic()/2; iPeriodic++) {
        InitiatePeriodicComms(geometry, config, iPeriodic, PERIODIC_SENSOR);
        CompletePeriodicComms(geometry, config, iPeriodic, PERIODIC_SENSOR);
      }

      /*--- Set final pressure switch for each point ---*/

      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++)
        nodes->SetSensor(iPoint, fabs(iPoint_UndLapl[iPoint]) / jPoint_UndLapl[iPoint]);
    }

    /*--- MPI parallelization ---*/

    InitiateComms(geometry, config, SENSOR);
    CompleteComms(geometry, config, SENSOR);

  }

  /*!
   * \brief Generic implementation of explicit iterations with a preconditioner.
   * \note The preconditioner is a functor implementing the methods:
   *       - compute(config, iPoint): Should prepare the preconditioner for iPoint.
   *       - apply(iVar, residual[], resTruncError[]): Apply it to compute the iVar update.
   *       See Explicit_Iteration for the general form of the preconditioner.
   */
  template<ENUM_TIME_INT IntegrationType, class ResidualPrecond>
  void Explicit_Iteration_impl(ResidualPrecond& preconditioner, CGeometry *geometry,
                               CSolver **solver_container, CConfig *config, unsigned short iRKStep) {

    static_assert(IntegrationType == CLASSICAL_RK4_EXPLICIT ||
                  IntegrationType == RUNGE_KUTTA_EXPLICIT ||
                  IntegrationType == EULER_EXPLICIT, "");

    const bool adjoint = config->GetContinuous_Adjoint();

    const su2double RK_AlphaCoeff = config->Get_Alpha_RKStep(iRKStep);

    /*--- Hard-coded classical RK4 coefficients. Will be added to config. ---*/
    const su2double RK_FuncCoeff[] = {1.0/6.0, 1.0/3.0, 1.0/3.0, 1.0/6.0};
    const su2double RK_TimeCoeff[] = {0.5, 0.5, 1.0, 1.0};

    /*--- Set shared residual variables to 0 and declare
     *    local ones for current thread to work on. ---*/

    SU2_OMP_MASTER
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      SetRes_RMS(iVar, 0.0);
      SetRes_Max(iVar, 0.0, 0);
    }
    SU2_OMP_BARRIER

    su2double resMax[MAXNVAR] = {0.0}, resRMS[MAXNVAR] = {0.0};
    const su2double* coordMax[MAXNVAR] = {nullptr};
    unsigned long idxMax[MAXNVAR] = {0};

    /*--- Update the solution and residuals ---*/

    if (!adjoint) {
      SU2_OMP(for schedule(static,omp_chunk_size) nowait)
      for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {

        su2double Vol = geometry->nodes->GetVolume(iPoint) + geometry->nodes->GetPeriodicVolume(iPoint);
        su2double Delta = nodes->GetDelta_Time(iPoint) / Vol;

        const su2double* Res_TruncError = nodes->GetResTruncError(iPoint);
        const su2double* Residual = LinSysRes.GetBlock(iPoint);

        preconditioner.compute(config, iPoint);

        for (unsigned short iVar = 0; iVar < nVar; iVar++) {

          su2double Res = preconditioner.apply(iVar, Residual, Res_TruncError);

          /*--- "Static" switch which should be optimized at compile time. ---*/
          switch(IntegrationType) {

            case EULER_EXPLICIT:
              nodes->AddSolution(iPoint,iVar, -Res*Delta);
              break;

            case RUNGE_KUTTA_EXPLICIT:
              nodes->AddSolution(iPoint, iVar, -Res*Delta*RK_AlphaCoeff);
              break;

            case CLASSICAL_RK4_EXPLICIT:
            {
              su2double tmp_time = -1.0*RK_TimeCoeff[iRKStep]*Delta;
              su2double tmp_func = -1.0*RK_FuncCoeff[iRKStep]*Delta;

              if (iRKStep < 3) {
                /* Base Solution Update */
                nodes->AddSolution(iPoint,iVar, tmp_time*Res);

                /* New Solution Update */
                nodes->AddSolution_New(iPoint,iVar, tmp_func*Res);
              } else {
                nodes->SetSolution(iPoint, iVar, nodes->GetSolution_New(iPoint, iVar) + tmp_func*Res);
              }
            }
            break;
          }

          /*--- Update residual information for current thread. ---*/
          resRMS[iVar] += Res*Res;
          if (fabs(Res) > resMax[iVar]) {
            resMax[iVar] = fabs(Res);
            idxMax[iVar] = iPoint;
            coordMax[iVar] = geometry->nodes->GetCoord(iPoint);
          }
        }
      }
      /*--- Reduce residual information over all threads in this rank. ---*/
      SU2_OMP_CRITICAL
      for (unsigned short iVar = 0; iVar < nVar; iVar++) {
        AddRes_RMS(iVar, resRMS[iVar]);
        AddRes_Max(iVar, resMax[iVar], geometry->nodes->GetGlobalIndex(idxMax[iVar]), coordMax[iVar]);
      }
      SU2_OMP_BARRIER
    }

    /*--- MPI solution ---*/

    InitiateComms(geometry, config, SOLUTION);
    CompleteComms(geometry, config, SOLUTION);

    if (!adjoint) {
      SU2_OMP_MASTER {
        /*--- Compute the root mean square residual ---*/

        SetResidual_RMS(geometry, config);

        /*--- For verification cases, compute the global error metrics. ---*/

        ComputeVerificationError(geometry, config);
      }
      SU2_OMP_BARRIER
    }

  }

  /*!
   * \brief Generic implementation of explicit iterations without preconditioner.
   */
  template<ENUM_TIME_INT IntegrationType>
  FORCEINLINE void Explicit_Iteration(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iRKStep) {
    struct Identity {
      FORCEINLINE void compute(const CConfig*, unsigned long) {}
      FORCEINLINE su2double apply(unsigned short iVar, const su2double* res, const su2double* resTrunc) {
        return res[iVar] + resTrunc[iVar];
      }
    } precond;

    Explicit_Iteration_impl<IntegrationType>(precond, geometry, solver_container, config, iRKStep);
  }

  /*!
   * \brief Generic implementation to prepare an implicit iteration with an optional preconditioner applied to the diagonal.
   * \tparam DiagonalPrecond - A function object implementing:
   *         - active: A boolean variable to determine if the preconditioner should be used.
   *         - (config, iPoint, delta): Compute and return a matrix type compatible with the Jacobian matrix,
   *           where "delta" is V/dt.
   */
  template<class DiagonalPrecond>
  void PrepareImplicitIteration_impl(DiagonalPrecond& preconditioner, CGeometry *geometry, CConfig *config) {

    const bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

    /*--- Set shared residual variables to 0 and declare local ones for current thread to work on. ---*/

    SU2_OMP_MASTER
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      SetRes_RMS(iVar, 0.0);
      SetRes_Max(iVar, 0.0, 0);
    }
    SU2_OMP_BARRIER

    su2double resMax[MAXNVAR] = {0.0}, resRMS[MAXNVAR] = {0.0};
    const su2double* coordMax[MAXNVAR] = {nullptr};
    unsigned long idxMax[MAXNVAR] = {0};

    /*--- Add pseudotime term to Jacobian. ---*/

    if (implicit) {
      SU2_OMP(for schedule(static,omp_chunk_size) nowait)
      for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {

        /*--- Modify matrix diagonal to improve diagonal dominance. ---*/

        if (nodes->GetDelta_Time(iPoint) != 0.0) {

          su2double Vol = geometry->nodes->GetVolume(iPoint) + geometry->nodes->GetPeriodicVolume(iPoint);

          su2double Delta = Vol / nodes->GetDelta_Time(iPoint);

          if (preconditioner.active)
            Jacobian.AddBlock2Diag(iPoint, preconditioner(config, iPoint, Delta));
          else
            Jacobian.AddVal2Diag(iPoint, Delta);
        }
        else {
          Jacobian.SetVal2Diag(iPoint, 1.0);
        }
      }
    }

    /*--- Right hand side of the system (-Residual) and initial guess (x = 0) ---*/

    SU2_OMP(for schedule(static,omp_chunk_size) nowait)
    for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {

      /*--- Multigrid contribution to residual. ---*/

      su2double* local_Res_TruncError = nodes->GetResTruncError(iPoint);

      if (nodes->GetDelta_Time(iPoint) == 0.0) {
        for (unsigned short iVar = 0; iVar < nVar; iVar++) {
          LinSysRes(iPoint,iVar) = 0.0;
          local_Res_TruncError[iVar] = 0.0;
        }
      }

      for (unsigned short iVar = 0; iVar < nVar; iVar++) {
        unsigned long total_index = iPoint*nVar + iVar;
        LinSysRes[total_index] = - (LinSysRes[total_index] + local_Res_TruncError[iVar]);
        LinSysSol[total_index] = 0.0;

        su2double Res = fabs(LinSysRes[total_index]);
        resRMS[iVar] += Res*Res;
        if (Res > resMax[iVar]) {
          resMax[iVar] = Res;
          idxMax[iVar] = iPoint;
          coordMax[iVar] = geometry->nodes->GetCoord(iPoint);
        }
      }
    }
    SU2_OMP_CRITICAL
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      AddRes_RMS(iVar, resRMS[iVar]);
      AddRes_Max(iVar, resMax[iVar], geometry->nodes->GetGlobalIndex(idxMax[iVar]), coordMax[iVar]);
    }
    SU2_OMP_BARRIER

    /*--- Compute the root mean square residual ---*/
    SU2_OMP_MASTER
    SetResidual_RMS(geometry, config);
    SU2_OMP_BARRIER
  }

  /*!
   * \brief Generic implementation to complete an implicit iteration, i.e. update the solution.
   * \tparam compute_ur - Whether to use automatic under-relaxation for the update.
   */
  template<bool compute_ur>
  void CompleteImplicitIteration_impl(CGeometry *geometry, CConfig *config) {

    if (compute_ur) ComputeUnderRelaxationFactor(config);

    /*--- Update solution with under-relaxation and communicate it. ---*/

    if (!config->GetContinuous_Adjoint()) {
      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {
        for (unsigned short iVar = 0; iVar < nVar; iVar++) {
          nodes->AddSolution(iPoint, iVar, nodes->GetUnderRelaxation(iPoint)*LinSysSol[iPoint*nVar+iVar]);
        }
      }
    }

    for (unsigned short iPeriodic = 1; iPeriodic <= config->GetnMarker_Periodic()/2; iPeriodic++) {
      InitiatePeriodicComms(geometry, config, iPeriodic, PERIODIC_IMPLICIT);
      CompletePeriodicComms(geometry, config, iPeriodic, PERIODIC_IMPLICIT);
    }

    InitiateComms(geometry, config, SOLUTION);
    CompleteComms(geometry, config, SOLUTION);

    /*--- For verification cases, compute the global error metrics. ---*/
    SU2_OMP_MASTER
    ComputeVerificationError(geometry, config);
    SU2_OMP_BARRIER
  }

  /*!
   * \brief Evaluate the vorticity and strain rate magnitude.
   * \tparam VelocityOffset: Index in the primitive variables where the velocity starts.
   */
  template<size_t VelocityOffset>
  void ComputeVorticityAndStrainMag(const CConfig& config, unsigned short iMesh) {

    const auto& Gradient_Primitive = nodes->GetGradient_Primitive();
    auto& StrainMag = nodes->GetStrainMag();

    SU2_OMP_MASTER {
      StrainMag_Max = 0.0;
      Omega_Max = 0.0;
    }
    SU2_OMP_BARRIER

    su2double strainMax = 0.0, omegaMax = 0.0;

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {

      constexpr size_t u = VelocityOffset;
      constexpr size_t v = VelocityOffset+1;
      constexpr size_t w = VelocityOffset+2;

      /*--- Vorticity ---*/

      su2double* Vorticity = nodes->GetVorticity(iPoint);

      Vorticity[0] = 0.0; Vorticity[1] = 0.0;

      Vorticity[2] = Gradient_Primitive(iPoint,v,0)-Gradient_Primitive(iPoint,u,1);

      if (nDim == 3) {
        Vorticity[0] = Gradient_Primitive(iPoint,w,1)-Gradient_Primitive(iPoint,v,2);
        Vorticity[1] = -(Gradient_Primitive(iPoint,w,0)-Gradient_Primitive(iPoint,u,2));
      }

      /*--- Strain Magnitude ---*/

      AD::StartPreacc();
      AD::SetPreaccIn(&Gradient_Primitive[iPoint][VelocityOffset], nDim, nDim);

      su2double Div = 0.0;
      for (unsigned long iDim = 0; iDim < nDim; iDim++)
        Div += Gradient_Primitive(iPoint, iDim+VelocityOffset, iDim);
      Div /= 3.0;

      StrainMag(iPoint) = 0.0;

      /*--- Add diagonal part ---*/

      for (unsigned long iDim = 0; iDim < nDim; iDim++) {
        StrainMag(iPoint) += pow(Gradient_Primitive(iPoint, iDim+VelocityOffset, iDim) - Div, 2);
      }
      if (nDim == 2) {
        StrainMag(iPoint) += pow(Div, 2);
      }

      /*--- Add off diagonals ---*/

      StrainMag(iPoint) += 2.0*pow(0.5*(Gradient_Primitive(iPoint,u,1) + Gradient_Primitive(iPoint,v,0)), 2);

      if (nDim == 3) {
        StrainMag(iPoint) += 2.0*pow(0.5*(Gradient_Primitive(iPoint,u,2) + Gradient_Primitive(iPoint,w,0)), 2);
        StrainMag(iPoint) += 2.0*pow(0.5*(Gradient_Primitive(iPoint,v,2) + Gradient_Primitive(iPoint,w,1)), 2);
      }

      StrainMag(iPoint) = sqrt(2.0*StrainMag(iPoint));
      AD::SetPreaccOut(StrainMag(iPoint));

      /*--- Max is not differentiable, so we not register them for preacc. ---*/
      strainMax = max(strainMax, StrainMag(iPoint));
      omegaMax = max(omegaMax, GeometryToolbox::Norm(3, Vorticity));

      AD::EndPreacc();
    }

    if ((iMesh == MESH_0) && (config.GetComm_Level() == COMM_FULL)) {
      SU2_OMP_CRITICAL {
        StrainMag_Max = max(StrainMag_Max, strainMax);
        Omega_Max = max(Omega_Max, omegaMax);
      }

      SU2_OMP_BARRIER
      SU2_OMP_MASTER {
        su2double MyOmega_Max = Omega_Max;
        su2double MyStrainMag_Max = StrainMag_Max;

        SU2_MPI::Allreduce(&MyStrainMag_Max, &StrainMag_Max, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&MyOmega_Max, &Omega_Max, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      }
      SU2_OMP_BARRIER
    }

  }

  /*!
   * \brief Destructor.
   */
  ~CFVMFlowSolverBase();

 public:

  /*!
   * \brief Load a solution from a restart file.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver - Container vector with all of the solvers.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iter - Current external iteration number.
   * \param[in] update_geo - Flag for updating coords and grid velocity.
   */
  void LoadRestart(CGeometry **geometry, CSolver ***solver, CConfig *config, int iter, bool update_geo) override;

  /*!
   * \brief Set the initial condition for the Euler Equations.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container with all the solutions.
   * \param[in] config - Definition of the particular problem.
   * \param[in] ExtIter - External iteration.
   */
  void SetInitialCondition(CGeometry **geometry, CSolver ***solver_container, CConfig *config, unsigned long ExtIter) override;

  /*!
   * \brief Compute the gradient of the primitive variables using Green-Gauss method,
   *        and stores the result in the <i>Gradient_Primitive</i> variable.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \param[in] reconstruction - indicator that the gradient being computed is for upwind reconstruction.
   */
  void SetPrimitive_Gradient_GG(CGeometry* geometry, const CConfig* config, bool reconstruction = false) override;

  /*!
   * \brief Compute the gradient of the primitive variables using a Least-Squares method,
   *        and stores the result in the <i>Gradient_Primitive</i> variable.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   * \param[in] reconstruction - indicator that the gradient being computed is for upwind reconstruction.
   */
  void SetPrimitive_Gradient_LS(CGeometry* geometry, const CConfig* config, bool reconstruction = false) override;

  /*!
   * \brief Compute the limiter of the primitive variables.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  void SetPrimitive_Limiter(CGeometry* geometry, const CConfig* config) final;

  /*!
   * \brief Implementation of implicit Euler iteration.
   */
  void ImplicitEuler_Iteration(CGeometry *geometry, CSolver **solver_container, CConfig *config) final;

  /*!
   * \brief Set the total residual adding the term that comes from the Dual Time Strategy.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] config - Definition of the particular problem.
   * \param[in] iRKStep - Current step of the Runge-Kutta iteration.
   * \param[in] iMesh - Index of the mesh in multigrid computations.
   * \param[in] RunTime_EqSystem - System of equations which is going to be solved.
   */
  void SetResidual_DualTime(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iRKStep,
                            unsigned short iMesh, unsigned short RunTime_EqSystem) override;

  /*!
   * \brief Set a uniform inlet profile
   *
   * The values at the inlet are set to match the values specified for
   * inlets in the configuration file.
   *
   * \param[in] config - Definition of the particular problem.
   * \param[in] iMarker - Surface marker where the coefficient is computed.
   */
  void SetUniformInlet(const CConfig* config, unsigned short iMarker) final;

  /*!
   * \brief Store of a set of provided inlet profile values at a vertex.
   * \param[in] val_inlet - vector containing the inlet values for the current vertex.
   * \param[in] iMarker - Surface marker where the coefficient is computed.
   * \param[in] iVertex - Vertex of the marker <i>iMarker</i> where the inlet is being set.
   */
  void SetInletAtVertex(const su2double* val_inlet, unsigned short iMarker, unsigned long iVertex) final;

  /*!
   * \brief Get the set of value imposed at an inlet.
   * \param[in] val_inlet - vector returning the inlet values for the current vertex.
   * \param[in] val_inlet_point - Node index where the inlet is being set.
   * \param[in] val_kind_marker - Enumerated type for the particular inlet type.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param config - Definition of the particular problem.
   * \return Value of the face area at the vertex.
   */
  su2double GetInletAtVertex(su2double* val_inlet, unsigned long val_inlet_point, unsigned short val_kind_marker,
                             string val_marker, const CGeometry* geometry, const CConfig* config) const final;

  /*!
   * \author T. Kattmann
   * \brief Impose via the residual the Euler wall boundary condition.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  inline void BC_Euler_Wall(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                            CNumerics* visc_numerics, CConfig* config, unsigned short val_marker) final {
    /*--- Call the equivalent symmetry plane boundary condition. ---*/
    BC_Sym_Plane(geometry, solver_container, conv_numerics, visc_numerics, config, val_marker);
  }

  /*!
   * \author T. Kattmann
   * \brief Impose the symmetry boundary condition using the residual.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   * \param[in] val_marker - Surface marker where the boundary condition is applied.
   */
  void BC_Sym_Plane(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics, CNumerics* visc_numerics,
                    CConfig* config, unsigned short val_marker) override;

  /*!
   * \brief Impose a periodic boundary condition by summing contributions from the complete control volume.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   */
  void BC_Periodic(CGeometry* geometry, CSolver** solver_container, CNumerics* numerics, CConfig* config) final;

  /*!
   * \brief Impose the interface state across sliding meshes.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics - Description of the numerical method.
   * \param[in] visc_numerics - Description of the numerical method.
   * \param[in] config - Definition of the particular problem.
   */
  void BC_Fluid_Interface(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                          CNumerics* visc_numerics, CConfig* config) final;

  /*!
   * \brief Impose a custom or verification boundary condition.
   * \param[in] geometry         - Geometrical definition of the problem.
   * \param[in] solver_container - Container vector with all the solutions.
   * \param[in] conv_numerics    - Description of the convective numerical method.
   * \param[in] visc_numerics    - Description of the viscous numerical method.
   * \param[in] config           - Definition of the particular problem.
   * \param[in] val_marker       - Surface marker where the boundary condition is applied.
   */
  void BC_Custom(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics, CNumerics* visc_numerics,
                 CConfig* config, unsigned short val_marker) final;

  /*!
   * \brief Compute the density at the infinity.
   * \return Value of the density at the infinity.
   */
  inline su2double GetDensity_Inf(void) const final { return Density_Inf; }

  /*!
   * \brief Compute 2-norm of the velocity at the infinity.
   * \return Value of the 2-norm of the velocity at the infinity.
   */
  inline su2double GetModVelocity_Inf(void) const final { return GeometryToolbox::Norm(nDim, Velocity_Inf); }

  /*!
   * \brief Compute the pressure at the infinity.
   * \return Value of the pressure at the infinity.
   */
  inline su2double GetPressure_Inf(void) const final { return Pressure_Inf; }

  /*!
   * \brief Get the temperature value at infinity.
   * \return Value of the temperature at infinity.
   */
  inline su2double GetTemperature_Inf(void) const { return Temperature_Inf; }

  /*!
   * \brief Compute the density multiply by velocity at the infinity.
   * \param[in] val_dim - Index of the velocity vector.
   * \return Value of the density multiply by the velocity at the infinity.
   */
  inline su2double GetDensity_Velocity_Inf(unsigned short val_dim) const final {
    return Density_Inf * Velocity_Inf[val_dim];
  }

  /*!
   * \brief Get the velocity at the infinity.
   * \param[in] val_dim - Index of the velocity vector.
   * \return Value of the velocity at the infinity.
   */
  inline su2double GetVelocity_Inf(unsigned short val_dim) const final { return Velocity_Inf[val_dim]; }

  /*!
   * \brief Get the velocity at the infinity.
   * \return Value of the velocity at the infinity.
   */
  inline su2double* GetVelocity_Inf(void) final { return Velocity_Inf; }

  /*!
   * \brief Set the velocity at infinity.
   * \param[in] val_dim - Index of the velocity vector.
   * \param[in] val_velocity - Value of the velocity.
   */
  inline void SetVelocity_Inf(unsigned short val_dim, su2double val_velocity) final {
    Velocity_Inf[val_dim] = val_velocity;
  }

  /*!
   * \brief Compute the density multiply by energy at the infinity.
   * \return Value of the density multiply by  energy at the infinity.
   */
  inline su2double GetDensity_Energy_Inf(void) const final { return Density_Inf * Energy_Inf; }

  /*!
   * \brief Set the freestream pressure.
   * \param[in] Value of freestream pressure.
   */
  inline void SetPressure_Inf(su2double p_inf) final { Pressure_Inf = p_inf; }

  /*!
   * \brief Set the freestream temperature.
   * \param[in] Value of freestream temperature.
   */
  inline void SetTemperature_Inf(su2double t_inf) final { Temperature_Inf = t_inf; }

  /*!
   * \brief Set the freestream temperature.
   * \param[in] Value of freestream temperature.
   */
  inline void SetDensity_Inf(su2double rho_inf) final { Density_Inf = rho_inf; }

  /*!
   * \brief Compute the viscosity at the infinity.
   * \return Value of the viscosity at the infinity.
   */
  inline su2double GetViscosity_Inf(void) const final { return Viscosity_Inf; }

  /*!
   * \brief Get the turbulent kinetic energy at the infinity.
   * \return Value of the turbulent kinetic energy at the infinity.
   */
  inline su2double GetTke_Inf(void) const final { return Tke_Inf; }

  /*!
   * \brief Get the max Omega.
   * \return Value of the max Omega.
   */
  inline su2double GetOmega_Max(void) const final { return Omega_Max; }

  /*!
   * \brief Get the max Strain rate magnitude.
   * \return Value of the max Strain rate magnitude.
   */
  inline su2double GetStrainMag_Max(void) const final { return StrainMag_Max; }

  /*!
   * \brief Provide the non dimensional lift coefficient (inviscid contribution).
   * \param val_marker Surface where the coefficient is going to be computed.
   * \return Value of the lift coefficient (inviscid contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCL_Inv(unsigned short val_marker) const final { return InvCoeff.CL[val_marker]; }

  /*!
   * \brief Provide the non dimensional drag coefficient (inviscid contribution).
   * \param val_marker Surface where the coeficient is going to be computed.
   * \return Value of the drag coefficient (inviscid contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCD_Inv(unsigned short val_marker) const final { return InvCoeff.CD[val_marker]; }

  /*!
   * \brief Provide the non dimensional lift coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the lift coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CL(unsigned short val_marker) const final { return SurfaceCoeff.CL[val_marker]; }

  /*!
   * \brief Provide the non dimensional drag coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the drag coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CD(unsigned short val_marker) const final { return SurfaceCoeff.CD[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CSF(unsigned short val_marker) const final { return SurfaceCoeff.CSF[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CEff(unsigned short val_marker) const final { return SurfaceCoeff.CEff[val_marker]; }

  /*!
   * \brief Provide the non dimensional x force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFx(unsigned short val_marker) const final { return SurfaceCoeff.CFx[val_marker]; }

  /*!
   * \brief Provide the non dimensional y force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFy(unsigned short val_marker) const final { return SurfaceCoeff.CFy[val_marker]; }

  /*!
   * \brief Provide the non dimensional z force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFz(unsigned short val_marker) const final { return SurfaceCoeff.CFz[val_marker]; }

  /*!
   * \brief Provide the non dimensional x moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMx(unsigned short val_marker) const final { return SurfaceCoeff.CMx[val_marker]; }

  /*!
   * \brief Provide the non dimensional y moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMy(unsigned short val_marker) const final { return SurfaceCoeff.CMy[val_marker]; }

  /*!
   * \brief Provide the non dimensional z moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMz(unsigned short val_marker) const final { return SurfaceCoeff.CMz[val_marker]; }

  /*!
   * \brief Provide the non dimensional lift coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the lift coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CL_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CL[val_marker]; }

  /*!
   * \brief Provide the non dimensional drag coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the drag coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CD_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CD[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CSF_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CSF[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CEff_Inv(unsigned short val_marker) const final {
    return SurfaceInvCoeff.CEff[val_marker];
  }

  /*!
   * \brief Provide the non dimensional x force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFx_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CFx[val_marker]; }

  /*!
   * \brief Provide the non dimensional y force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFy_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CFy[val_marker]; }

  /*!
   * \brief Provide the non dimensional z force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFz_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CFz[val_marker]; }

  /*!
   * \brief Provide the non dimensional x moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMx_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CMx[val_marker]; }

  /*!
   * \brief Provide the non dimensional y moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMy_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CMy[val_marker]; }

  /*!
   * \brief Provide the non dimensional z moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMz_Inv(unsigned short val_marker) const final { return SurfaceInvCoeff.CMz[val_marker]; }

  /*!
   * \brief Provide the non dimensional lift coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the lift coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CL_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CL[val_marker]; }

  /*!
   * \brief Provide the non dimensional drag coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the drag coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CD_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CD[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CSF_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CSF[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CEff_Mnt(unsigned short val_marker) const final {
    return SurfaceMntCoeff.CEff[val_marker];
  }

  /*!
   * \brief Provide the non dimensional x force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFx_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CFx[val_marker]; }

  /*!
   * \brief Provide the non dimensional y force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFy_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CFy[val_marker]; }

  /*!
   * \brief Provide the non dimensional z force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFz_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CFz[val_marker]; }

  /*!
   * \brief Provide the non dimensional x moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMx_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CMx[val_marker]; }

  /*!
   * \brief Provide the non dimensional y moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMy_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CMy[val_marker]; }

  /*!
   * \brief Provide the non dimensional z moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMz_Mnt(unsigned short val_marker) const final { return SurfaceMntCoeff.CMz[val_marker]; }

  /*!
   * \brief Provide the non dimensional sideforce coefficient (inviscid contribution).
   * \param val_marker Surface where the coeficient is going to be computed.
   * \return Value of the sideforce coefficient (inviscid contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCSF_Inv(unsigned short val_marker) const final { return InvCoeff.CSF[val_marker]; }

  /*!
   * \brief Provide the non dimensional efficiency coefficient (inviscid contribution).
   * \param val_marker Surface where the coeficient is going to be computed.
   * \return Value of the efficiency coefficient (inviscid contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCEff_Inv(unsigned short val_marker) const final { return InvCoeff.CEff[val_marker]; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional sideforce coefficient.
   * \return Value of the sideforce coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CSF() const final { return TotalCoeff.CSF; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CEff() const final { return TotalCoeff.CEff; }

  /*!
   * \brief Get the reference force used to compute CL, CD, etc.
   */
  inline su2double GetAeroCoeffsReferenceForce() const final { return AeroCoeffForceRef; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional lift coefficient.
   * \return Value of the lift coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CL() const final { return TotalCoeff.CL; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional drag coefficient.
   * \return Value of the drag coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CD() const final { return TotalCoeff.CD; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional x moment coefficient.
   * \return Value of the moment x coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CMx() const final { return TotalCoeff.CMx; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional y moment coefficient.
   * \return Value of the moment y coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CMy() const final { return TotalCoeff.CMy; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional z moment coefficient.
   * \return Value of the moment z coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CMz() const final { return TotalCoeff.CMz; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional x moment coefficient.
   * \return Value of the moment x coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CoPx() const final { return TotalCoeff.CoPx; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional y moment coefficient.
   * \return Value of the moment y coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CoPy() const final { return TotalCoeff.CoPy; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional z moment coefficient.
   * \return Value of the moment z coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CoPz() const final { return TotalCoeff.CoPz; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional x force coefficient.
   * \return Value of the force x coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CFx() const final { return TotalCoeff.CFx; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional y force coefficient.
   * \return Value of the force y coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CFy() const final { return TotalCoeff.CFy; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional z force coefficient.
   * \return Value of the force z coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CFz() const final { return TotalCoeff.CFz; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional thrust coefficient.
   * \return Value of the rotor efficiency coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CT() const final { return TotalCoeff.CT; }

  /*!
   * \brief Store the total (inviscid + viscous) non dimensional thrust coefficient.
   * \param[in] val_Total_CT - Value of the total thrust coefficient.
   */
  inline void SetTotal_CT(su2double val_Total_CT) final { TotalCoeff.CT = val_Total_CT; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional torque coefficient.
   * \return Value of the rotor efficiency coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CQ() const final { return TotalCoeff.CQ; }

  /*!
   * \brief Store the total (inviscid + viscous) non dimensional torque coefficient.
   * \param[in] val_Total_CQ - Value of the total torque coefficient.
   */
  inline void SetTotal_CQ(su2double val_Total_CQ) final { TotalCoeff.CQ = val_Total_CQ; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional rotor Figure of Merit.
   * \return Value of the rotor efficiency coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CMerit() const final { return TotalCoeff.CMerit; }

  /*!
   * \brief Store the total (inviscid + viscous) non dimensional drag coefficient.
   * \param[in] val_Total_CD - Value of the total drag coefficient.
   */
  inline void SetTotal_CD(su2double val_Total_CD) final { TotalCoeff.CD = val_Total_CD; }

  /*!
   * \brief Store the total (inviscid + viscous) non dimensional lift coefficient.
   * \param[in] val_Total_CL - Value of the total lift coefficient.
   */
  inline void SetTotal_CL(su2double val_Total_CL) final { TotalCoeff.CL = val_Total_CL; }

  /*!
   * \brief Get the inviscid contribution to the lift coefficient.
   * \return Value of the lift coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CL_Inv() const final { return AllBoundInvCoeff.CL; }

  /*!
   * \brief Get the inviscid contribution to the drag coefficient.
   * \return Value of the drag coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CD_Inv() const final { return AllBoundInvCoeff.CD; }

  /*!
   * \brief Get the inviscid contribution to the sideforce coefficient.
   * \return Value of the sideforce coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CSF_Inv() const final { return AllBoundInvCoeff.CSF; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CEff_Inv() const final { return AllBoundInvCoeff.CEff; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMx_Inv() const final { return AllBoundInvCoeff.CMx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMy_Inv() const final { return AllBoundInvCoeff.CMy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMz_Inv() const final { return AllBoundInvCoeff.CMz; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPx_Inv() const final { return AllBoundInvCoeff.CoPx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPy_Inv() const final { return AllBoundInvCoeff.CoPy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPz_Inv() const final { return AllBoundInvCoeff.CoPz; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFx_Inv() const final { return AllBoundInvCoeff.CFx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFy_Inv() const final { return AllBoundInvCoeff.CFy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFz_Inv() const final { return AllBoundInvCoeff.CFz; }

  /*!
   * \brief Get the inviscid contribution to the lift coefficient.
   * \return Value of the lift coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CL_Mnt() const final { return AllBoundMntCoeff.CL; }

  /*!
   * \brief Get the inviscid contribution to the drag coefficient.
   * \return Value of the drag coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CD_Mnt() const final { return AllBoundMntCoeff.CD; }

  /*!
   * \brief Get the inviscid contribution to the sideforce coefficient.
   * \return Value of the sideforce coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CSF_Mnt() const final { return AllBoundMntCoeff.CSF; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CEff_Mnt() const final { return AllBoundMntCoeff.CEff; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMx_Mnt() const final { return AllBoundMntCoeff.CMx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMy_Mnt() const final { return AllBoundMntCoeff.CMy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMz_Mnt() const final { return AllBoundMntCoeff.CMz; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPx_Mnt() const final { return AllBoundMntCoeff.CoPx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPy_Mnt() const final { return AllBoundMntCoeff.CoPy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPz_Mnt() const final { return AllBoundMntCoeff.CoPz; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFx_Mnt() const final { return AllBoundMntCoeff.CFx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFy_Mnt() const final { return AllBoundMntCoeff.CFy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFz_Mnt() const final { return AllBoundMntCoeff.CFz; }

  /*!
   * \brief Provide the non dimensional lift coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the lift coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CL_Visc(unsigned short val_marker) const final { return SurfaceViscCoeff.CL[val_marker]; }

  /*!
   * \brief Provide the non dimensional drag coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the drag coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CD_Visc(unsigned short val_marker) const final { return SurfaceViscCoeff.CD[val_marker]; }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CSF_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CSF[val_marker];
  }

  /*!
   * \brief Provide the non dimensional side-force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the side-force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CEff_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CEff[val_marker];
  }

  /*!
   * \brief Provide the non dimensional x force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFx_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CFx[val_marker];
  }

  /*!
   * \brief Provide the non dimensional y force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFy_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CFy[val_marker];
  }

  /*!
   * \brief Provide the non dimensional z force coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z force coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CFz_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CFz[val_marker];
  }

  /*!
   * \brief Provide the non dimensional x moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the x moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMx_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CMx[val_marker];
  }

  /*!
   * \brief Provide the non dimensional y moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the y moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMy_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CMy[val_marker];
  }

  /*!
   * \brief Provide the non dimensional z moment coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the z moment coefficient on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_CMz_Visc(unsigned short val_marker) const final {
    return SurfaceViscCoeff.CMz[val_marker];
  }

  /*!
   * \brief Get the inviscid contribution to the lift coefficient.
   * \return Value of the lift coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CL_Visc() const final { return AllBoundViscCoeff.CL; }

  /*!
   * \brief Get the inviscid contribution to the drag coefficient.
   * \return Value of the drag coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CD_Visc() const final { return AllBoundViscCoeff.CD; }

  /*!
   * \brief Get the inviscid contribution to the sideforce coefficient.
   * \return Value of the sideforce coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CSF_Visc() const final { return AllBoundViscCoeff.CSF; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CEff_Visc() const final { return AllBoundViscCoeff.CEff; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMx_Visc() const final { return AllBoundViscCoeff.CMx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMy_Visc() const final { return AllBoundViscCoeff.CMy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CMz_Visc() const final { return AllBoundViscCoeff.CMz; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPx_Visc() const final { return AllBoundViscCoeff.CoPx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPy_Visc() const final { return AllBoundViscCoeff.CoPy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CoPz_Visc() const final { return AllBoundViscCoeff.CoPz; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFx_Visc() const final { return AllBoundViscCoeff.CFx; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFy_Visc() const final { return AllBoundViscCoeff.CFy; }

  /*!
   * \brief Get the inviscid contribution to the efficiency coefficient.
   * \return Value of the efficiency coefficient (inviscid contribution).
   */
  inline su2double GetAllBound_CFz_Visc() const final { return AllBoundViscCoeff.CFz; }

  /*!
   * \brief Get the non dimensional lift coefficient (viscous contribution).
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the lift coefficient (viscous contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCL_Visc(unsigned short val_marker) const final { return ViscCoeff.CL[val_marker]; }

  /*!
   * \brief Get the non dimensional sideforce coefficient (viscous contribution).
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the sideforce coefficient (viscous contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCSF_Visc(unsigned short val_marker) const final { return ViscCoeff.CSF[val_marker]; }

  /*!
   * \brief Get the non dimensional drag coefficient (viscous contribution).
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \return Value of the drag coefficient (viscous contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetCD_Visc(unsigned short val_marker) const final { return ViscCoeff.CD[val_marker]; }

  /*!
   * \brief Get the total heat flux.
   * \param[in] val_marker - Surface marker where the heat flux is computed.
   * \return Value of the integrated heat flux (viscous contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_HF_Visc(unsigned short val_marker) const final { return Surface_HF_Visc[val_marker]; }

  /*!
   * \brief Get the maximum (per surface) heat flux.
   * \param[in] val_marker - Surface marker where the heat flux is computed.
   * \return Value of the maximum heat flux (viscous contribution) on the surface <i>val_marker</i>.
   */
  inline su2double GetSurface_MaxHF_Visc(unsigned short val_marker) const final {
    return Surface_MaxHF_Visc[val_marker];
  }

  /*!
   * \brief Set the value of the Equivalent Area coefficient.
   * \param[in] val_cequivarea - Value of the Equivalent Area coefficient.
   */
  inline void SetTotal_CpDiff(su2double val_pressure) final { Total_CpDiff = val_pressure; }

  /*!
   * \brief Set the value of the Equivalent Area coefficient.
   * \param[in] val_cequivarea - Value of the Equivalent Area coefficient.
   */
  inline void SetTotal_HeatFluxDiff(su2double val_heat) final { Total_HeatFluxDiff = val_heat; }

  //zhen: for temperature inverse design
  inline void SetTotal_TemDiff(su2double val_tem) final { Total_TemDiff = val_tem; }

  /*!
   * \brief Set the value of the Near-Field pressure oefficient.
   * \param[in] val_cnearfieldpress - Value of the Near-Field pressure coefficient.
   */
  inline void SetTotal_CNearFieldOF(su2double val_cnearfieldpress) final { Total_CNearFieldOF = val_cnearfieldpress; }

  /*!
   * \author H. Kline
   * \brief Set the total "combo" objective (weighted sum of other values).
   * \param[in] ComboObj - Value of the combined objective.
   */
  inline void SetTotal_ComboObj(su2double ComboObj) final { Total_ComboObj = ComboObj; }

  /*!
   * \author H. Kline
   * \brief Provide the total "combo" objective (weighted sum of other values).
   * \return Value of the "combo" objective values.
   */
  inline su2double GetTotal_ComboObj() const final { return Total_ComboObj; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional Equivalent Area coefficient.
   * \return Value of the Equivalent Area coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_CpDiff() const final { return Total_CpDiff; }

  /*!
   * \brief Provide the total (inviscid + viscous) non dimensional Equivalent Area coefficient.
   * \return Value of the Equivalent Area coefficient (inviscid + viscous contribution).
   */
  inline su2double GetTotal_HeatFluxDiff() const final { return Total_HeatFluxDiff; }

  //zhen: for temperature inverse design
  inline su2double GetTotal_TemDiff() const final { return Total_TemDiff; }

  /*!
   * \brief Set the value of the custom objective function.
   * \param[in] val_Total_Custom_ObjFunc - Value of the total custom objective function.
   * \param[in] val_weight - Value of the weight for the custom objective function.
   */
  inline void SetTotal_Custom_ObjFunc(su2double val_total_custom_objfunc, su2double val_weight) final {
    Total_Custom_ObjFunc = val_total_custom_objfunc * val_weight;
  }

  /*!
   * \brief Add the value of the custom objective function.
   * \param[in] val_Total_Custom_ObjFunc - Value of the total custom objective function.
   * \param[in] val_weight - Value of the weight for the custom objective function.
   */
  inline void AddTotal_Custom_ObjFunc(su2double val_total_custom_objfunc, su2double val_weight) final {
    Total_Custom_ObjFunc += val_total_custom_objfunc * val_weight;
  }

  /*!
   * \brief Provide the total heat load.
   * \return Value of the heat load (viscous contribution).
   */
  inline su2double GetTotal_HeatFlux(void) const final { return Total_Heat; }

  /*!
   * \brief Provide the total heat load.
   * \return Value of the heat load (viscous contribution).
   */
  inline su2double GetTotal_MaxHeatFlux() const final { return Total_MaxHeat; }

  /*!
   * \brief Store the total heat load.
   * \param[in] val_Total_Heat - Value of the heat load.
   */
  inline void SetTotal_HeatFlux(su2double val_Total_Heat) final { Total_Heat = val_Total_Heat; }

  /*!
   * \brief Store the total heat load.
   * \param[in] val_Total_Heat - Value of the heat load.
   */
  inline void SetTotal_MaxHeatFlux(su2double val_Total_MaxHeat) final { Total_MaxHeat = val_Total_MaxHeat; }

  /*!
   * \brief Provide the total custom objective function.
   * \return Value of the custom objective function.
   */
  inline su2double GetTotal_Custom_ObjFunc() const final { return Total_Custom_ObjFunc; }

  /*!
   * \brief Provide the Pressure coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the pressure coefficient.
   */
  inline su2double GetCPressure(unsigned short val_marker, unsigned long val_vertex) const final {
    return CPressure[val_marker][val_vertex];
  }

  /*!
   * \brief Provide the Target Pressure coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the pressure coefficient.
   */
  inline su2double GetCPressureTarget(unsigned short val_marker, unsigned long val_vertex) const final {
    return CPressureTarget[val_marker][val_vertex];
  }

  /*!
   * \brief Set the value of the target Pressure coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the pressure coefficient.
   */
  inline void SetCPressureTarget(unsigned short val_marker, unsigned long val_vertex, su2double val_pressure) final {
    CPressureTarget[val_marker][val_vertex] = val_pressure;
  }

  //zhen: used for temperature inverse design
  inline su2double GetTemTarget(unsigned short val_marker, unsigned long val_vertex) const final {
    return TemTarget[val_marker][val_vertex];
  }
  inline su2double GetTem(unsigned short val_marker, unsigned long val_vertex) const final {
    return Tem[val_marker][val_vertex];
  }
  inline void SetTemTarget(unsigned short val_marker, unsigned long val_vertex, su2double val_temperature) final {
    TemTarget[val_marker][val_vertex] = val_temperature;
  }
  /*!
   * \brief Value of the characteristic variables at the boundaries.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the pressure coefficient.
   */
  inline su2double* GetCharacPrimVar(unsigned short val_marker, unsigned long val_vertex) final {
    return CharacPrimVar[val_marker][val_vertex];
  }

  /*!
   * \brief Value of the characteristic variables at the boundaries.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the pressure coefficient.
   */
  inline void SetCharacPrimVar(unsigned short val_marker, unsigned long val_vertex, unsigned short val_var,
                               su2double val_value) final {
    CharacPrimVar[val_marker][val_vertex][val_var] = val_value;
  }

  /*!
   * \brief Value of the total temperature at an inlet boundary.
   * \param[in] val_marker - Surface marker where the total temperature is evaluated.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the total temperature is evaluated.
   * \return Value of the total temperature
   */
  inline su2double GetInlet_Ttotal(unsigned short val_marker, unsigned long val_vertex) const final {
    return Inlet_Ttotal[val_marker][val_vertex];
  }

  /*!
   * \brief Value of the total pressure at an inlet boundary.
   * \param[in] val_marker - Surface marker where the total pressure is evaluated.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the total pressure is evaluated.
   * \return Value of the total pressure
   */
  inline su2double GetInlet_Ptotal(unsigned short val_marker, unsigned long val_vertex) const final {
    return Inlet_Ptotal[val_marker][val_vertex];
  }

  /*!
   * \brief A component of the unit vector representing the flow direction at an inlet boundary.
   * \param[in] val_marker - Surface marker where the flow direction is evaluated
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the flow direction is evaluated
   * \param[in] val_dim - The component of the flow direction unit vector to be evaluated
   * \return Component of a unit vector representing the flow direction.
   */
  inline su2double GetInlet_FlowDir(unsigned short val_marker, unsigned long val_vertex,
                                    unsigned short val_dim) const final {
    return Inlet_FlowDir[val_marker][val_vertex][val_dim];
  }

  /*!
   * \brief Set the value of the total temperature at an inlet boundary.
   * \param[in] val_marker - Surface marker where the total temperature is set.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the total temperature is set.
   * \param[in] val_ttotal - Value of the total temperature
   */
  inline void SetInlet_Ttotal(unsigned short val_marker, unsigned long val_vertex, su2double val_ttotal) final {
    /*--- Since this call can be accessed indirectly using python, do some error
     * checking to prevent segmentation faults ---*/
    if (val_marker >= nMarker)
      SU2_MPI::Error("Out-of-bounds marker index used on inlet.", CURRENT_FUNCTION);
    else if (val_vertex >= nVertex[val_marker])
      SU2_MPI::Error("Out-of-bounds vertex index used on inlet.", CURRENT_FUNCTION);
    else
      Inlet_Ttotal[val_marker][val_vertex] = val_ttotal;
  }

  /*!
   * \brief Set the value of the total pressure at an inlet boundary.
   * \param[in] val_marker - Surface marker where the total pressure is set.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the total pressure is set.
   * \param[in] val_ptotal - Value of the total pressure
   */
  inline void SetInlet_Ptotal(unsigned short val_marker, unsigned long val_vertex, su2double val_ptotal) final {
    /*--- Since this call can be accessed indirectly using python, do some error
     * checking to prevent segmentation faults ---*/
    if (val_marker >= nMarker)
      SU2_MPI::Error("Out-of-bounds marker index used on inlet.", CURRENT_FUNCTION);
    else if (val_vertex >= nVertex[val_marker])
      SU2_MPI::Error("Out-of-bounds vertex index used on inlet.", CURRENT_FUNCTION);
    else
      Inlet_Ptotal[val_marker][val_vertex] = val_ptotal;
  }

  /*!
   * \brief Set a component of the unit vector representing the flow direction at an inlet boundary.
   * \param[in] val_marker - Surface marker where the flow direction is set.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the flow direction is set.
   * \param[in] val_dim - The component of the flow direction unit vector to be set
   * \param[in] val_flowdir - Component of a unit vector representing the flow direction.
   */
  inline void SetInlet_FlowDir(unsigned short val_marker, unsigned long val_vertex, unsigned short val_dim,
                               su2double val_flowdir) final {
    /*--- Since this call can be accessed indirectly using python, do some error
     * checking to prevent segmentation faults ---*/
    if (val_marker >= nMarker)
      SU2_MPI::Error("Out-of-bounds marker index used on inlet.", CURRENT_FUNCTION);
    else if (val_vertex >= nVertex[val_marker])
      SU2_MPI::Error("Out-of-bounds vertex index used on inlet.", CURRENT_FUNCTION);
    else
      Inlet_FlowDir[val_marker][val_vertex][val_dim] = val_flowdir;
  }

  /*!
   * \brief Compute the global error measures (L2, Linf) for verification cases.
   * \param[in] geometry - Geometrical definition.
   * \param[in] config   - Definition of the particular problem.
   */
  void ComputeVerificationError(CGeometry* geometry, CConfig* config) final;

  /*!
   * \brief Print verification error to screen, derived solvers must define this.
   * \param[in] config - Definition of the particular problem.
   */
  virtual void PrintVerificationError(const CConfig* config) const = 0;

  /*!
   * \brief Compute the pressure forces and all the adimensional coefficients.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  void Pressure_Forces(const CGeometry* geometry, const CConfig* config) final;

  /*!
   * \brief Compute the pressure forces and all the adimensional coefficients.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  void Momentum_Forces(const CGeometry* geometry, const CConfig* config) final;

  /*!
   * \brief Compute the viscous forces and all the addimensional coefficients.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  void Friction_Forces(const CGeometry* geometry, const CConfig* config) final;

  /*!
   * \brief Compute the buffet sensor.
   * \param[in] geometry - Geometrical definition of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  inline virtual void Buffet_Monitoring(const CGeometry *geometry, const CConfig *config) { }

  /*!
   * \brief Allocates the final pointer of SlidingState depending on how many donor vertex donate to it.
   * That number is stored in SlidingStateNodes[val_marker][val_vertex].
   * \param[in] val_marker - marker index
   * \param[in] val_vertex - vertex index
   */
  inline void SetSlidingStateStructure(unsigned short val_marker, unsigned long val_vertex) final {
    for (int iVar = 0; iVar < nPrimVar + 1; iVar++) {
      if (SlidingState[val_marker][val_vertex][iVar] != nullptr) delete[] SlidingState[val_marker][val_vertex][iVar];
    }

    for (int iVar = 0; iVar < nPrimVar + 1; iVar++)
      SlidingState[val_marker][val_vertex][iVar] = new su2double[GetnSlidingStates(val_marker, val_vertex)];
  }

  /*!
   * \brief Set the outer state for fluid interface nodes.
   * \param[in] val_marker   - marker index
   * \param[in] val_vertex   - vertex index
   * \param[in] val_state    - requested state component
   * \param[in] donor_index  - index of the donor node to set
   * \param[in] component    - set value
   */
  inline void SetSlidingState(unsigned short val_marker, unsigned long val_vertex, unsigned short val_state,
                              unsigned long donor_index, su2double component) final {
    SlidingState[val_marker][val_vertex][val_state][donor_index] = component;
  }

  /*!
   * \brief Set the number of outer state for fluid interface nodes.
   * \param[in] val_marker - marker index
   * \param[in] val_vertex - vertex index
   * \param[in] value - number of outer states
   */
  inline void SetnSlidingStates(unsigned short val_marker, unsigned long val_vertex, int value) final {
    SlidingStateNodes[val_marker][val_vertex] = value;
  }

  /*!
   * \brief Get the number of outer state for fluid interface nodes.
   * \param[in] val_marker - marker index
   * \param[in] val_vertex - vertex index
   */
  inline int GetnSlidingStates(unsigned short val_marker, unsigned long val_vertex) const final {
    return SlidingStateNodes[val_marker][val_vertex];
  }

  /*!
   * \brief Get the outer state for fluid interface nodes.
   * \param[in] val_marker - marker index
   * \param[in] val_vertex - vertex index
   * \param[in] val_state  - requested state component
   * \param[in] donor_index- index of the donor node to get
   */
  inline su2double GetSlidingState(unsigned short val_marker, unsigned long val_vertex, unsigned short val_state,
                                   unsigned long donor_index) const final {
    return SlidingState[val_marker][val_vertex][val_state][donor_index];
  }

  /*!
   * \brief Set the conjugate heat variables.
   * \param[in] val_marker        - marker index
   * \param[in] val_vertex        - vertex index
   * \param[in] pos_var           - variable position (in vector of all conjugate heat variables)
   * \param[in] relaxation factor - relaxation factor for the change of the variables
   * \param[in] val_var           - value of the variable
   */
  inline void SetConjugateHeatVariable(unsigned short val_marker, unsigned long val_vertex, unsigned short pos_var,
                                       su2double relaxation_factor, su2double val_var) final {
    HeatConjugateVar[val_marker][val_vertex][pos_var] =
        relaxation_factor * val_var + (1.0 - relaxation_factor) * HeatConjugateVar[val_marker][val_vertex][pos_var];
  }

  /*!
   * \brief Set the conjugate heat variables.
   * \param[in] val_marker        - marker index
   * \param[in] val_vertex        - vertex index
   * \param[in] pos_var           - variable position (in vector of all conjugate heat variables)
   */
  inline su2double GetConjugateHeatVariable(unsigned short val_marker, unsigned long val_vertex,
                                            unsigned short pos_var) const final {
    return HeatConjugateVar[val_marker][val_vertex][pos_var];
  }

  /*!
   * \brief Get the skin friction coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the skin friction coefficient.
   */
  inline su2double GetCSkinFriction(unsigned short val_marker, unsigned long val_vertex,
                                    unsigned short val_dim) const final {
    return CSkinFriction[val_marker](val_vertex,val_dim);
  }

  /*!
   * \brief Get the wall shear stress.
   * \param[in] val_marker - Surface marker where the wall shear stress is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the wall shear stress is evaluated.
   * \return Value of the wall shear stress.
   */
  inline su2double GetWallShearStress(unsigned short val_marker, unsigned long val_vertex) const final {
    return WallShearStress[val_marker][val_vertex];
  }

  /*!
   * \brief Get the skin friction coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the heat transfer coefficient.
   */
  inline su2double GetHeatFlux(unsigned short val_marker, unsigned long val_vertex) const final {
    return HeatFlux[val_marker][val_vertex];
  }

  /*!
   * \brief Get the skin friction coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the heat transfer coefficient.
   */
  inline su2double GetHeatFluxTarget(unsigned short val_marker, unsigned long val_vertex) const final {
    return HeatFluxTarget[val_marker][val_vertex];
  }

  /*!
   * \brief Set the value of the target Pressure coefficient.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the pressure coefficient.
   */
  inline void SetHeatFluxTarget(unsigned short val_marker, unsigned long val_vertex, su2double val_heat) final {
    HeatFluxTarget[val_marker][val_vertex] = val_heat;
  }

  /*!
   * \brief Get the y plus.
   * \param[in] val_marker - Surface marker where the coefficient is computed.
   * \param[in] val_vertex - Vertex of the marker <i>val_marker</i> where the coefficient is evaluated.
   * \return Value of the y plus.
   */
  inline su2double GetYPlus(unsigned short val_marker, unsigned long val_vertex) const final {
    return YPlus[val_marker][val_vertex];
  }
};
