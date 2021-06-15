"""
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 *
 * @file  collocation_optimization.py
 * @brief Create a vertical jumping trajectory, simulate to get initial values,
          optimize to satisfy all collcoation constraints.
 * @author Yetong Zhang
"""

import gtsam
import gtdynamics as gtd
import numpy as np

import os, sys, inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0, parentdir)

from src.jumping_robot import Actuator, JumpingRobot
from src.jr_graph_builder import JRGraphBuilder
from src.jr_simulator import JRSimulator
from src.helpers import OptimizeLM
from src.jr_visualizer import visualize_jr_trajectory
from scripts.jr01_simulation import vertical_jump_simulation


def vertical_jump_optimization(jr, controls, sim_values, step_phases):
    """ Collocation optimization for vertical jump. """
    jr_graph_builder = JRGraphBuilder()
    collocation = gtd.CollocationScheme.Trapezoidal
    graph = jr_graph_builder.trajectory_graph(jr, step_phases, collocation)
    # graph.push_back(jr_graph_builder.control_priors(jr, controls))

    # goal factors
    num_steps = len(step_phases)
    graph.push_back(jr_graph_builder.vertical_jump_max_height_factors(jr, num_steps))

    goal_pose = gtsam.Pose3(gtsam.Rot3(), gtsam.Point3(0, 0, 1.40))
    graph.push_back(jr_graph_builder.target_pose_goal_factor(jr, num_steps, goal_pose))

    # symmetry factor for control
    graph.push_back(jr_graph_builder.control_symmetry_factors(jr))

    results = OptimizeLM(graph, sim_values)
    print("final error: ", graph.error(results))

    return results


def get_final_torso_pose(jr, values, step_phases):
    num_steps = len(step_phases)
    torso_i = jr.robot.link("torso").id()
    torso_pose = gtd.Pose(values, torso_i, num_steps)
    return torso_pose


def print_controls(jr, values):
    Tos = []
    Tcs = []
    for actuator in jr.actuators:
        j = actuator.j
        To = values.atDouble(Actuator.ValveOpenTimeKey(j))
        Tc = values.atDouble(Actuator.ValveCloseTimeKey(j))
        Tos.append(To)
        Tcs.append(Tc)
    print("valve open time: ", Tos)
    print("valve close time: ", Tcs)


def main():
    """ Main file. """
    # create jumping robot
    yaml_file_path = JumpingRobot.icra_yaml()
    init_config = JumpingRobot.icra_init_config()
    jr = JumpingRobot.from_yaml(yaml_file_path, init_config)

    # create controls
    Tos = [0, 0, 0, 0]
    Tcs = [0.098, 0.098, 0.098, 0.098]
    controls = JumpingRobot.create_controls(Tos, Tcs)

    # simulation
    dt = 2e-3
    sim_values, step_phases = vertical_jump_simulation(jr, controls, dt)
    print("simulation final torso pose:\n", get_final_torso_pose(jr, sim_values, step_phases))

    # collocation optimization
    results = vertical_jump_optimization(jr, controls, sim_values, step_phases)
    print("optimized final torso pose:\n", get_final_torso_pose(jr, results, step_phases))

    # resulting controls
    print_controls(jr, results)

    # visualize
    visualize_jr_trajectory(results, jr, len(step_phases), dt)

if __name__ == "__main__":
    main()