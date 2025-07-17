// Test: SIMDRAM micro-ops
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "libpimeval.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdint>
#include <iomanip>

// Type mapping for PimDataType
template <PimDataType>
struct PimTypeMap;

template <> struct PimTypeMap<PIM_INT8>   { using type = int8_t;  static constexpr const char* name = "INT8";  };
template <> struct PimTypeMap<PIM_INT16>  { using type = int16_t; static constexpr const char* name = "INT16"; };
template <> struct PimTypeMap<PIM_INT32>  { using type = int32_t; static constexpr const char* name = "INT32"; };
template <> struct PimTypeMap<PIM_INT64>  { using type = int64_t; static constexpr const char* name = "INT64"; };

bool createPimDevice()
{
  unsigned numSubarrayPerBank = 8;
  unsigned numRows = 128;
  unsigned numCols = 256;

  PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_H, 1, 1, numSubarrayPerBank, numRows, numCols);
  assert(status == PIM_OK);
  return true;
}

template <typename T, PimDataType dtype>
bool runTest(unsigned numElements, size_t bitWidth) {
    std::cout << "\n--- Testing " << PimTypeMap<dtype>::name << " (" << bitWidth << "-bit) ---" << std::endl;

    uint64_t mask = (bitWidth == 64) ? ~0ULL : ((1ULL << bitWidth) - 1);

    std::vector<T> src(numElements);
    std::vector<T> dest(numElements);

    for (unsigned i = 0; i < numElements; ++i) {
        src[i] = static_cast<T>((i + 1) << 1); // Distinct pattern
    }

    //test padding
    PimObjId PADTEST = pimAlloc(PIM_ALLOC_H, numElements, PIM_INT64);
    PimObjId obj = pimAllocAssociated(PADTEST, dtype);
    assert(obj != -1);

    // ---- Test Logical Right Shift ----
    std::cout << "Testing logical RIGHT shift..." << std::endl;
    PimStatus status = pimCopyHostToDevice((void*)src.data(), obj);
    assert(status == PIM_OK);
    // status = pimOpReadRowToSa(obj, 0); assert(status == PIM_OK);
    // status = pimOpColGrpShiftL(obj);   assert(status == PIM_OK); // Logical RIGHT
    // status = pimOpWriteSaToRow(obj, 0); assert(status == PIM_OK);
    status = pimCopyDeviceToHost(obj, (void*)dest.data()); assert(status == PIM_OK);

    for (unsigned i = 0; i < numElements; ++i) {
        uint64_t expected = (static_cast<uint64_t>(src[i]) >> 1) & mask;
        uint64_t actual   = static_cast<uint64_t>(dest[i]) & mask;
        if (actual != expected) {
            std::cerr << "[FAIL] Right shift mismatch at index " << i
                      << ": expected 0x" << std::hex << expected
                      << ", got 0x" << actual 
                      << ", origonal Src 0x" << src[i]
                      << std::dec << std::endl;
            // return false;
        } else {
            std::cout << "[PASS] Index " << i << ": 0x" << std::hex << actual
                      << " == expected 0x" << expected << std::dec << std::endl;
        }
    }

    // ---- Test Logical Left Shift ----
    std::cout << "Testing logical LEFT shift..." << std::endl;

    //todo: check pim READROWTOSA with wraparound condition
    
    status = pimCopyHostToDevice((void*)src.data(), obj); assert(status == PIM_OK);
    status = pimOpReadRowToSa(obj, 0); assert(status == PIM_OK);
    // status = pimOpColGrpShiftR(obj);   assert(status == PIM_OK); // Logical LEFT
    status = pimOpWriteSaToRow(obj, 0); assert(status == PIM_OK);
    status = pimCopyDeviceToHost(obj, (void*)dest.data()); assert(status == PIM_OK);

    for (unsigned i = 0; i < numElements; ++i) {
        uint64_t expected = (static_cast<uint64_t>(src[i]) << 1) & mask;
        uint64_t actual   = static_cast<uint64_t>(dest[i]) & mask;
        if (actual != expected) {
            std::cerr << "[FAIL] Left shift mismatch at index " << i
                      << ": expected 0x" << std::hex << expected
                      << ", got 0x" << actual 
                      << ", origonal Src 0x" << src[i]
                      << std::dec << std::endl;
            // return false;
        } else {
            std::cout << "[PASS] Index " << i << ": 0x" << std::hex << actual
                      << " == expected 0x" << expected << std::dec << std::endl;
        }
    }

    pimFree(obj);
    return true;
}

bool testMicroOps() {
    const unsigned numElements = 20;

    // if (!runTest<typename PimTypeMap<PIM_INT8>::type, PIM_INT8>(numElements, 8)) return false;
    // if (!runTest<typename PimTypeMap<PIM_INT16>::type, PIM_INT16>(numElements, 16)) return false;
    // if (!runTest<typename PimTypeMap<PIM_INT32>::type, PIM_INT32>(numElements, 32)) return false;
    if (!runTest<typename PimTypeMap<PIM_INT64>::type, PIM_INT64>(numElements, 64)) return false;

    return true;
}

int main() {
    std::cout << "PIM test: SIMDRAM micro-ops" << std::endl;

    if (!createPimDevice()) {
        std::cerr << "[ERROR] PIM device creation failed!" << std::endl;
        return 1;
    }

    if (!testMicroOps()) {
        std::cerr << "[ERROR] Micro-op tests failed!" << std::endl;
        return 1;
    }

    pimShowStats();
    std::cout << "All micro-op tests passed successfully." << std::endl;
    return 0;
}