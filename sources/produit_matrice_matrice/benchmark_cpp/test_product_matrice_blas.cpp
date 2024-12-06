#include <cstdlib>
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include <chrono>
#include "Matrix.hpp"
#include "ProdMatMat.hpp"

extern "C" void sgemm_(char const& trA, char const& trB, int const& m, int const& n, int const& k,
                       float const& alpha, float const* A, int const& ldA, float const* B,
                       int const& ldB, float const& beta, float* C, int const& ldC );

std::tuple<std::vector<float>,std::vector<float>,
	   std::vector<float>,std::vector<float>>  computeTensors(int dim)
{
  float pi = std::acos(-1.0);
  auto u1 = std::vector < float >(dim);
  auto u2 = std::vector < float >(dim);
  auto v1 = std::vector < float >(dim);
  auto v2 = std::vector < float >(dim);

  for (int i = 0; i < dim; i++)
    {
      u1[i] = std::cos(1.67 * i * pi / dim);
      u2[i] = std::sin(2.03 * i * pi / dim + 0.25);
      v1[i] = std::cos(1.23 * i * i * pi / (7.5 * dim));
      v2[i] = std::sin(0.675 * i / (3.1 * dim));
    }
  return std::make_tuple(u1, u2, v1, v2);
}

Matrix initTensorMatrices(const std::vector < float >&u, const std::vector < float >&v)
{
  Matrix A(u.size(), v.size());
  for (unsigned long irow = 0UL; irow < u.size(); ++irow)
    for (unsigned long jcol = 0UL; jcol < v.size(); ++jcol)
      A(irow, jcol) = u[irow] * v[jcol];
  return A;
}

float dot(const std::vector < float >&u, const std::vector < float >&v)
{
  assert(u.size() == v.size());
  float scal = 0.0;
  for (unsigned long i = 0UL; i < u.size(); ++i)
    scal += u[i] * v[i];
  return scal;
}

bool verifProduct(const std::vector < float >&uA, std::vector < float >&vA,
		  const std::vector < float >&uB, std::vector < float >&vB, const Matrix & C)
{
  float vAdotuB = dot(vA, uB);
  for (int irow = 0; irow < C.nbRows; irow++)
    for (int jcol = 0; jcol < C.nbCols; jcol++)
      {
	float rightVal = uA[irow] * vAdotuB * vB[jcol];
	if (std::fabs(rightVal - C(irow, jcol)) >
	    100*std::fabs(C(irow, jcol) * std::numeric_limits < float >::epsilon()))
	  {
	    std::
	      cerr << "Erreur numérique : valeur attendue pour C( " << irow << ", " << jcol
		   << " ) -> " << rightVal << " mais valeur trouvée : " << C(irow,jcol) << std::endl;
	    return false;
	  }
      }
  return true;
}

int main(int nargs, char *vargs[])
{
  int dim = 1024;
  if (nargs > 1)
    dim = atoi(vargs[1]);
  std::vector < float >uA, vA, uB, vB;
  std::tie(uA, vA, uB, vB) = computeTensors(dim);

  Matrix A = initTensorMatrices(uA, vA);
  Matrix B = initTensorMatrices(uB, vB);

  std::chrono::time_point < std::chrono::system_clock > start, end;
  start = std::chrono::system_clock::now();
  Matrix C(dim,dim);
  sgemm_('N', 'N', dim, dim, dim, 1., A.data(), dim, B.data(), dim, 0., C.data(), dim);
  end = std::chrono::system_clock::now();
  std::chrono::duration < float >elapsed_seconds = end - start;

  bool isPassed = verifProduct(uA, vA, uB, vB, C);
  if (isPassed)
    {
      std::cout << "Test passed\n";
      std::cout << "Temps CPU produit matrice-matrice blas : " << elapsed_seconds.count() << " secondes\n";
      std::cout << "GFlops -> " << (double(dim)*dim*dim)/elapsed_seconds.count()/1024./1024./1024. <<std::endl;
    }
  else
    std::cout << "Test failed\n";

  return (isPassed ? EXIT_SUCCESS : EXIT_FAILURE);
}
