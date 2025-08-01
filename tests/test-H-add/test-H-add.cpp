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

template <>
struct PimTypeMap<PIM_INT8>
{
  using type = int8_t;
  static constexpr const char *name = "INT8";
};
template <>
struct PimTypeMap<PIM_INT16>
{
  using type = int16_t;
  static constexpr const char *name = "INT16";
};
template <>
struct PimTypeMap<PIM_INT32>
{
  using type = int32_t;
  static constexpr const char *name = "INT32";
};
template <>
struct PimTypeMap<PIM_INT64>
{
  using type = int64_t;
  static constexpr const char *name = "INT64";
};

bool createPimDevice()
{
  unsigned numSubarrayPerBank = 8;
  unsigned numRows = 128;
  unsigned numCols = 256;

  PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_H, 1, 1, numSubarrayPerBank, numRows, numCols);
  assert(status == PIM_OK);
  return true;
}

bool debugFunc(PimObjId obj, std::vector<int32_t> dest, unsigned numElements)
{
  assert(pimCopyDeviceToHost(obj, (void *)dest.data()) == PIM_OK);

  numElements = 1;

  for (unsigned i = 0; i < numElements; ++i){
    std::cerr << "[DEBUG] Index " << i << ": " << dest[i] << std::endl;
  }

  return true;
}

