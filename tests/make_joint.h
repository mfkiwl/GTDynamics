/* ----------------------------------------------------------------------------
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file  make_joint.h
 * @brief Function often used in tests
 * @author: Frank Dellaert
 */

#pragma once

#include <gtdynamics/universal_robot/HelicalJoint.h>
#include <gtdynamics/universal_robot/Link.h>

namespace gtdynamics {
/// Create a joint with given rest transform cMp and screw-axis in child frame.
JointConstSharedPtr make_joint(gtsam::Pose3 cMp, gtsam::Vector6 cScrewAxis) {
  // create links
  std::string name = "l1";
  double mass = 100;
  gtsam::Matrix3 inertia = gtsam::Vector3(3, 2, 1).asDiagonal();
  gtsam::Pose3 bMcom;
  gtsam::Pose3 bMl;

  auto l1 = boost::make_shared<Link>(Link(1, name, mass, inertia, bMcom, bMl));
  auto l2 = boost::make_shared<Link>(
      Link(2, name, mass, inertia, cMp.inverse(), bMl));

  // create joint
  JointParams joint_params;
  joint_params.effort_type = JointEffortType::Actuated;
  joint_params.scalar_limits.value_lower_limit = -1.57;
  joint_params.scalar_limits.value_upper_limit = 1.57;
  joint_params.scalar_limits.value_limit_threshold = 0;
  gtsam::Pose3 bMj = gtsam::Pose3(gtsam::Rot3(), gtsam::Point3(0, 0, 2));
  gtsam::Pose3 jMc = bMj.inverse() * l2->bMcom();
  gtsam::Vector6 jScrewAxis = jMc.AdjointMap() * cScrewAxis;

  return boost::make_shared<const HelicalJoint>(1, "j1", bMj, l1, l2,
                                                jScrewAxis, joint_params);
}
}  // namespace gtdynamics
