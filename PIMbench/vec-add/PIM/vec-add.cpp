// Test: C++ version of vector addition
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include <iostream>
#include <vector>
#include <getopt.h>
#include <stdint.h>
#include <iomanip>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include "util.h"
#include "libpimeval.h"

using namespace std;

// Params ---------------------------------------------------------------------
typedef struct Params
{
  uint64_t vectorLength;
  char *configFile;
  char *inputFile;
  bool shouldVerify;
} Params;

void usage()
{
  fprintf(stderr,
          "\nUsage:  ./vec-add.out [options]"
          "\n"
          "\n    -l    input size (default=2048 elements)"
          "\n    -c    dramsim config file"
          "\n    -i    input file containing two vectors (default=generates vector with random numbers)"
          "\n    -v    t = verifies PIM output with host output. (default=false)"
          "\n");
}

struct Params getInputParams(int argc, char **argv)
{
  struct Params p;
  p.vectorLength = 2048;
  p.configFile = nullptr;
  p.inputFile = nullptr;
  p.shouldVerify = false;

  int opt;
  while ((opt = getopt(argc, argv, "h:l:c:i:v:")) >= 0)
  {
    switch (opt)
    {
    case 'h':
      usage();
      exit(0);
      break;
    case 'l':
      p.vectorLength = strtoull(optarg, NULL, 0);
      break;
    case 'c':
      p.configFile = optarg;
      break;
    case 'i':
      p.inputFile = optarg;
      break;
    case 'v':
      p.shouldVerify = (*optarg == 't') ? true : false;
      break;
    default:
      fprintf(stderr, "\nUnrecognized option!\n");
      usage();
      exit(0);
    }
  }
  return p;
}

void vectorAddition(uint64_t vectorLength, std::vector<int> &src1, std::vector<int> &src2, std::vector<int> &dst)
{
  PimObjId srcObj1 = pimAlloc(PIM_ALLOC_AUTO, vectorLength, PIM_INT32);
  if (srcObj1 == -1)
  {
    std::cout << "Abort" << std::endl;
    return;
  }
  PimObjId srcObj2 = pimAllocAssociated(srcObj1, PIM_INT32);
  if (srcObj2 == -1)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  PimStatus status = pimCopyHostToDevice((void *)src1.data(), srcObj1);
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  status = pimCopyHostToDevice((void *)src2.data(), srcObj2);
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  status = pimAdd(srcObj1, srcObj2, srcObj1);
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  dst.resize(vectorLength);
  status = pimCopyDeviceToHost(srcObj1, (void *)dst.data());
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
  }
  pimFree(srcObj1);
  pimFree(srcObj2);
}

int main(int argc, char* argv[])
{
  struct Params params = getInputParams(argc, argv);
  std::cout << "Running Vector Add on PIM for vector length: " << params.vectorLength << "\n\n";
  std::vector<int> src1(params.vectorLength, 1), src2(params.vectorLength, 2), dst;
  if (params.shouldVerify) {  
    if (params.inputFile == nullptr)
    {
      getVector(params.vectorLength, src1);
      getVector(params.vectorLength, src2);
    } else {
      std::cout << "Reading from input file is not implemented yet." << std::endl;
      return 1;
    }
  }
  if (!createDevice(params.configFile)) return 1;
  //TODO: Check if vector can fit in one iteration. Otherwise need to run addition in multiple iteration.
  vectorAddition(params.vectorLength, src1, src2, dst);
  if (params.shouldVerify) {
    // verify result
    #pragma omp parallel for
    for (unsigned i = 0; i < params.vectorLength; ++i)
    {
      int sum = src1[i] + src2[i];
      if (dst[i] != sum)
      {
        std::cout << "Wrong answer for addition: " << src1[i] << " + " << src2[i] << " = " << dst[i] << " (expected " << sum << ")" << std::endl;
      }
    }
  }

  pimShowStats();

  return 0;
}
