#pragma once

#ifndef __pangolinlib__
#define __pandolinlib__

#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
//using namespace std;

#include <Eigen/Core>
#include <Eigen/Geometry>
//using namespace Eigen;

#include <pangolin/pangolin.h>
//多线程的库
#include <thread>
#include <regex>

void pview(std::string file);

#endif