/* ----------------------------------------------------------------------------
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file DynamicsGraphBuilder.h
 * @brief Builds a dynamics graph from a UniversalRobot object.
 * @author Yetong Zhang, Alejandro Escontrela
 */

#pragma once

#include <OptimizerSetting.h>
#include <UniversalRobot.h>

#include <gtsam/inference/LabeledSymbol.h>
#include <gtsam/linear/NoiseModel.h>

#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/Values.h>

#include <cmath>
#include <iosfwd>
#include <vector>
#include <string>
#include <boost/optional.hpp>

namespace robot {

// TODO(aescontrela3, yetongumich): can we not use inline here?

/* Shorthand for F_i_j_t, for wrenches at j-th joint on the i-th link at time t.
 */
inline gtsam::LabeledSymbol WrenchKey(int i, int j, int t) {
  return gtsam::LabeledSymbol('F', i * 16 + j,
                              t);  // a hack here for a key with 3 numbers
}

/* Shorthand for C_i_t, for contact wrench on i-th link at time t.*/
inline gtsam::LabeledSymbol ContactWrenchKey(int i, int t) {
  return gtsam::LabeledSymbol('C', i, t);
}

/* Shorthand for T_j_t, for torque on the j-th joint at time t. */
inline gtsam::LabeledSymbol TorqueKey(int j, int t) {
  return gtsam::LabeledSymbol('T', j, t);
}

/* Shorthand for p_i_t, for COM pose on the i-th link at time t. */
inline gtsam::LabeledSymbol PoseKey(int i, int t) {
  return gtsam::LabeledSymbol('p', i, t);
}

/* Shorthand for V_i_t, for 6D link twist vector on the i-th link. */
inline gtsam::LabeledSymbol TwistKey(int i, int t) {
  return gtsam::LabeledSymbol('V', i, t);
}

/* Shorthand for A_i_t, for twist accelerations on the i-th link at time t. */
inline gtsam::LabeledSymbol TwistAccelKey(int i, int t) {
  return gtsam::LabeledSymbol('A', i, t);
}

/* Shorthand for q_j_t, for j-th joint angle at time t. */
inline gtsam::LabeledSymbol JointAngleKey(int j, int t) {
  return gtsam::LabeledSymbol('q', j, t);
}

/* Shorthand for v_j_t, for j-th joint velocity at time t. */
inline gtsam::LabeledSymbol JointVelKey(int j, int t) {
  return gtsam::LabeledSymbol('v', j, t);
}

/* Shorthand for a_j_t, for j-th joint acceleration at time t. */
inline gtsam::LabeledSymbol JointAccelKey(int j, int t) {
  return gtsam::LabeledSymbol('a', j, t);
}

/* Shorthand for dt_k, for duration for timestep dt_k during phase k. */
inline gtsam::LabeledSymbol PhaseKey(int k) {
  return gtsam::LabeledSymbol('t', 0, k);
}

/* Shorthand for t_t, time at time step t. */
inline gtsam::LabeledSymbol TimeKey(int t) {
  return gtsam::LabeledSymbol('t', 1, t);
}

/**
 * DynamicsGraphBuilder is a class which builds a factor graph to do kinodynamic
 * motion planning
 */
class DynamicsGraphBuilder {
 private:
  manipulator::OptimizerSetting opt_;

 public:
  /**
   * Constructor
   */
  DynamicsGraphBuilder() {
    opt_ = manipulator::OptimizerSetting();
    // set all dynamics related factors to be constrained
    opt_.bp_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.bv_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.ba_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.p_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.v_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.a_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.f_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.t_cost_model = gtsam::noiseModel::Constrained::All(1);
    opt_.tf_cost_model = gtsam::noiseModel::Constrained::All(6);
    opt_.q_cost_model = gtsam::noiseModel::Constrained::All(1);
    opt_.qv_cost_model = gtsam::noiseModel::Constrained::All(1);

    opt_.setLM();
  }
  ~DynamicsGraphBuilder() {}

  enum CollocationScheme { Euler, RungeKutta, Trapezoidal, HermiteSimpson };

  enum OptimizerType { GaussNewton, LM, PDL };

