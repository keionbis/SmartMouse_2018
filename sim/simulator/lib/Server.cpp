#include <sim/simulator/lib/Server.h>
#include <sim/simulator/lib/TopicNames.h>
#include <msgs/world_statistics.pb.h>

void Server::start() {
  thread_ = new std::thread(std::bind(&Server::RunLoop, this));
  sim_time_ = Time::Zero;
}

void Server::RunLoop() {
  node_ptr_ = new ignition::transport::Node();

  world_stats_pub_ = node_ptr_->Advertise<smartmouse::msgs::WorldStatistics>(TopicNames::kWorldStatistics);
  sim_state_pub_ = node_ptr_->Advertise<smartmouse::msgs::RobotSimState>(TopicNames::kRobotSimState);
  node_ptr_->Subscribe(TopicNames::kServerControl, &Server::OnServerControl, this);
  node_ptr_->Subscribe(TopicNames::kPhysics, &Server::OnPhysics, this);
  node_ptr_->Subscribe(TopicNames::kMaze, &Server::OnMaze, this);
  node_ptr_->Subscribe(TopicNames::kRobotCommand, &Server::OnRobotCommand, this);

  while (true) {
    Time update_rate = Time(0, ns_of_sim_per_step_);
    Time start_step_time = Time::GetWallTime();
    Time desired_step_time = update_rate / real_time_factor_;

    // special case when update_rate is zero, like on startup.
    if (desired_step_time == 0) {
      Time::MSleep(1);
      continue;
    }

    smartmouse::msgs::RobotSimState sim_state_msg;

    // Begin Critical Section
    {
      std::lock_guard<std::mutex> guard(physics_mutex_);
      if (quit_) {
        break;
      }

      if (pause_at_steps_ > 0 && pause_at_steps_ == steps_) {
        pause_at_steps_ = 0;
        pause_ = true;
        Time::MSleep(1);
        continue;
      }

      if (pause_) {
        Time::MSleep(1);
        continue;
      }

      sim_state_msg = Step();
    }
    // End Critical Section

    Time end_step_time = Time::GetWallTime();
    Time used_step_time = end_step_time - start_step_time;

    if (used_step_time < desired_step_time) {
      // there is a fudge factor here to account for the time to publish world stats
      Time sleep_time = desired_step_time - used_step_time - 50e-6;
      Time::Sleep(sleep_time);
    } else {
      std::cout << "Update took " << used_step_time.Float() << ". Skipping sleep." << std::endl;
    }

    Time actual_end_step_time = Time::GetWallTime();
    Time actual_step_time = actual_end_step_time - start_step_time;
    Time rtf = update_rate / actual_step_time;

    // announce completion of this step
    smartmouse::msgs::WorldStatistics world_stats_msg;
    world_stats_msg.set_steps(steps_);
    ignition::msgs::Time *sim_time_msg = world_stats_msg.mutable_sim_time();
    *sim_time_msg = sim_time_.toIgnMsg();
    world_stats_msg.set_real_time_factor(rtf.Double());
    world_stats_pub_.Publish(world_stats_msg);

    sim_state_pub_.Publish(sim_state_msg);
  }
}

smartmouse::msgs::RobotSimState Server::Step() {
  // update sim time
  auto dt = Time(0, ns_of_sim_per_step_);
  sim_time_ += dt;

//  double f = (cmd_.left().abstract_force() + cmd_.right().abstract_force())/2.0;
  double f = 0.1;
  double x_m = internal_state_.p().x();
  internal_state_.mutable_p()->set_x(x_m + f * dt.Double());

  smartmouse::msgs::RobotSimState sim_state_msg;
  auto stamp = sim_state_msg.mutable_stamp();
  stamp->set_sec(sim_time_.sec);
  stamp->set_nsec(sim_time_.nsec);
  sim_state_msg.set_true_x_meters(internal_state_.p().x());
  sim_state_msg.set_true_y_meters(internal_state_.p().y());
  sim_state_msg.set_true_yaw_rad(internal_state_.p().theta());

  // increment step counter
  ++steps_;

  return sim_state_msg;
}

void Server::ResetTime() {
  sim_time_ = Time::Zero;
  steps_ = 0UL;
  pause_at_steps_ = 0ul;
}

void Server::OnServerControl(const smartmouse::msgs::ServerControl &msg) {
  // Enter critical section
  {
    std::lock_guard<std::mutex> guard(physics_mutex_);
    if (msg.has_pause()) {
      pause_ = msg.pause();
    }
    if (msg.has_quit()) {
      quit_ = msg.quit();
    }
    if (msg.has_step()) {
      pause_ = false;
      pause_at_steps_ = steps_ + msg.step();
    }
    if (msg.has_reset_time()) {
      ResetTime();
    }
  }
  // End critical section
}

void Server::OnPhysics(const smartmouse::msgs::PhysicsConfig &msg) {
  // Enter critical section
  {
    std::lock_guard<std::mutex> guard(physics_mutex_);
    if (msg.has_ns_of_sim_per_step()) {
      ns_of_sim_per_step_ = msg.ns_of_sim_per_step();
    }
    if (msg.has_real_time_factor()) {
      if (msg.real_time_factor() >= 1e-3 && msg.real_time_factor() <= 10) {
        real_time_factor_ = msg.real_time_factor();
      }
    }
  }
  // End critical section
}

void Server::OnMaze(const smartmouse::msgs::Maze &msg) {
  // Enter critical section
  {
    std::lock_guard<std::mutex> guard(physics_mutex_);
    maze_ = msg;
  }
  // End critical section
}

void Server::OnRobotCommand(const smartmouse::msgs::RobotCommand &msg) {
  // Enter critical section
  {
    std::lock_guard<std::mutex> guard(physics_mutex_);
    cmd_ = msg;
  }
  // End critical section
}

void Server::join() {
  thread_->join();
}
