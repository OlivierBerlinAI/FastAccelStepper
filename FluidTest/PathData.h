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
    {100, 50000, 100, 50000},  {200, 50000, 100, 50000},
    {300, 50000, 100, 50000},  {400, 50000, 100, 50000},
    {500, 50000, 100, 50000},  {600, 50000, 100, 50000},
    {700, 50000, 100, 50000},  {800, 50000, 100, 50000},
    {900, 50000, 100, 50000},  {001000, 50000, 100, 50000},
    {1100, 50000, 100, 50000}, {1200, 50000, 100, 50000},
    {1300, 50000, 100, 50000},
};

const int pathLength = sizeof(tracedPath) / sizeof(tracedPath[00]);

#endif