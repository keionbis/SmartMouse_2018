#ifdef SIM

#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/gazebo_client.hh>
#include <iostream>

#include "SimMouse.h"
#include "WallFollow.h"
#include "SimTimer.h"
#include "StepSolveCommand.h"

int main(int argc, char* argv[]){
  // Load gazebo
  bool connected = gazebo::client::setup(argc, argv);
  if (!connected){
    printf("failed to connect to gazebo. Is it running?\n");
    exit(0);
  }

  AbstractMaze maze;
  SimMouse mouse(&maze);
  SimTimer timer;
  Command::setTimerImplementation(&timer);

  // Create our node for communication
  gazebo::transport::NodePtr node(new gazebo::transport::Node());
  node->Init();

  gazebo::transport::SubscriberPtr timeSub =
    node->Subscribe("~/world_stats", &SimTimer::simTimeCallback);

  gazebo::transport::SubscriberPtr poseSub =
    node->Subscribe("~/mouse/pose", &SimMouse::poseCallback, &mouse);

  gazebo::transport::SubscriberPtr senseSub =
    node->Subscribe("~/mouse/base/laser/scan", &SimMouse::senseCallback, &mouse);

  mouse.controlPub = node->Advertise<gazebo::msgs::JointCmd>("~/mouse/joint_cmd");

  mouse.simInit();
  Scheduler scheduler(new StepSolveCommand(new WallFollow(&mouse)));

  while (true){
    scheduler.run();
  }
}

#endif