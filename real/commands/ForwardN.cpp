#include <real/RealMouse.h>
#include "ForwardN.h"

ForwardN::ForwardN(unsigned int n) : Command("Forward"), mouse(RealMouse::inst()), n(n) {}


void ForwardN::initialize() {
  start = mouse->getGlobalPose();
  mouse->kinematic_controller.enable_sensor_pose_estimate = true;
  mouse->kinematic_controller.start(start, KinematicController::dispToNthEdge(mouse, n));
//  mouse->kinematic_controller.plan_traj(start, KinematicController::poseOfNthEdge(mouse, n));
  digitalWrite(RealMouse::LED_4, 1);
}

void ForwardN::execute() {
  double l, r;
  std::tie(l, r) = mouse->kinematic_controller.compute_wheel_velocities(this->mouse);
  mouse->setSpeedCps(l, r);
}

bool ForwardN::isFinished() {
  return mouse->kinematic_controller.drive_straight_state.dispError <= 0;
}

void ForwardN::end() {
  digitalWrite(RealMouse::LED_4, 0);
  mouse->kinematic_controller.enable_sensor_pose_estimate = false;
}

