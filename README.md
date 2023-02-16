# Hyperspectral camera calibration using OpenCL

## Requirements
- Microsoft Visual Studio 2019 supporting C++17
- XIMEA software package (xiApi, drivers)
- GPU drivers supporting OpenCL
- XIMEA hyperspectral camera

## Visual Studio Solution
The solution contains:
- the implementation of hyperspectral camera calibration using OpenCL
- unit tests

## To run:
Both the implementation and tests are run using Microsoft Visual Studio 2019. The camera has to be connected to the computer prior to the execution of the program. After the start of the program, the program performs a benchmark of implemented algorithms. After the benchmark is done, the program start displaying data. Unit tests do not require a camera to be connected.

COMPARISON_ITERATIONS is a macro defined in main.cpp, which represents the number of iterations used in the benchmark of the implemented algorithms. The value of this macro should be set to 0 if benchmark is not wanted.

## Authors
Tri Le Mau
