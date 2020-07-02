/* ----------------------------------------------------------------------------
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file  testPrismaticJoint.cpp
 * @brief Test Joint class.
 * @Author: Frank Dellaert, Mandy Xie, Alejandro Escontrela, and Yetong Zhang
 */

#include "gtdynamics/universal_robot/Link.h"
#include "gtdynamics/universal_robot/PrismaticJoint.h"
#include "gtdynamics/universal_robot/sdf.h"
#include "gtdynamics/utils/utils.h"

#include <gtsam/base/Testable.h>
#include <gtsam/base/TestableAssertions.h>

#include <CppUnitLite/TestHarness.h>

using namespace gtdynamics; 
using gtsam::assert_equal, gtsam::Pose3, gtsam::Point3, gtsam::Rot3;

/**
 * Construct a Prismatic joint via Parameters and ensure all values are as
 * expected.
 */
TEST(Joint, params_constructor_prismatic) {
  auto simple_urdf =
      get_sdf(std::string(URDF_PATH) + "/test/simple_urdf_prismatic.urdf");
  LinkSharedPtr l1 =
      std::make_shared<Link>(*simple_urdf.LinkByName("l1"));
  LinkSharedPtr l2 =
      std::make_shared<Link>(*simple_urdf.LinkByName("l2"));

  ScrewJointBase::Parameters parameters;
  parameters.effort_type = JointEffortType::Actuated;
  parameters.joint_lower_limit = 0;
  parameters.joint_upper_limit = 2;
  parameters.joint_limit_threshold = 0;

  const gtsam::Vector3 j1_axis = (gtsam::Vector(3) << 0, 0, 1).finished();

  PrismaticJointSharedPtr j1 = std::make_shared<PrismaticJoint>(
      "j1", Pose3(Rot3::Rx(1.5707963268), Point3(0, 0, 2)),
      l1, l2, parameters, j1_axis);

  // get shared ptr
  EXPECT(j1->getSharedPtr() == j1);

  // get, set ID
  j1->setID(1);
  EXPECT(j1->getID() == 1);

  // name
  EXPECT(assert_equal(j1->name(), "j1"));

  // joint effort type
  EXPECT(j1->jointEffortType() == Joint::JointEffortType::Actuated);

  // other link
  EXPECT(j1->otherLink(l2) == l1);
  EXPECT(j1->otherLink(l1) == l2);

  // rest transform
  Pose3 T_12comRest(Rot3::Rx(1.5707963268),
                           Point3(0, -1, 1));
  Pose3 T_21comRest(Rot3::Rx(-1.5707963268),
                           Point3(0, -1, -1));
  EXPECT(assert_equal(T_12comRest, j1->transformFrom(l2), 1e-5));
  EXPECT(assert_equal(T_21comRest, j1->transformTo(l2), 1e-5));

  // transform from (translating +1)
  Pose3 T_12com(Rot3::Rx(1.5707963268), Point3(0, -2, 1));
  Pose3 T_21com(Rot3::Rx(-1.5707963268),
                       Point3(0, -1, -2));
  EXPECT(assert_equal(T_12com, j1->transformFrom(l2, 1), 1e-5));
  EXPECT(assert_equal(T_21com, j1->transformFrom(l1, 1), 1e-5));

  // transfrom to (translating +1)
  EXPECT(assert_equal(T_12com, j1->transformTo(l1, 1), 1e-5));
  EXPECT(assert_equal(T_21com, j1->transformTo(l2, 1), 1e-5));

  // screw axis
  gtsam::Vector6 screw_axis_l1, screw_axis_l2;
  screw_axis_l1 << 0, 0, 0, 0, 1, 0;
  screw_axis_l2 << 0, 0, 0, 0, 0, 1;
  EXPECT(assert_equal(screw_axis_l1, j1->screwAxis(l1)));
  EXPECT(assert_equal(screw_axis_l2, j1->screwAxis(l2), 1e-5));

  // links
  auto links = j1->links();
  EXPECT(links[0] == l1);
  EXPECT(links[1] == l2);

  // parent & child link
  EXPECT(j1->parentLink() == l1);
  EXPECT(j1->childLink() == l2);

  // joint limit
  EXPECT(assert_equal(0, j1->jointLowerLimit()));
  EXPECT(assert_equal(2, j1->jointUpperLimit()));
  EXPECT(assert_equal(0.0, j1->jointLimitThreshold()));
}

int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
