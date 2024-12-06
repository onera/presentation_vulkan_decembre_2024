#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#if defined(_OPENMP)
#include <omp.h>
#endif
#include "ProdMatMat.hpp"

namespace {
void prodSubBlocks(int iRowBlkA, int iColBlkB, int iColBlkA, int szBlock,
                   const Matrix& A, const Matrix& B, Matrix& C) 
{
  for (int j = iColBlkB; j < std::min(B.nbCols, iColBlkB + szBlock); j++)
    for (int k = iColBlkA; k < std::min(A.nbCols, iColBlkA + szBlock); k++)
      for (int i = iRowBlkA; i < std::min(A.nbRows, iRowBlkA + szBlock); ++i)
        C(i, j) += A(i, k) * B(k, j);
}
const int szBlock = 256;
}  // namespace

Matrix operator*(const Matrix& A, const Matrix& B) {
  Matrix C(A.nbRows, B.nbCols, 0.0);
# pragma omp parallel for collapse(2)
  for (int jBlock = 0; jBlock < B.nbCols; jBlock += szBlock)
    for (int iBlock = 0; iBlock < A.nbRows; iBlock += szBlock)
      for (int kBlock = 0; kBlock < A.nbCols; kBlock += szBlock)
        prodSubBlocks(iBlock, jBlock, kBlock, szBlock, A, B, C);
  return C;
}
