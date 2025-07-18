// File: pimCore.cpp
// PIMeval Simulator - PIM Core
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimCore.h"
#include <random>
#include <cstdio>
#include <iomanip>
#include <sstream>


//! @brief  pimCore ctor
pimCore::pimCore(unsigned numRows, unsigned numCols)
  : m_numRows(numRows),
    m_numCols(numCols),
    m_array(numRows, std::vector<bool>(numCols)),
    m_senseAmpCol(numRows)
{
  // Initialize memory contents with random 0/1
  if (0) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 1);
    for (unsigned row = 0; row < m_numRows; ++row) {
      for (unsigned col = 0; col < m_numCols; ++col) {
        m_array[row][col] = dist(gen);
      }
    }
  }

  for (int reg = PIM_RREG_SA; reg < PIM_RREG_MAX; ++reg) {
    declareRowReg(static_cast<PimRowReg>(reg));
  }
}

//! @brief  pimCore dtor
pimCore::~pimCore()
{
}

//! @brief  Initialize a row reg
bool
pimCore::declareRowReg(PimRowReg reg)
{
  m_rowRegs[reg].resize(m_numCols);
  return true;
}

//! @brief  Read a memory row to SA
bool
pimCore::readRow(unsigned rowIndex)
{
  if (rowIndex >= m_numRows) {
    std::printf("PIM-Error: Out-of-boundary subarray row read: index = %u, numRows = %u\n", rowIndex, m_numRows);
    m_memoryAccessLog.push_back("Failed readRow: rowIndex = " + std::to_string(rowIndex) + " (out of bounds)");
    return false;
  }
  m_memoryAccessLog.push_back("readRow: rowIndex = " + std::to_string(rowIndex));
  m_rowRegs[PIM_RREG_SA] = m_array[rowIndex];
  return true;
}

//! @brief  Read a memory column
bool
pimCore::readCol(unsigned colIndex)
{
  if (colIndex >= m_numCols) {
    std::printf("PIM-Error: Out-of-boundary subarray column read: index = %u, numCols = %u\n", colIndex, m_numCols);
    m_memoryAccessLog.push_back("Failed readCol: colIndex = " + std::to_string(colIndex) + " (out of bounds)");
    return false;
  }
  m_memoryAccessLog.push_back("readCol: colIndex = " + std::to_string(colIndex));
  for (unsigned row = 0; row < m_numRows; ++row) {
    m_senseAmpCol[row] = m_array[row][colIndex];
  }
  return true;
}

//! @brief  Read multiple rows to SA. Original contents of these rows are replaced with majority values.
//!         Input parameters: A list of (row-index, is-dual-contact-negated)
bool
pimCore::readMultiRows(const std::vector<std::pair<unsigned, bool>>& rowIdxs)
{
  std::string logEntry = "readMultiRows: indices = ";
  for (const auto& kv : rowIdxs) {
    logEntry += "(" + std::to_string(kv.first) + ", dualContact=" + (kv.second ? "true" : "false") + ") ";
  }
  // sanity check
  if (rowIdxs.size() % 2 == 0) {
    std::printf("PIM-Error: Behavior of simultaneously reading even number of rows is undefined\n");
    m_memoryAccessLog.push_back(logEntry + " - Failed (even number of rows)");
    return false;
  }
  for (const auto& kv : rowIdxs) {
    if (kv.first >= m_numRows) {
      std::printf("PIM-Error: Out-of-boundary subarray multi-row read: idx = %u, numRows = %u\n", kv.first, m_numRows);
      m_memoryAccessLog.push_back(logEntry + " - Failed (index " + std::to_string(kv.first) + " out of bounds)");
      return false;
    }
  }
  m_memoryAccessLog.push_back(logEntry);

  // compute majority
  for (unsigned col = 0; col < m_numCols; ++col) {
    unsigned sum = 0;
    for (const auto& kv : rowIdxs) {
      unsigned idx = kv.first;
      bool isDCCN = kv.second;
      bool val = (isDCCN ? !m_array[idx][col] : m_array[idx][col]);
      sum += val ? 1 : 0;
    }
    bool maj = (sum > rowIdxs.size() / 2);
    for (const auto& kv : rowIdxs) {
      unsigned idx = kv.first;
      bool isDCCN = kv.second;
      m_array[idx][col] = (isDCCN ? !maj : maj);
    }
    m_rowRegs[PIM_RREG_SA][col] = maj;
  }
  return true;
}

