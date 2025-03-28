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
  unsigned numSubarrayPerBank = 8;
  unsigned numRows = 128;
  unsigned numCols = 256;

  PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, numSubarrayPerBank, numRows, numCols);
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

bool testRowHammer()
{
  unsigned numElements = 1000;

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
    src3[i] = i * 3;
  }

  PimStatus status = pimCopyHostToDevice((void*)src1.data(), obj1);
  assert(status == PIM_OK);
  status = pimCopyHostToDevice((void*)src2.data(), obj2);
  assert(status == PIM_OK);
  status = pimCopyHostToDevice((void*)src3.data(), obj3);
  assert(status == PIM_OK);

  int numBits = 32;
  bool useScalar = 0; 
  uint64_t scalarVal = 32;

  // for (int i = 0; i < 10; ++i) {
    pimOpSet(obj1, PIM_RREG_R1, 0);
    for (int i = 0; i < numBits; ++i) {
      pimOpReadRowToSa(obj1, i);
      pimOpXor(obj1, PIM_RREG_SA, PIM_RREG_R1, PIM_RREG_R2);
      implReadRowOrScalar(obj1, i, useScalar, scalarVal);
      pimOpSel(obj1, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_R1, PIM_RREG_R1);
      pimOpXor(obj1, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_SA);
      pimOpWriteSaToRow(obj1, i);
    }
  // }

  return true;
}

int main()
{
  std::cout << "PIM test: BitSIMD-V basic" << std::endl;

  bool ok = createPimDevice();
  if (!ok) {
    std::cout << "PIM device creation failed!" << std::endl;
    return 1;
  }

  ok = testRowHammer();
  if (!ok) {
    std::cout << "Test failed!" << std::endl;
    return 1;
  }

  pimShowStats();
  std::cout << "All correct!" << std::endl;
  return 0;
}