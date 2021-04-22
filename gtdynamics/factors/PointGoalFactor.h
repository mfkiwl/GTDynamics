/* ----------------------------------------------------------------------------
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file  PointGoalFactor.h
 * @brief Link point goal factor.
 * @author Alejandro Escontrela
 */

#pragma once

#include <gtsam/base/Matrix.h>
#include <gtsam/base/Vector.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/nonlinear/NonlinearFactor.h>

#include <iostream>
#include <string>

#include "gtdynamics/utils/values.h"

namespace gtdynamics {

/**
 * PointGoalFactor is a unary factor enforcing that a point on a link
 * reaches a desired goal point.
 */
class PointGoalFactor : public gtsam::NoiseModelFactor1<gtsam::Pose3> {
 private:
  using This = PointGoalFactor;
  using Base = gtsam::NoiseModelFactor1<gtsam::Pose3>;

  // Point, expressed in link CoM, where this factor is enforced.
  gtsam::Point3 point_com_;
  // Goal point in the spatial frame.
  gtsam::Point3 goal_point_;

 public:
  /**
   * Construct from joint angle limits
   * @param pose_key key for COM pose of the link
   * @param cost_model noise model
   * @param point_com point on link, in COM coordinate frame
   * @param goal_point end effector pose goal, in world coordinates
   */
  PointGoalFactor(gtsam::Key pose_key,
                  const gtsam::noiseModel::Base::shared_ptr &cost_model,
                  const gtsam::Point3 &point_com,
                  const gtsam::Point3 &goal_point)
      : Base(cost_model, pose_key),
        point_com_(point_com),
        goal_point_(goal_point) {}

  virtual ~PointGoalFactor() {}

  /// Return goal point.
  const gtsam::Point3 &goalPoint() const { return goal_point_; }

  /**
   * Error function
   * @param wTcom -- The link pose.
   */
  gtsam::Vector evaluateError(
      const gtsam::Pose3 &wTcom,
      boost::optional<gtsam::Matrix &> H_pose = boost::none) const override {
    // Change point reference frame from com to spatial.
    auto sTp_t = wTcom.transformFrom(point_com_, H_pose);
    return sTp_t - goal_point_;
  }

  //// @return a deep copy of this factor
  gtsam::NonlinearFactor::shared_ptr clone() const override {
    return boost::static_pointer_cast<gtsam::NonlinearFactor>(
        gtsam::NonlinearFactor::shared_ptr(new This(*this)));
  }

  /// print contents
  void print(const std::string &s = "",
             const gtsam::KeyFormatter &keyFormatter =
                 gtsam::DefaultKeyFormatter) const override {
    std::cout << s << "PointGoalFactor\n";
    Base::print("", keyFormatter);
    std::cout << "point on link: " << point_com_.transpose() << std::endl;
    std::cout << "goal point: " << goal_point_.transpose() << std::endl;
  }

 private:
  /// Serialization function
  friend class boost::serialization::access;
  template <class ARCHIVE>
  void serialize(ARCHIVE &ar, const unsigned int version) {  // NOLINT
    ar &boost::serialization::make_nvp(
        "NoiseModelFactor1", boost::serialization::base_object<Base>(*this));
  }
};

/**
 * @brief  Add PointGoalFactors for a stance foot.
 * @param factors graph to add to.
 * @param cost_model noise model
 * @param point_com point on link, in COM coordinate frame
 * @param goal_point end effector goal, in world coordinates
 * @param i The link id.
 * @param num_steps number of time steps
 * @param k_start starting time index (default 0).
 */
void AddStanceGoals(gtsam::NonlinearFactorGraph *factors,
                    const gtsam::SharedNoiseModel &cost_model,
                    const gtsam::Point3 &point_com,
                    const gtsam::Point3 &goal_point,  //
                    unsigned char i, size_t num_steps, size_t k_start = 0) {
  for (int k = k_start; k < k_start + num_steps; k++) {
    gtsam::Key pose_key = internal::PoseKey(i, k);
    factors->emplace_shared<PointGoalFactor>(pose_key, cost_model, point_com,
                                             goal_point);
  }
}

/**
 * @brief Add PointGoalFactors for swing foot, starting at (k_start, cp_goal).
 *
 * Swing feet is moved according to a pre-determined height trajectory, and
 * moved by the 3D vector step.
 * To see the curve, go to https://www.wolframalpha.com/ and type
 *    0.2 * pow(t, 1.1) * pow(1 - t, 0.7) for t from 0 to 1
 *
 * @param factors graph to add to.
 * @param cost_model noise model
 * @param point_com point on link, in COM coordinate frame
 * @param cp_goal initial end effector goal, in world coordinates
 * @param step 3D vector to move by
 * @param i The link id.
 * @param num_steps number of time steps
 * @param k_start starting time index (default 0).
 */
void AddSwingGoals(gtsam::NonlinearFactorGraph *factors,
                   const gtsam::SharedNoiseModel &cost_model,
                   const gtsam::Point3 &point_com,
                   gtsam::Point3 cp_goal,  // by value
                   const gtsam::Point3 &step, unsigned char i, size_t num_steps,
                   size_t k_start = 0) {
  const double dt = 1.0 / (num_steps - 1);
  const gtsam::Point3 delta_step = step * dt;
  for (int k = k_start; k < k_start + num_steps; k++) {
    gtsam::Key pose_key = internal::PoseKey(i, k);
    double t = dt * (k - k_start);
    double h = 0.2 * pow(t, 1.1) * pow(1 - t, 0.7);  // reaches 6 cm height
    factors->emplace_shared<PointGoalFactor>(pose_key, cost_model, point_com,
                                             cp_goal + gtsam::Point3(0, 0, h));
    cp_goal = cp_goal + delta_step;
  }
}

}  // namespace gtdynamics