bool testMicroOpsBasic()
{
  const unsigned numElements = 10;

  std::vector<int32_t> src1(numElements);
  std::vector<int32_t> src2(numElements);
  std::vector<int32_t> zeros(numElements, 0);
  std::vector<int32_t> ones(numElements, -1);
  std::vector<int32_t> dest(numElements);

  for (unsigned i = 0; i < numElements; ++i)
  {
    src1[i] = static_cast<int32_t>(i * 3 + 1); // num1
    src2[i] = static_cast<int32_t>(i * 7 + 5); // num2
  }

  // Main data objects
  PimObjId xorVal = pimAlloc(PIM_ALLOC_H, numElements, PIM_INT32);
  assert(xorVal != -1);

  PimObjId carry = pimAllocAssociated(xorVal, PIM_INT32);
  assert(carry != -1);

  PimObjId andVal = pimAllocAssociated(xorVal, PIM_INT32);
  assert(andVal != -1);

  PimObjId tmp_or = pimAllocAssociated(xorVal, PIM_INT32);
  assert(tmp_or != -1);

  PimObjId tmp_and = pimAllocAssociated(xorVal, PIM_INT32);
  assert(tmp_and != -1);

  // Constants and backups
  PimObjId zeroRow = pimAllocAssociated(xorVal, PIM_INT32);
  assert(zeroRow != -1);

  PimObjId oneRow = pimAllocAssociated(xorVal, PIM_INT32);
  assert(oneRow != -1);

  PimObjId zeroRowInit = pimAllocAssociated(xorVal, PIM_INT32);
  assert(zeroRowInit != -1);

  PimObjId oneRowInit = pimAllocAssociated(xorVal, PIM_INT32);
  assert(oneRowInit != -1);

  // Temp copies of xor and carry (to preserve before destructive AAPs)
  PimObjId xorValTmp = pimAllocAssociated(xorVal, PIM_INT32);
  assert(xorValTmp != -1);

  PimObjId carryTmp = pimAllocAssociated(xorVal, PIM_INT32);
  assert(carryTmp != -1);

  // DCC for ~tmp_and
  PimObjId tmp_and_dcc = pimCreateDualContactRef(tmp_and);
  assert(tmp_and_dcc != -1);

  // Copy input and constants
  assert(pimCopyHostToDevice((void *)src1.data(), xorVal) == PIM_OK); // a
  assert(pimCopyHostToDevice((void *)src2.data(), carry) == PIM_OK);  // b
  assert(pimCopyHostToDevice((void *)zeros.data(), zeroRowInit) == PIM_OK);
  assert(pimCopyHostToDevice((void *)ones.data(), oneRowInit) == PIM_OK);
  // assert(pimOpAP(1, zeroRow, 0, zeroRowInit, 0, zeroRowInit, 0) == PIM_OK); // zeroRow ← zeroRowInit
  // assert(pimOpAP(1, oneRow,  0, oneRowInit,  0, oneRowInit,  0) == PIM_OK); // oneRow  ← oneRowInit

  for (int i = 0; i < 32; ++i)
  {
    // Refresh constants before using them in AAP
    assert(pimCopyObjectToObject(zeroRowInit, zeroRow) == PIM_OK);
    assert(pimCopyObjectToObject(oneRowInit, oneRow) == PIM_OK);

    // Preserve xorVal and carry
    assert(pimCopyObjectToObject(xorVal, xorValTmp) == PIM_OK);
    assert(pimCopyObjectToObject(carry, carryTmp) == PIM_OK);

    // andVal = MAJ(xorVal, carry, 0)
    assert(pimOpAAP(3, 1, xorVal, 0, carry, 0, zeroRow, 0, andVal, 0) == PIM_OK);

    // tmp_or = MAJ(xorValTmp, carryTmp, 1)
    assert(pimOpAAP(3, 1, xorValTmp, 0, carryTmp, 0, oneRow, 0, tmp_or, 0) == PIM_OK);

    // tmp_and = andVal (safe copy)
    assert(pimCopyObjectToObject(andVal, tmp_and) == PIM_OK);

    // restore zero row value
    assert(pimCopyObjectToObject(zeroRowInit, zeroRow) == PIM_OK);

    // xorVal = tmp_or & ~tmp_and = MAJ(tmp_or, tmp_and_dcc, 0)
    assert(pimOpAAP(3, 1, tmp_or, 0, tmp_and_dcc, 0, zeroRow, 0, xorVal, 0) == PIM_OK);

    // carry = andVal << 1
    assert(pimOpReadRowToSa(andVal, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(andVal, 1) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(carry, 0) == PIM_OK);
  }

  // Copy result back and validate
  assert(pimCopyDeviceToHost(xorVal, (void *)dest.data()) == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i)
  {
    int32_t expected = src1[i] + src2[i];
    if (dest[i] != expected)
    {
      std::cerr << "[FAIL] Index " << i << ": " << src1[i]
                << " + " << src2[i] << " = " << expected
                << ", but got " << dest[i] << std::endl;
      return false;
    }
  }

  std::cout << "Bit-serial 32-bit adder with correct AAP/AP usage OK." << std::endl;
  return true;
}

bool testMicroOpsAdvance()
{
  const unsigned numElements = 1;

  std::vector<int32_t> src1(numElements);
  std::vector<int32_t> src2(numElements);
  std::vector<int32_t> zeros(numElements, 0);
  std::vector<int32_t> ones(numElements, -1);
  std::vector<int32_t> dest(numElements);

  for (unsigned i = 0; i < numElements; ++i)
  {
    src1[i] = static_cast<int32_t>(i * 3 + 1); // num1
    src2[i] = static_cast<int32_t>(i * 7 + 5); // num2
  }

  // Main data objects
  PimObjId xorVal = pimAlloc(PIM_ALLOC_H, numElements, PIM_INT32);
  assert(xorVal != -1);

  PimObjId carry = pimAllocAssociated(xorVal, PIM_INT32);
  assert(carry != -1);

  PimObjId andVal = pimAllocAssociated(xorVal, PIM_INT32);
  assert(andVal != -1);

  PimObjId tmp_and = pimAllocAssociated(xorVal, PIM_INT32);
  assert(tmp_and != -1);

  // Constants and backups
  PimObjId zeroRow = pimAllocAssociated(xorVal, PIM_INT32);
  assert(zeroRow != -1);

  PimObjId oneRow = pimAllocAssociated(xorVal, PIM_INT32);
  assert(oneRow != -1);

  PimObjId zeroRowInit = pimAllocAssociated(xorVal, PIM_INT32);
  assert(zeroRowInit != -1);

  PimObjId oneRowInit = pimAllocAssociated(xorVal, PIM_INT32);
  assert(oneRowInit != -1);

  // Temp copies of xor and carry (to preserve before destructive AAPs)
  PimObjId xorValTmp = pimAllocAssociated(xorVal, PIM_INT32);
  assert(xorValTmp != -1);

  PimObjId carryTmp = pimAllocAssociated(xorVal, PIM_INT32);
  assert(carryTmp != -1);

  // DCC for ~tmp_and
  PimObjId tmp_and_dcc = pimCreateDualContactRef(tmp_and);
  assert(tmp_and_dcc != -1);

  // Copy input and constants
  assert(pimCopyHostToDevice((void *)src1.data(), xorVal) == PIM_OK); // a
  assert(pimCopyHostToDevice((void *)src2.data(), carry) == PIM_OK);  // b
  assert(pimCopyHostToDevice((void *)zeros.data(), zeroRowInit) == PIM_OK);
  assert(pimCopyHostToDevice((void *)ones.data(), oneRowInit) == PIM_OK);

  //assure the temp variable is updated
  assert(pimCopyObjectToObject(xorVal, xorValTmp) == PIM_OK);

  for (int i = 0; i < 32; ++i)
  {
    // Refresh constants before using them in AAP
    assert(pimCopyObjectToObject(zeroRowInit, zeroRow) == PIM_OK);
    assert(pimCopyObjectToObject(oneRowInit, oneRow) == PIM_OK);

    // Preserve xorVal and carry
    assert(pimCopyObjectToObject(carry, carryTmp) == PIM_OK);

    // andVal = MAJ(xorVal, carry, 0)
    assert(pimOpAAP(3, 2, xorVal, 0, carry, 0, zeroRow, 0, andVal, 0, tmp_and, 0) == PIM_OK);

    // oneRow(tmp_or) = MAJ(xorValTmp, carryTmp, 1)
    assert(pimOpAP(3, xorValTmp, 0, carryTmp, 0, oneRow, 0) == PIM_OK);

    // restore zero row value
    assert(pimCopyObjectToObject(zeroRowInit, zeroRow) == PIM_OK);

    // xorVal = oneRow(tmp_or) & ~tmp_and = MAJ(tmp_or, tmp_and_dcc, 0)
    assert(pimOpAAP(3, 2, oneRow, 0, tmp_and_dcc, 0, zeroRow, 0, xorVal, 0, xorValTmp, 0) == PIM_OK);

    // carry = andVal << 1
    assert(pimOpReadRowToSa(andVal, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(andVal, 1) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(carry, 0) == PIM_OK);
  }

  // Copy result back and validate
  assert(pimCopyDeviceToHost(xorVal, (void *)dest.data()) == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i)
  {
    int32_t expected = src1[i] + src2[i];
    if (dest[i] != expected)
    {
      std::cerr << "[FAIL] Index " << i << ": " << src1[i]
                << " + " << src2[i] << " = " << expected
                << ", but got " << dest[i] << std::endl;
      return false;
    }
  }

  std::cout << "Bit-serial 32-bit adder with correct AAP/AP usage OK." << std::endl;
  return true;
}

bool testMicroOpsOnlyAAPandShift()
{
  const unsigned numElements = 1;

  std::vector<int32_t> src1(numElements);
  std::vector<int32_t> src2(numElements);
  std::vector<int32_t> zeros(numElements, 0);
  std::vector<int32_t> ones(numElements, -1);
  std::vector<int32_t> dest(numElements);

  for (unsigned i = 0; i < numElements; ++i)
  {
    src1[i] = static_cast<int32_t>(i * 3 + 1); // num1
    src2[i] = static_cast<int32_t>(i * 7 + 5); // num2
  }

  // Main data objects
  PimObjId xorVal = pimAlloc(PIM_ALLOC_H, numElements, PIM_INT32);
  assert(xorVal != -1);

  PimObjId carry = pimAllocAssociated(xorVal, PIM_INT32);
  assert(carry != -1);

  PimObjId andVal = pimAllocAssociated(xorVal, PIM_INT32);
  assert(andVal != -1);

  PimObjId tmp_and = pimAllocAssociated(xorVal, PIM_INT32);
  assert(tmp_and != -1);

  // Constants and backups
  PimObjId zeroRow = pimAllocAssociated(xorVal, PIM_INT32);
  assert(zeroRow != -1);

  PimObjId oneRow = pimAllocAssociated(xorVal, PIM_INT32);
  assert(oneRow != -1);

  PimObjId zeroRowInit = pimAllocAssociated(xorVal, PIM_INT32);
  assert(zeroRowInit != -1);

  PimObjId oneRowInit = pimAllocAssociated(xorVal, PIM_INT32);
  assert(oneRowInit != -1);

  // Temp copies of xor and carry (to preserve before destructive AAPs)
  PimObjId xorValTmp = pimAllocAssociated(xorVal, PIM_INT32);
  assert(xorValTmp != -1);

  PimObjId carryTmp = pimAllocAssociated(xorVal, PIM_INT32);
  assert(carryTmp != -1);

  // DCC for ~tmp_and
  PimObjId tmp_and_dcc = pimCreateDualContactRef(tmp_and);
  assert(tmp_and_dcc != -1);

  // Copy input and constants
  assert(pimCopyHostToDevice((void *)src1.data(), xorVal) == PIM_OK); // a
  assert(pimCopyHostToDevice((void *)src2.data(), carry) == PIM_OK);  // b
  assert(pimCopyHostToDevice((void *)zeros.data(), zeroRowInit) == PIM_OK);
  assert(pimCopyHostToDevice((void *)ones.data(), oneRowInit) == PIM_OK);

  //assure the temp variable is updated
  assert(pimOpAAP(1, 1, xorVal, 0, xorValTmp, 0) == PIM_OK);

  for (int i = 0; i < 32; ++i)
  {
    // Refresh constants before using them in AAP
    assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);
    assert(pimOpAAP(1, 1, oneRowInit, 0, oneRow, 0) == PIM_OK);

    // Preserve xorVal and carry
    assert(pimOpAAP(1, 1, carry, 0, carryTmp, 0) == PIM_OK);

    // andVal = MAJ(xorVal, carry, 0)
    assert(pimOpAAP(3, 2, xorVal, 0, carry, 0, zeroRow, 0, andVal, 0, tmp_and, 0) == PIM_OK);

    // oneRow(tmp_or) = MAJ(xorValTmp, carryTmp, 1)
    assert(pimOpAP(3, xorValTmp, 0, carryTmp, 0, oneRow, 0) == PIM_OK);

    // restore zero row value
    assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);

    // xorVal = oneRow(tmp_or) & ~tmp_and = MAJ(tmp_or, tmp_and_dcc, 0)
    assert(pimOpAAP(3, 2, oneRow, 0, tmp_and_dcc, 0, zeroRow, 0, xorVal, 0, xorValTmp, 0) == PIM_OK);

    // carry = andVal << 1
    assert(pimOpReadRowToSa(andVal, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(andVal, 1) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(carry, 0) == PIM_OK);
  }

  // Copy result back and validate
  assert(pimCopyDeviceToHost(xorVal, (void *)dest.data()) == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i)
  {
    int32_t expected = src1[i] + src2[i];
    if (dest[i] != expected)
    {
      std::cerr << "[FAIL] Index " << i << ": " << src1[i]
                << " + " << src2[i] << " = " << expected
                << ", but got " << dest[i] << std::endl;
      return false;
    }
  }

  std::cout << "Bit-serial 32-bit adder with correct AAP/AP usage OK." << std::endl;
  return true;
}

bool testkoggeStoneAdd(){
  const unsigned numElements = 10;

  std::vector<int32_t> src1(numElements);
  std::vector<int32_t> src2(numElements);
  std::vector<int32_t> zeros(numElements, 0);
  std::vector<int32_t> ones(numElements, -1);
  std::vector<int32_t> dest(numElements);

  for (unsigned i = 0; i < numElements; ++i)
  {
    src1[i] = static_cast<int32_t>(i * 3 + 1); // num1
    src2[i] = static_cast<int32_t>(i * 7 + 5); // num2
  }

  // Main data objects
  PimObjId G = pimAlloc(PIM_ALLOC_H1, numElements, PIM_INT32);
  assert(G != -1);

  PimObjId P0 = pimAllocAssociated(G, PIM_INT32);
  assert(P0 != -1);

  PimObjId G_temp1 = pimAllocAssociated(G, PIM_INT32);
  assert(G_temp1 != -1);

  PimObjId P = pimAllocAssociated(G, PIM_INT32);
  assert(P != -1);

  PimObjId P_temp1 = pimAllocAssociated(G, PIM_INT32);
  assert(P_temp1 != -1);

  PimObjId P_temp2 = pimAllocAssociated(G, PIM_INT32);
  assert(P_temp2 != -1);

  // Constants and backups
  PimObjId zeroRow = pimAllocAssociated(G, PIM_INT32);
  assert(zeroRow != -1);

  PimObjId oneRow = pimAllocAssociated(G, PIM_INT32);
  assert(oneRow != -1);

  PimObjId zeroRowInit = pimAllocAssociated(G, PIM_INT32);
  assert(zeroRowInit != -1);

  PimObjId oneRowInit = pimAllocAssociated(G, PIM_INT32);
  assert(oneRowInit != -1);

  // DCC for ~P0
  PimObjId not_P = pimCreateDualContactRef(P0);
  assert(not_P != -1);

  // Copy input and constants
  assert(pimCopyHostToDevice((void *)src1.data(), G) == PIM_OK); // a
  assert(pimCopyHostToDevice((void *)src2.data(), P0) == PIM_OK);  // b
  assert(pimCopyHostToDevice((void *)zeros.data(), zeroRowInit) == PIM_OK);
  assert(pimCopyHostToDevice((void *)ones.data(), oneRowInit) == PIM_OK);

  //assure the temp variable for initial xor
  assert(pimOpAAP(1, 1, G, 0, P, 0) == PIM_OK);
  assert(pimOpAAP(1, 1, P0, 0, P_temp1, 0) == PIM_OK);

  //Cory Zero and ones to ther temp row
  assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);
  assert(pimOpAAP(1, 1, oneRowInit, 0, oneRow, 0) == PIM_OK);

  //initial xor
  // a & b
  assert(pimOpAAP(3, 1, G, 0, P0, 0, zeroRow, 0, G_temp1, 0) == PIM_OK);

  // restore zero row value
  assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);

  // a | b
  assert(pimOpAP(3, P, 0, P_temp1, 0, oneRow, 0) == PIM_OK);

  // restore one row value
  assert(pimOpAAP(1, 1, oneRowInit, 0, oneRow, 0) == PIM_OK);

  // a ^ b
  assert(pimOpAAP(3, 2, not_P, 0, P, 0, zeroRow, 0, P_temp1, 0, P_temp2, 0) == PIM_OK);

  // restore zero row value
  assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);

  /**********
  initial preperation finished, start repeating process
  **********/

  for(int i = 1; i < 16; i = i*2){
    //shifting to prepare for execute
    //shift G_temp1
    assert(pimOpReadRowToSa(G_temp1, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(G_temp1, i) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(G_temp1, 0) == PIM_OK);
    //shift P_temp2
    assert(pimOpReadRowToSa(P_temp2, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(P_temp2, i) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(P_temp2, 0) == PIM_OK);

    //calculate G
    // P_(i-1) & (G_(i-1) << i)
    assert(pimOpAP(3, P_temp1, 0, G_temp1, 0, zeroRow, 0) == PIM_OK);

    // restore zero row value
    assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);

    // G = G_(i-1) | (P_(i-1) & (G_(i-1) << i))
    assert(pimOpAP(3, G, 0, G_temp1, 0, oneRow, 0) == PIM_OK);

    // restore one row value
    assert(pimOpAAP(1, 1, oneRowInit, 0, oneRow, 0) == PIM_OK);

    //calculatre P
    // P = P_(i-1) & (P_(i-1) << i)
    assert(pimOpAAP(3, 1, P, 0, P_temp2, 0, zeroRow, 0, P_temp1, 0) == PIM_OK);

    // restore zero row value
    assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);
  }

  //calculate last G and produce the last value
  //shift G_temp1
  assert(pimOpReadRowToSa(G_temp1, 0) == PIM_OK);
  assert(pimOpColGrpShiftR(G_temp1, 16) == PIM_OK); // Logical LEFT shift
  assert(pimOpWriteSaToRow(G_temp1, 0) == PIM_OK);

  //calculate G
  // P_(i-1) & (G_(i-1) << i)
  assert(pimOpAP(3, P_temp1, 0, G_temp1, 0, zeroRow, 0) == PIM_OK);

  // restore zero row value
  assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);

  // G = G_(i-1) | (P_(i-1) & (G_(i-1) << i))
  assert(pimOpAP(3, G, 0, G_temp1, 0, oneRow, 0) == PIM_OK);

  // restore one row value
  assert(pimOpAAP(1, 1, oneRowInit, 0, oneRow, 0) == PIM_OK);

  //last Xor
  assert(pimOpReadRowToSa(G_temp1, 0) == PIM_OK);
  assert(pimOpColGrpShiftR(G_temp1, 1) == PIM_OK);
  assert(pimOpWriteSaToRow(G_temp1, 0) == PIM_OK);

  assert(pimOpAAP(1, 1, G_temp1, 0, G, 0) == PIM_OK);

  assert(pimOpAAP(1, 1, not_P, 0, P, 0) == PIM_OK);

  // a & b
  assert(pimOpAP(3, G, 0, not_P, 0, zeroRow, 0) == PIM_OK);

  // restore zero row value
  assert(pimOpAAP(1, 1, zeroRowInit, 0, zeroRow, 0) == PIM_OK);

  // a | b
  assert(pimOpAP(3, G_temp1, 0, P, 0, oneRow, 0) == PIM_OK);

  // restore one row value
  assert(pimOpAAP(1, 1, oneRowInit, 0, oneRow, 0) == PIM_OK);

  // a ^ b
  assert(pimOpAP(3, P0, 0, P, 0, zeroRow, 0) == PIM_OK);

  // Copy result back and validate
  assert(pimCopyDeviceToHost(P0, (void *)dest.data()) == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i)
  {
    int32_t expected = src1[i] + src2[i];
    if (dest[i] != expected)
    {
      std::cerr << "[FAIL] Index " << i << ": " << src1[i]
                << " + " << src2[i] << " = " << expected
                << ", but got " << dest[i] << std::endl;
      return false;
    }
  }

  std::cout << "Bit-serial 32-bit adder with correct AAP/AP usage OK." << std::endl;
  return true;
}