//! @brief  Write multiple rows. All rows are written with same values from SA.
//!         Input parameters: A list of (row-index, is-dual-contact-negated)
bool
pimCore::writeMultiRows(const std::vector<std::pair<unsigned, bool>>& rowIdxs)
{
  std::string logEntry = "writeMultiRows: indices = ";
  for (const auto& kv : rowIdxs) {
    logEntry += "(" + std::to_string(kv.first) + ", dualContact=" + (kv.second ? "true" : "false") + ") ";
  }
  // sanity check
  for (const auto& kv : rowIdxs) {
    if (kv.first >= m_numRows) {
      std::printf("PIM-Error: Out-of-boundary subarray multi-row read: idx = %u, numRows = %u\n", kv.first, m_numRows);
      m_memoryAccessLog.push_back(logEntry + " - Failed (index " + std::to_string(kv.first) + " out of bounds)");
      return false;
    }
  }
  m_memoryAccessLog.push_back(logEntry);
  // write
  for (unsigned col = 0; col < m_numCols; ++col) {
    bool val = m_rowRegs[PIM_RREG_SA][col];
    for (const auto& kv : rowIdxs) {
      unsigned idx = kv.first;
      bool isDCCN = kv.second;
      m_array[idx][col] = (isDCCN ? !val :val);
    }
  }
  return true;
}

//! @brief  Write to a memory row
bool
pimCore::writeRow(unsigned rowIndex)
{
  if (rowIndex >= m_numRows) {
    std::printf("PIM-Error: Out-of-boundary subarray row write: index = %u, numRows = %u\n", rowIndex, m_numRows);
    m_memoryAccessLog.push_back("Failed writeRow: rowIndex = " + std::to_string(rowIndex) + " (out of bounds)");
    return false;
  }
  m_memoryAccessLog.push_back("writeRow: rowIndex = " + std::to_string(rowIndex));
  m_array[rowIndex] = m_rowRegs[PIM_RREG_SA];
  return true;
}

//! @brief  Write to a memory column
bool
pimCore::writeCol(unsigned colIndex)
{
  if (colIndex >= m_numCols) {
    std::printf("PIM-Error: Out-of-boundary subarray column write: index = %u, numCols = %u\n", colIndex, m_numCols);
    m_memoryAccessLog.push_back("Failed writeCol: colIndex = " + std::to_string(colIndex) + " (out of bounds)");
    return false;
  }
  m_memoryAccessLog.push_back("writeCol: colIndex = " + std::to_string(colIndex));
  for (unsigned row = 0; row < m_numRows; ++row) {
    m_array[row][colIndex] = m_senseAmpCol[row];
  }
  return true;
}

//! @brief  Set values to row sense amplifiers
bool
pimCore::setSenseAmpRow(const std::vector<bool>& vals)
{
  if (vals.size() != m_numCols) {
    std::printf("PIM-Error: Incorrect data size write to row SAs: size = %lu, numCols = %u\n", vals.size(), m_numCols);
    return false;
  }
  m_rowRegs[PIM_RREG_SA] = vals;
  return true;
}

//! @brief  Set values to row sense amplifiers
bool
pimCore::setSenseAmpCol(const std::vector<bool>& vals)
{
  if (vals.size() != m_numRows) {
    std::printf("PIM-Error: Incorrect data size write to col SAs: size = %lu, numRows = %u\n", vals.size(), m_numRows);
    return false;
  }
  m_senseAmpCol = vals;
  return true;
}

//! @brief  Print out memory subarray contents
void
pimCore::print() const
{
  std::ostringstream oss;
  // header
  oss << "  Row S ";
  for (unsigned col = 0; col < m_array[0].size(); ++col) {
    oss << (col % 8 == 0 ? '+' : '-');
  }
  oss << std::endl;
  for (unsigned row = 0; row < m_array.size(); ++row) {
    // row index
    oss << std::setw(5) << row << ' ';
    // col SA
    oss << m_senseAmpCol[row] << ' ';
    // row contents
    for (unsigned col = 0; col < m_array[0].size(); ++col) {
      oss << m_array[row][col];
    }
    oss << std::endl;
  }
  // footer
  oss << "        ";
  for (unsigned col = 0; col < m_array[0].size(); ++col) {
    oss << (col % 8 == 0 ? '+' : '-');
  }
  oss << std::endl;
  // row SA
  oss << "     SA ";
  for (unsigned col = 0; col < m_array[0].size(); ++col) {
    oss << m_rowRegs.at(PIM_RREG_SA)[col];
  }
  oss << std::endl;
  std::printf("%s\n", oss.str().c_str());
}

void pimCore::printMemoryAccess() const
{
    std::printf("\nRecorded Memory Accesses:\n");
    for (const auto &entry : m_memoryAccessLog)
    {
      // std::printf("memory access log");
      std::printf("%s\n", entry.c_str());
    }
    std::printf("\n");
}

