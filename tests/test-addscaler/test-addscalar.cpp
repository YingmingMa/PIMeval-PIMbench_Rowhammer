// Test: Row Hammer basic tests
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "libpimeval.h"
// #include "bitSerialBitsimd.h"
#include <iostream>
#include <vector>
#include <cassert>

bool getBit(uint64_t val, int nth) { return (val >> nth) & 1; }

bool createPimDevice()
{
  unsigned numSubarrayPerBank = 16;
  unsigned numRows = 8192;
  unsigned numCols = 1024;

  PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_V_AP, 1, 1, numSubarrayPerBank, numRows, numCols);
  assert(status == PIM_OK);
  return true;
}

void
implReadRowOrScalar(PimObjId src, unsigned bitIdx, bool useScalar, uint64_t scalarVal)
{
  if (useScalar) {
    pimOpSet(src, PIM_RREG_SA, getBit(scalarVal, bitIdx));
  } else {
    pimOpReadRowToSa(src, bitIdx);
  }
}

bool testCommands()
{
  unsigned numElements = 500;

  // bitSerialBitsimd device;

  std::vector<int> src1(numElements);
  std::vector<int> src2(numElements);
  std::vector<int> src3(numElements);
  std::vector<int> dest(numElements);

  PimObjId obj1 = pimAlloc(PIM_ALLOC_V1, numElements, PIM_INT32);
  assert(obj1 != -1);
  PimObjId obj2 = pimAllocAssociated(obj1, PIM_INT32);
  assert(obj2 != -1);
  PimObjId obj3 = pimAllocAssociated(obj1, PIM_INT32);
  assert(obj3 != -1);
  PimObjId obj4 = pimAllocAssociated(obj1, PIM_INT32);
  assert(obj4 != -1);

  // assign some initial values
  for (unsigned i = 0; i < numElements; ++i) {
    src1[i] = i;
    src2[i] = i * 2 - 10;
    // src3[i] = i * 3;
  }

  PimStatus status = pimCopyHostToDevice((void*)src1.data(), obj1);
  assert(status == PIM_OK);
  status = pimCopyHostToDevice((void*)src2.data(), obj2);
  assert(status == PIM_OK);
  // status = pimCopyHostToDevice((void*)src3.data(), obj3);
  // assert(status == PIM_OK);

  std::vector<int> cpu_result(numElements);

  int numBits = 32;
  bool useScalar = 1; 
  uint64_t scalarVal = 2;

  //do multiply obj1 with scaler value, store it to obj3

  // cond copy the first
  pimOpReadRowToSa(obj1, 0);
  pimOpMove(obj1, PIM_RREG_SA, PIM_RREG_R2);
  pimOpSet(obj1, PIM_RREG_R1, 0);
  for (int i = 0; i < numBits; ++i) {
    implReadRowOrScalar(obj1, i, useScalar, scalarVal);
    pimOpSel(obj1, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_R1, PIM_RREG_SA);
    pimOpWriteSaToRow(obj3, i);
  }

  // add and cond write
  for (int i = 1; i < numBits; ++i) {
    pimOpReadRowToSa(obj1, i);
    pimOpMove(obj1, PIM_RREG_SA, PIM_RREG_R3); // cond

    pimOpSet(obj1, PIM_RREG_R1, 0); // carry
    for (int j = 0; i + j < numBits; ++j) {
      // add
      implReadRowOrScalar(obj1, j, useScalar, scalarVal);
      pimOpXnor(obj1, PIM_RREG_SA, PIM_RREG_R1, PIM_RREG_R2);
      pimOpReadRowToSa(obj3, i + j);
      pimOpSel(obj1, PIM_RREG_R2, PIM_RREG_R1, PIM_RREG_SA, PIM_RREG_R1);
      pimOpXnor(obj1, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_R2);
      // cond write
      pimOpSel(obj1, PIM_RREG_R3, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_SA);
      pimOpWriteSaToRow(obj3, i + j);
    }
  }

  useScalar = 0;
  //add obj3 with obj2 store value in obj 3
  pimOpSet(obj3, PIM_RREG_R1, 0);
  for (int i = 0; i < numBits; ++i) {
    pimOpReadRowToSa(obj3, i);
    pimOpXor(obj3, PIM_RREG_SA, PIM_RREG_R1, PIM_RREG_R2);
    implReadRowOrScalar(obj2, i, useScalar, scalarVal);
    pimOpSel(obj3, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_R1, PIM_RREG_R1);
    pimOpXor(obj3, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_SA);
    pimOpWriteSaToRow(obj2, i);
  }

  // Validate result using CPU computation
  for (unsigned i = 0; i < numElements; ++i) {
    cpu_result[i] = src1[i] * scalarVal + src2[i];
  }

  dest.resize(numElements);
  PimStatus copy_status = pimCopyDeviceToHost(obj2, dest.data());
  assert(copy_status == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i) {
    if (cpu_result[i] != dest[i]) {
      std::cerr << "Mismatch at index " << i << ": expected " << cpu_result[i] << ", got " << dest[i] << std::endl;
      return false;
    }
  }

  std::cout << "All results match!" << std::endl;
  
  return true;
}

int main()
{
  std::cout << "PIM test: BitSIMD-V_AP ScallerADD" << std::endl;

  bool ok = createPimDevice();
  if (!ok) {
    std::cout << "PIM device creation failed!" << std::endl;
    return 1;
  }

  ok = testCommands();
  if (!ok) {
    std::cout << "Test failed!" << std::endl;
    return 1;
  }

  pimShowStats();
  return 0;
}