  /** return linear factor graph of all dynamics factors
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
     joint_angles               -- joint angles
     joint_vels                 -- joint velocities
     fk_results                 -- forward kinematics results
     gravity                    -- gravity in world frame
     planar_axis                -- axis of the plane, used only for planar robot
   */
  static gtsam::GaussianFactorGraph linearDynamicsGraph(const UniversalRobot &robot, const int t,
                                                        const UniversalRobot::JointValues& joint_angles,
                                                        const UniversalRobot::JointValues& joint_vels,
                                                        const UniversalRobot::FKResults &fk_results,
                                                        const boost::optional<gtsam::Vector3> &gravity = boost::none,
                                                        const boost::optional<gtsam::Vector3> &planar_axis = boost::none);

  /* return linear factor graph with priors on torques */
  static gtsam::GaussianFactorGraph linearFDPriors(const UniversalRobot &robot,
                                                   const int t, 
                                                   const UniversalRobot::JointValues& torque_values);

  /** sovle forward kinodynamics using linear factor graph, return values of all variables
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
     joint_angles               -- std::map <joint name, angle>
     joint_vels                 -- std::map <joint name, velocity>
     torques                    -- std::map <joint name, torque>
     fk_results                 -- forward kinematics results
     gravity                    -- gravity in world frame
     planar_axis                -- axis of the plane, used only for planar robot
  * return values of all variables
  */
  static gtsam::Values linearSolveFD(const UniversalRobot &robot, const int t,
                                     const UniversalRobot::JointValues& joint_angles,
                                     const UniversalRobot::JointValues& joint_vels,
                                     const UniversalRobot::JointValues& torques,
                                     const UniversalRobot::FKResults &fk_results,
                                     const boost::optional<gtsam::Vector3> &gravity = boost::none,
                                     const boost::optional<gtsam::Vector3> &planar_axis = boost::none);


  /* return q-level nonlinear factor graph (pose related factors) */
  gtsam::NonlinearFactorGraph qFactors(const UniversalRobot &robot,
                                       const int t) const;

  /* return v-level nonlinear factor graph (twist related factors) */
  gtsam::NonlinearFactorGraph vFactors(const UniversalRobot &robot,
                                       const int t) const;

  /* return a-level nonlinear factor graph (acceleration related factors) */
  gtsam::NonlinearFactorGraph aFactors(const UniversalRobot &robot,
                                       const int t) const;

  /* return dynamics-level nonlinear factor graph (wrench related factors) */
  gtsam::NonlinearFactorGraph dynamicsFactors(
      const UniversalRobot &robot, const int t,
      const boost::optional<gtsam::Vector3> &gravity = boost::none,
      const boost::optional<gtsam::Vector3> &planar_axis = boost::none) const;

  /** return nonlinear factor graph of all dynamics factors
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
     gravity                    -- gravity in world frame
     planar_axis               -- axis of the plane, used only for planar robot
     contacts                   -- vector of length num_links where 1 denotes
        contact link and 0 denotes no contact.
   */
  gtsam::NonlinearFactorGraph dynamicsFactorGraph(
      const UniversalRobot &robot, const int t,
      const boost::optional<gtsam::Vector3> &gravity = boost::none,
      const boost::optional<gtsam::Vector3> &planar_axis = boost::none,
      const boost::optional<std::vector<uint>> &contacts = boost::none) const;

  /** return prior factors of torque, angle, velocity
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
     joint_angles               -- joint angles specified in order of joints
     joint_vels                 -- joint velocites specified in order of joints
     torques                    -- joint torques specified in order of joints
   */
  gtsam::NonlinearFactorGraph forwardDynamicsPriors(
      const UniversalRobot &robot, const int t,
      const gtsam::Vector &joint_angles, const gtsam::Vector &joint_vels,
      const gtsam::Vector &torques) const;

  /** return prior factors of initial state, torques along trajectory
  * Keyword arguments:
     robot                      -- the robot
     num_steps                  -- total time steps
     joint_angles               -- joint angles specified in order of joints
     joint_vels                 -- joint velocites specified in order of joints
     torques_seq                -- joint torques along the trajectory
   */
  gtsam::NonlinearFactorGraph trajectoryFDPriors(
      const UniversalRobot &robot, const int num_steps,
      const gtsam::Vector &joint_angles, const gtsam::Vector &joint_vels,
      const std::vector<gtsam::Vector> &torques_seq) const;

