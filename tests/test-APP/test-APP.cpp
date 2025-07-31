// Test: SIMDRAM micro-ops
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "libpimeval.h"
#include <iostream>
#include <vector>
#include <cassert>


bool createPimDevice()
{
  unsigned numSubarrayPerBank = 8;
  unsigned numRows = 128;
  unsigned numCols = 256;

  PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_H, 1, 1, numSubarrayPerBank, numRows, numCols);
  assert(status == PIM_OK);
  return true;
}

bool testMicroOps()
{
  unsigned numElements = 10;

  std::vector<int> src1(numElements);
  std::vector<int> src2(numElements);
  std::vector<int> dest1(numElements);
  std::vector<int> dest2(numElements);
  for (unsigned i = 0; i < numElements; ++i) {
    src1[i] = static_cast<int>(i);
    src2[i] = static_cast<int>(i * 3 + 1);
  }

  PimObjId obj1 = pimAlloc(PIM_ALLOC_H1, numElements, PIM_INT32);
  assert(obj1 != -1);
  PimObjId obj2 = pimAllocAssociated(obj1, PIM_INT32);
  assert(obj2 != -1);
  PimObjId obj3 = pimCreateDualContactRef(obj2);
  assert(obj3 != -1);

  // Test APP_GND (OR)
  PimStatus status = pimCopyHostToDevice((void*)src1.data(), obj1);
  assert(status == PIM_OK);
  status = pimCopyHostToDevice((void*)src2.data(), obj2);
  assert(status == PIM_OK);
  status = pimOpAPP_GND(obj1, 0);
  assert(status == PIM_OK);
  status = pimOpReadRowToSa(obj2, 0);
  assert(status == PIM_OK);
  status = pimOpWriteSaToRow(obj2, 0);
  assert(status == PIM_OK);
  status = pimCopyDeviceToHost(obj1, (void*)dest1.data());
  assert(status == PIM_OK);
  status = pimCopyDeviceToHost(obj2, (void*)dest2.data());
  assert(status == PIM_OK);
  for (unsigned i = 0; i < numElements; ++i) {
     if ((src1[i] | src2[i]) != dest2[i] || src1[i] != dest1[i]) {
        std::cout << "Row APP failed - incorrect mem contents at index " << i << std::endl;
        std::cout << "src1[" << i << "] = " << static_cast<int>(src1[i]) << std::endl;
        std::cout << "src2[" << i << "] = " << static_cast<int>(src2[i]) << std::endl;
        std::cout << "dest1[" << i << "] = " << static_cast<int>(dest1[i]) << std::endl;
        std::cout << "dest2[" << i << "] = " << static_cast<int>(dest2[i]) << std::endl;
        return false;
    } else {
      // std::cout << "Row APP pass - correct mem contents at index " << i << std::endl;
      //   std::cout << "src1[" << i << "] = " << static_cast<int>(src1[i]) << std::endl;
      //   std::cout << "src2[" << i << "] = " << static_cast<int>(src2[i]) << std::endl;
      //   std::cout << "dest1[" << i << "] = " << static_cast<int>(dest1[i]) << std::endl;
      //   std::cout << "dest2[" << i << "] = " << static_cast<int>(dest2[i]) << std::endl;
    }
  }
  std::cout << "APP_GND pass" << std::endl;
  
  //Test APP_VDD (and)
  status = pimCopyHostToDevice((void*)src1.data(), obj1);
  assert(status == PIM_OK);
  status = pimCopyHostToDevice((void*)src2.data(), obj2);
  assert(status == PIM_OK);
  status = pimOpAPP_VDD(obj1, 0);
  assert(status == PIM_OK);
  status = pimOpReadRowToSa(obj2, 0);
  assert(status == PIM_OK);
  status = pimOpWriteSaToRow(obj2, 0);
  assert(status == PIM_OK);
  status = pimCopyDeviceToHost(obj1, (void*)dest1.data());
  assert(status == PIM_OK);
  status = pimCopyDeviceToHost(obj2, (void*)dest2.data());
  assert(status == PIM_OK);
  for (unsigned i = 0; i < numElements; ++i) {
     if ((src1[i] & src2[i]) != dest2[i] || src1[i] != dest1[i]) {
        std::cout << "Row APP failed - incorrect mem contents at index " << i << std::endl;
        std::cout << "src1[" << i << "] = " << static_cast<int>(src1[i]) << std::endl;
        std::cout << "src2[" << i << "] = " << static_cast<int>(src2[i]) << std::endl;
        std::cout << "dest1[" << i << "] = " << static_cast<int>(dest1[i]) << std::endl;
        std::cout << "dest2[" << i << "] = " << static_cast<int>(dest2[i]) << std::endl;
        return false;
    } else {
      // std::cout << "Row APP pass - correct mem contents at index " << i << std::endl;
      //   std::cout << "src1[" << i << "] = " << static_cast<int>(src1[i]) << std::endl;
      //   std::cout << "src2[" << i << "] = " << static_cast<int>(src2[i]) << std::endl;
      //   std::cout << "dest1[" << i << "] = " << static_cast<int>(dest1[i]) << std::endl;
      //   std::cout << "dest2[" << i << "] = " << static_cast<int>(dest2[i]) << std::endl;
    }
  }
  std::cout << "APP_VDD pass" << std::endl;

  return true;
}

int main()
{
  std::cout << "PIM test: SIMDRAM micro-ops" << std::endl;

  bool ok = createPimDevice();
  if (!ok) {
    std::cout << "PIM device creation failed!" << std::endl;
    return 1;
  }

  ok = testMicroOps();
  if (!ok) {
    std::cout << "Test failed!" << std::endl;
    return 1;
  }

  pimShowStats();
  std::cout << "All correct!" << std::endl;
  return 0;
}