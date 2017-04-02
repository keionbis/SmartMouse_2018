#pragma once

#include <common/Mouse.h>
#include <common/SensorReading.h>

class ConsoleMouse : public Mouse {
public:

  virtual SensorReading checkWalls() override;

  static ConsoleMouse *inst();

  void seedMaze(AbstractMaze *maze);

private:

  static ConsoleMouse *instance;
  AbstractMaze *true_maze;

  ConsoleMouse();

  ConsoleMouse(int starting_row, int starting_col);
};
