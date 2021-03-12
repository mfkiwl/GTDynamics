/* ----------------------------------------------------------------------------
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file  testScrewJoint.cpp
 * @brief Test ScrewJoint class.
 * @author Frank Dellaert, Mandy Xie, Alejandro Escontrela, and Yetong Zhang
 */

#include <CppUnitLite/TestHarness.h>
#include <gtsam/base/Testable.h>
#include <gtsam/base/TestableAssertions.h>

#include "gtdynamics/universal_robot/Link.h"
#include "gtdynamics/universal_robot/ScrewJoint.h"
#include "gtdynamics/universal_robot/sdf.h"
#include "gtdynamics/utils/utils.h"

using namespace gtdynamics;

using gtsam::assert_equal, gtsam::Pose3, gtsam::Point3, gtsam::Rot3;

/**
 * Construct a Screw joint via Parameters and ensure all values are as
 * expected.
 */
TEST(Joint, params_constructor) {
  auto simple_urdf = get_sdf(std::string(URDF_PATH) + "/test/simple_urdf.urdf");
  LinkSharedPtr l1 = boost::make_shared<Link>(*simple_urdf.LinkByName("l1"));
  LinkSharedPtr l2 = boost::make_shared<Link>(*simple_urdf.LinkByName("l2"));

  JointParams parameters;
  parameters.effort_type = JointEffortType::Actuated;
  parameters.scalar_limits.value_lower_limit = -1.57;
  parameters.scalar_limits.value_upper_limit = 1.57;
  parameters.scalar_limits.value_limit_threshold = 0;

  ScrewJointSharedPtr j1 =
      boost::make_shared<ScrewJoint>("j1", Pose3(Rot3(), Point3(0, 0, 2)), l1, l2,
                                   parameters, gtsam::Vector3(1, 0, 0), 0.5);
  j1->setID(123);

  // name
  EXPECT(assert_equal(j1->name(), "j1"));

  // ID
  EXPECT(123 == j1->getID());

  // joint effort type
  EXPECT(j1->parameters().effort_type == JointEffortType::Actuated);

  // other link
  EXPECT(j1->otherLink(l2) == l1);
  EXPECT(j1->otherLink(l1) == l2);

  // screw axis
  gtsam::Vector6 screw_axis_l1, screw_axis_l2;
  screw_axis_l1 << -1, 0, 0, -0.5 / 2 / M_PI, -1, 0;  // parent frame
  screw_axis_l2 << 1, 0, 0, 0.5 / 2 / M_PI, -1, 0;    // child frame
  EXPECT(assert_equal(screw_axis_l2, j1->screwAxis(l2)));
  EXPECT(assert_equal(screw_axis_l1, j1->screwAxis(l1)));

  // rest transform
  Pose3 T_12comRest(Rot3::Rx(0), Point3(0, 0, 2));
  Pose3 T_21comRest(Rot3::Rx(0), Point3(0, 0, -2));
  EXPECT(assert_equal(T_12comRest, j1->transformFrom(l2)));
  EXPECT(assert_equal(T_21comRest, j1->transformTo(l2)));

  // transform from (rotating -pi/2)
  Pose3 T_12com(Rot3::Rx(-M_PI / 2), Point3(-0.125, 1, 1));
  Pose3 T_21com(Rot3::Rx(M_PI / 2), Point3(0.125, 1, -1));
  EXPECT(assert_equal(T_12com, j1->transformFrom(l2, -M_PI / 2)));
  EXPECT(assert_equal(T_21com, j1->transformFrom(l1, -M_PI / 2)));

  // transform to (rotating -pi/2)
  EXPECT(assert_equal(T_12com, j1->transformTo(l1, -M_PI / 2)));
  EXPECT(assert_equal(T_21com, j1->transformTo(l2, -M_PI / 2)));

  // should throw error
  CHECK_EXCEPTION(j1->transformTo(l1, gtsam::Values()),
                  gtsam::ValuesKeyDoesNotExist);

  // links
  auto links = j1->links();
  EXPECT(links[0] == l1);
  EXPECT(links[1] == l2);

  // parent & child link
  EXPECT(j1->parentLink() == l1);
  EXPECT(j1->childLink() == l2);

  // joint limit
  EXPECT(assert_equal(parameters.scalar_limits.value_lower_limit,
                      j1->parameters().scalar_limits.value_lower_limit));
  EXPECT(assert_equal(parameters.scalar_limits.value_upper_limit,
                      j1->parameters().scalar_limits.value_upper_limit));
  EXPECT(assert_equal(parameters.scalar_limits.value_limit_threshold,
                      j1->parameters().scalar_limits.value_limit_threshold));
}

int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
