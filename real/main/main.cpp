#include <Arduino.h>
#include <common/commanduino/CommanDuino.h>
#include <real/ArduinoTimer.h>
#include <real/RealMouse.h>
#include <common/core/util.h>
#include <common/core/Flood.h>
#include <common/commands/SolveCommand.h>

ArduinoTimer timer;
AbstractMaze maze;
Scheduler *scheduler;
RealMouse *mouse;
unsigned long last_t, last_blink;
bool done = false;
bool on = true;
bool paused = false;

void setup() {
  delay(1000);
  Command::setTimerImplementation(&timer);
  mouse = RealMouse::inst();
  mouse->setup();

  GlobalProgramSettings.quiet = false;

//  scheduler = new Scheduler(new NavTestCommand());
  scheduler = new Scheduler(new SolveCommand(new Flood(mouse)));

  last_t = timer.programTimeMs();
  last_blink = timer.programTimeMs();
}

void loop() {
  if (Serial1.available()) {
    int c = Serial1.read();
    if (c == (int) 'p') {
      Serial1.clear();
      analogWrite(RealMouse::MOTOR_LEFT_A, 0);
      analogWrite(RealMouse::MOTOR_RIGHT_A, 0);
      analogWrite(RealMouse::MOTOR_LEFT_B, 0);
      analogWrite(RealMouse::MOTOR_RIGHT_B, 0);
      paused = !paused;
    }
  }

  if (paused) {
    digitalWrite(RealMouse::SYS_LED, 1);
    return;
  }

  unsigned long now = timer.programTimeMs();
  double dt_s = (now - last_t) / 1000.0;

  if (now - last_blink > 100) {
    last_blink = now;
    digitalWrite(RealMouse::SYS_LED, on);
    on = !on;
  }

  // minimum period of main loop
  if (dt_s < 0.010) {
    return;
  }

  mouse->run(dt_s);

  if (!done) {
#ifdef PROFILE
    unsigned long t0 = micros();
#endif
    done = scheduler->run();
#ifdef PROFILE
    Serial.print("Schedule, ");
    Serial.println(micros() - t0);
#endif
  } else {
    mouse->setSpeedCps(0, 0);
    digitalWrite(RealMouse::SYS_LED, 1);
  }
  last_t = now;
}