  /** return nonlinear factor graph of the entire trajectory
  * Keyword arguments:
     robot                      -- the robot
     num_steps                  -- total time steps
     dt                         -- duration of each time step
     collocation                -- the collocation scheme
     gravity                    -- gravity in world frame
     planar_axis                -- axis of the plane, used only for planar robot
   */
  gtsam::NonlinearFactorGraph trajectoryFG(
      const UniversalRobot &robot, const int num_steps, const double dt,
      const CollocationScheme collocation,
      const boost::optional<gtsam::Vector3> &gravity = boost::none,
      const boost::optional<gtsam::Vector3> &planar_axis = boost::none) const;

  /** return nonlinear factor graph of the entire trajectory for multi-phase
  * Keyword arguments:
     robots                     -- the robot configuration for each phase
     phase_steps                -- number of time steps for each phase
     transition_graphs          -- dynamcis graph for each transition timestep
  (including guardian factors) collocation                -- the collocation
  scheme gravity                    -- gravity in world frame planar_axis --
  axis of the plane, used only for planar robot
   */
  gtsam::NonlinearFactorGraph multiPhaseTrajectoryFG(
      const std::vector<UniversalRobot> &robots,
      const std::vector<int> &phase_steps,
      const std::vector<gtsam::NonlinearFactorGraph> &transition_graphs,
      const CollocationScheme collocation,
      const boost::optional<gtsam::Vector3> &gravity = boost::none,
      const boost::optional<gtsam::Vector3> &planar_axis = boost::none) const;

  /** return collocation factors on angles and velocities from time step t to
  t+1
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
     dt                         -- duration of each timestep
     collocation                -- collocation scheme chosen
   */
  gtsam::NonlinearFactorGraph collocationFactors(
      const UniversalRobot &robot, const int t, const double dt,
      const CollocationScheme collocation) const;

  /** return collocation factors on angles and velocities from time step t to
  t+1, with dt as a varaible
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
     phase                      -- the phase of the timestep
     collocation                -- collocation scheme chosen
   */
  gtsam::NonlinearFactorGraph multiPhaseCollocationFactors(
      const UniversalRobot &robot, const int t, const int phase,
      const CollocationScheme collocation) const;

  /** return the joint accelerations
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
   */
  static gtsam::Vector jointAccels(const UniversalRobot &robot,
                                   const gtsam::Values &result, const int t);

  /* retirm joint velocities. */
  static gtsam::Vector jointVels(const UniversalRobot &robot,
                                 const gtsam::Values &result, const int t);

  /* retirm joint angles. */
  static gtsam::Vector jointAngles(const UniversalRobot &robot,
                                   const gtsam::Values &result, const int t);

  /** return zero values for all variables for initial value of optimization
  * Keyword arguments:
     robot                      -- the robot
     t                          -- time step
   */
  static gtsam::Values zeroValues(const UniversalRobot &robot, const int t);

  /** return zero values of the trajectory for initial value of optimization
  * Keyword arguments:
     robot                      -- the robot
     num_steps                  -- total time steps
     num_phases                 -- number of phases, -1 for not using
  multi-phase
   */
  static gtsam::Values zeroValuesTrajectory(const UniversalRobot &robot,
                                            const int num_steps,
                                            const int num_phases = -1);

  /** optimize factor graph
  * Keyword arguments:
     graph                      -- nonlinear factor graph
     init_values                -- initial values for optimization
     optim_type                 -- choice of optimizer type
   */
  static gtsam::Values optimize(const gtsam::NonlinearFactorGraph &graph,
                                const gtsam::Values &init_values,
                                OptimizerType optim_type);

  // print the factors of the factor graph
  static void print_graph(const gtsam::NonlinearFactorGraph &graph);

  // print the values
  static void print_values(const gtsam::Values &values);

  /** save factor graph in json format for visualization
  * Keyword arguments:
     file_path                  -- path of the json file to store the graph
     graph                      -- factor graph
     values                     -- values of variables in factor graph
     robot                      -- the robot
     t                          -- time step
     radial                     -- option to display in radial format
   */
  static void saveGraph(const std::string &file_path,
                        const gtsam::NonlinearFactorGraph &graph,
                        const gtsam::Values &values,
                        const UniversalRobot &robot, const int t,
                        bool radial = false);

  static void saveGraphMultiSteps(const std::string &file_path,
                                  const gtsam::NonlinearFactorGraph &graph,
                                  const gtsam::Values &values,
                                  const UniversalRobot &robot,
                                  const int num_steps, bool radial = false);
};

}  // namespace robot
