#ifndef pathdata_h
#define pathdata_h

// Traced path data structure
struct PathData {
  int16_t leftSteps;
  uint16_t leftTicks;
  int16_t rightSteps;
  uint16_t rightTicks;
};

const PathData tracedPath[] = {
    {5000, 32000, 100, 32000}, {5000, 32000, 100, 32000},
    {5000, 32000, 100, 32000}, {5000, 32000, 100, 32000},
    {5000, 32000, 100, 32000}, {500, 32000, 100, 32000},
    {500, 32000, 100, 32000},  {500, 32000, 100, 32000},
    {500, 32000, 100, 32000},  {500, 32000, 100, 32000},
    {5000, 32000, 100, 32000}, {5000, 32000, 100, 32000},
    {5000, 32000, 100, 32000}, {5000, 32000, 100, 32000},
    {5000, 32000, 100, 32000},
};

const int pathLength = sizeof(tracedPath) / sizeof(tracedPath[0]);

#endif