bool testkoggeStoneAdd_APP(){
  const unsigned numElements = 10;

  std::vector<int32_t> src1(numElements);
  std::vector<int32_t> src2(numElements);
  std::vector<int32_t> zeros(numElements, 0);
  std::vector<int32_t> ones(numElements, -1);
  std::vector<int32_t> dest(numElements);

  for (unsigned i = 0; i < numElements; ++i)
  {
    src1[i] = static_cast<int32_t>(i * 3 + 1); // num1
    src2[i] = static_cast<int32_t>(i * 7 + 5); // num2
  }

  // Main data objects
  PimObjId G = pimAlloc(PIM_ALLOC_H1, numElements, PIM_INT32);
  assert(G != -1);

  PimObjId P0 = pimAllocAssociated(G, PIM_INT32);
  assert(P0 != -1);

  PimObjId P = pimAllocAssociated(G, PIM_INT32);
  assert(P != -1);

  PimObjId P_temp1 = pimAllocAssociated(G, PIM_INT32);
  assert(P_temp1 != -1);

  PimObjId G_temp1 = pimAllocAssociated(G, PIM_INT32);
  assert(G_temp1 != -1);

  // DCC for ~P0
  PimObjId not_P = pimCreateDualContactRef(P0);
  assert(not_P != -1);

  // Copy input and constants
  assert(pimCopyHostToDevice((void *)src1.data(), G) == PIM_OK); // a
  assert(pimCopyHostToDevice((void *)src2.data(), P) == PIM_OK);  // b

  assert(pimOpAAP(1, 1, G, 0, P_temp1, 0) == PIM_OK);

  // a & b
  assert(pimOpAPP_VDD(P, 0) == PIM_OK);
  assert(pimOpAAP(1, 1, G, 0, P0, 0) == PIM_OK);

  assert(pimOpAPP_GND(P, 0) == PIM_OK);
  assert(pimOpAPP_VDD(P_temp1, 0) == PIM_OK);  //  a | b
  assert(pimOpAAP(1, 1, not_P, 0, P, 0) == PIM_OK);

  for(int i = 1; i < 16; i = i*2){
    //shifting to prepare for execute
    //shift G_temp1
    assert(pimOpReadRowToSa(G, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(G, i) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(G_temp1, 0) == PIM_OK);
    //shift P_temp1
    assert(pimOpReadRowToSa(P, 0) == PIM_OK);
    assert(pimOpColGrpShiftR(P, i) == PIM_OK); // Logical LEFT shift
    assert(pimOpWriteSaToRow(P_temp1, 0) == PIM_OK);

    //calculating G
    assert(pimOpAPP_VDD(P, 0) == PIM_OK);
    assert(pimOpAPP_GND(G_temp1, 0) == PIM_OK); // G_temp1 = P & G << i
    assert(pimOpAPP_AP(G, 0) == PIM_OK); // G = G | (P & G << i)

    //calculateing P
    assert(pimOpAPP_VDD(P_temp1, 0) == PIM_OK);
    assert(pimOpAPP_AP(P, 0) == PIM_OK); // P = (P << i) & P
  }

  assert(pimOpReadRowToSa(G, 0) == PIM_OK);
  assert(pimOpColGrpShiftR(G, 16) == PIM_OK); // Logical LEFT shift
  assert(pimOpWriteSaToRow(G_temp1, 0) == PIM_OK);

  //calculating G
  assert(pimOpAPP_VDD(P, 0) == PIM_OK);
  assert(pimOpAPP_GND(G_temp1, 0) == PIM_OK); // G_temp1 = P & G << i
  assert(pimOpReadRowToSa(G, 0) == PIM_OK);  // SA = G | (P & G << i)
  assert(pimOpColGrpShiftR(G, 1) == PIM_OK);
  assert(pimOpWriteSaToRow(G, 0) == PIM_OK); // G = (G | (P & G << i)) << 1

  assert(pimOpAAP(1, 1, not_P, 0, P, 0) == PIM_OK);
  // last xor
  assert(pimOpAPP_VDD(G, 0) == PIM_OK);
  assert(pimOpAPP_AP(not_P, 0) == PIM_OK); // P0 = a & b
  
  assert(pimOpAPP_GND(P, 0) == PIM_OK);
  assert(pimOpAPP_VDD(G, 0) == PIM_OK);  //  G = a | b
  assert(pimOpAPP_AP(P0, 0) == PIM_OK);

  // Copy result back and validate
  assert(pimCopyDeviceToHost(P0, (void *)dest.data()) == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i)
  {
    int32_t expected = src1[i] + src2[i];
    if (dest[i] != expected)
    {
      std::cerr << "[FAIL] Index " << i << ": " << src1[i]
                << " + " << src2[i] << " = " << expected
                << ", but got " << dest[i] << std::endl;
      return false;
    }
  }

  std::cout << "Bit-serial 32-bit adder with correct APP usage OK." << std::endl;

  return true;
}

bool testDRISANOR(){
  const unsigned numElements = 10;

  std::vector<int32_t> src1(numElements);
  std::vector<int32_t> src2(numElements);
  std::vector<int32_t> zeros(numElements, 0);
  std::vector<int32_t> ones(numElements, -1);
  std::vector<int32_t> dest(numElements);

  for (unsigned i = 0; i < numElements; ++i)
  {
    src1[i] = static_cast<int32_t>(i * 3 + 1); // num1
    src2[i] = static_cast<int32_t>(i * 7 + 5); // num2
  }

  // Main data objects
  PimObjId G = pimAlloc(PIM_ALLOC_H1, numElements, PIM_INT32);
  assert(G != -1);

  PimObjId P = pimAllocAssociated(G, PIM_INT32);
  assert(P != -1);

  // Copy input and constants
  assert(pimCopyHostToDevice((void *)src1.data(), G) == PIM_OK); // a
  assert(pimCopyHostToDevice((void *)src2.data(), P) == PIM_OK);  // b

  //initial
  assert(pimOpReadRowToSa(G, 0) == PIM_OK);
  assert(pimOpNor(G, PIM_RREG_SA, PIM_RREG_SA, PIM_RREG_R1) == PIM_OK); //R1 = ~G
  assert(pimOpMove(G, PIM_RREG_SA, PIM_RREG_R2) == PIM_OK); // R2 = G
  assert(pimOpReadRowToSa(P, 0) == PIM_OK); //SA = P
  assert(pimOpNor(G, PIM_RREG_SA, PIM_RREG_SA, PIM_RREG_R3) == PIM_OK); //R3 = ~P
  assert(pimOpNor(G, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_R2) == PIM_OK); //R2 = A nor B
  assert(pimOpNor(G, PIM_RREG_R1, PIM_RREG_R3, PIM_RREG_R1) == PIM_OK); //R1 = G = A & B
  assert(pimOpNor(G, PIM_RREG_R1, PIM_RREG_R2, PIM_RREG_SA) == PIM_OK); //SA = P = A xor B
  assert(pimOpWriteSaToRow(P, 0) == PIM_OK); //P0 stored in P

  assert(pimOpMove(G, PIM_RREG_SA, PIM_RREG_R2) == PIM_OK); // move P0 to R2

  //prefix G in R1, P in R2
  for (int i = 1; i < 32; i = i*2){
    assert(pimOpNor(G, PIM_RREG_R2, PIM_RREG_R2, PIM_RREG_R2) == PIM_OK); //R2 = ~p
    assert(pimOpNor(G, PIM_RREG_R1, PIM_RREG_R1, PIM_RREG_R3) == PIM_OK); //R3 = ~g
    assert(pimOpMove(G, PIM_RREG_R3, PIM_RREG_SA) == PIM_OK);
    assert(pimOpColGrpShiftR(G, i) == PIM_OK); //SA = ~g << i
    assert(pimOpNor(G, PIM_RREG_SA, PIM_RREG_R2, PIM_RREG_R3) == PIM_OK); //R3 = ~(~g << i) & p
    assert(pimOpNor(G, PIM_RREG_R3, PIM_RREG_R1, PIM_RREG_R1) == PIM_OK); //R1 = g nor ~(~g << i) & p
    assert(pimOpNor(G, PIM_RREG_R1, PIM_RREG_R1, PIM_RREG_R1) == PIM_OK); //R1 = new G
    if (i < 16){
      assert(pimOpMove(G, PIM_RREG_R2, PIM_RREG_SA) == PIM_OK);
      assert(pimOpColGrpShiftR(G, i) == PIM_OK); //SA = ~p << i
      assert(pimOpNor(G, PIM_RREG_SA, PIM_RREG_R2, PIM_RREG_R2) == PIM_OK);
    }
  }

  // final
  //shift G
  assert(pimOpMove(G, PIM_RREG_R1, PIM_RREG_SA) == PIM_OK); 
  assert(pimOpColGrpShiftR(G, 1) == PIM_OK);
  assert(pimOpMove(G, PIM_RREG_SA, PIM_RREG_R1) == PIM_OK); //R1 = ~G << 1

  assert(pimOpReadRowToSa(P, 0) == PIM_OK); //SA = P0
  assert(pimOpNor(G, PIM_RREG_SA, PIM_RREG_SA, PIM_RREG_R2) == PIM_OK); // R2 = ~P0
  assert(pimOpNor(G, PIM_RREG_R1, PIM_RREG_R1, PIM_RREG_R3) == PIM_OK); // R3 = G << 1
  assert(pimOpNor(G, PIM_RREG_R3, PIM_RREG_R2, PIM_RREG_R2) == PIM_OK); // R2 = G << 1 nor ~P0
  assert(pimOpNor(G, PIM_RREG_R1, PIM_RREG_SA, PIM_RREG_SA) == PIM_OK); // SA = P0 nor ~G << 1
  assert(pimOpNor(G, PIM_RREG_R2, PIM_RREG_SA, PIM_RREG_SA) == PIM_OK);

  assert(pimOpWriteSaToRow(G, 0) == PIM_OK); 


  // Copy result back and validate
  assert(pimCopyDeviceToHost(G, (void *)dest.data()) == PIM_OK);

  for (unsigned i = 0; i < numElements; ++i)
  {
    int32_t expected = src1[i] + src2[i];
    if (dest[i] != expected)
    {
      std::cerr << "[FAIL] Index " << i << ": " << src1[i]
                << " + " << src2[i] << " = " << expected
                << ", but got " << std::hex << dest[i] << std::dec << std::endl;
      return false;
    }
  }

  std::cout << "Bit-serial 32-bit adder with correct APP usage OK." << std::endl;

  return true;
}

int main()
{
  std::cout << "PIM test: SIMDRAM micro-ops" << std::endl;

  if (!createPimDevice())
  {
    std::cerr << "[ERROR] PIM device creation failed!" << std::endl;
    return 1;
  }

  if (!testkoggeStoneAdd())
  {
    std::cerr << "[ERROR] Micro-op tests failed!" << std::endl;
    return 1;
  }

  pimShowStats();
  std::cout << "All micro-op tests passed successfully." << std::endl;
  return 0;
}