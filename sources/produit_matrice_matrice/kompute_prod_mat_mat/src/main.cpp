#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <string>
using namespace std::string_literals;
#include "kompute/Kompute.hpp"

#include "shader/mulmatmat_h.hpp"

extern "C" void 
sgemm_(const char& trA, const char& trB, int const& M, int const& N, int const& K, float const& alpha, 
       float const *A, int const& ldA, float const *B, int const& ldB, float const& beta,
       float *C, int const& ldC);

std::pair<std::vector<float>,std::vector<float>>
get_tensor_matrix( std::uint32_t n, float frequency, float phase = 0.f )
{
    constexpr const float two_pi = 6.283185307179586;
    std::vector<float> U,Vt;
    U.reserve(n); Vt.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        U.push_back(std::cos(two_pi*i/frequency+phase));
        Vt.push_back(std::sin(two_pi*i/frequency+phase));
    }
    return {U, Vt};
}
// --------------------------------------------------------------------------------------
std::vector<float> 
compute_mat_from_tensor( std::vector<float> const& u, std::vector<float> const& vt )
{
    std::vector<float> mat(u.size()*vt.size());
    for (std::uint32_t i = 0; i < u.size(); ++i )
    {
        for (std::uint32_t j = 0; j < vt.size(); ++j )
        {
            mat[i*vt.size()+j] = u[i]*vt[j];
        }
    }
    return mat;
}
//
float compute_error( std::pair<std::vector<float>, std::vector<float>> const& A, std::pair<std::vector<float>, std::vector<float>> const& B,
                     std::vector<float> const& C )
{
    auto const& A_u = A.first;
    auto const& A_vt= A.second;
    auto const& B_u = B.first;
    auto const& B_vt= B.second;

    float scal = 0.f;
    for (std::size_t i = 0; i < A_vt.size(); ++i ) scal += A_vt[i]*B_u[i];
    float err = 0.f, frob_C = 0.f;
    for (std::size_t i = 0; i < A_u.size(); ++i)
      for (std::size_t j = 0; j < B_vt.size(); ++j)
      {
        float c_attended = scal * A_u[i] * B_vt[j];
        frob_C += c_attended*c_attended;
        float delta = C[i*B_vt.size()+j] - c_attended;
        if (delta*delta > 1.E-6) { std::cout << "C(" << i << "," << j << ") = " << C[i*B_vt.size()+j]  << "= ? " << c_attended << std::endl;exit(EXIT_FAILURE);}
        err += delta*delta;
      }
    return std::sqrt(err/frob_C);
}
//
const std::uint32_t workgroup_size = 16;
int main(int nargs, char *vargs[])
{
    kp::Manager mgr;
    std::uint32_t dim = 1024;
    if (nargs > 1)
        dim = std::stoul(vargs[1]);

    auto [A_u,A_vt] = get_tensor_matrix(dim, dim+1.f, 0.5f);
    auto [B_u,B_vt] = get_tensor_matrix(dim, 341.f, 0.25f);

    auto A = compute_mat_from_tensor( A_u, A_vt);
    auto B = compute_mat_from_tensor( B_u, B_vt);
    std::vector<float> C(A.size(), 0.f);

    std::cout << "Calcul blas (openblas) :" << std::endl;
    std::cout << "------------------------" << std::endl;
    std::vector<float>(dim*dim, 0.f).swap(C);
    char tr='T';
    int  dimb = int(A_u.size());
    auto beg_computation2 = std::chrono::high_resolution_clock::now();
    sgemm_(tr, tr, dimb, dimb, dimb, 1.0f, A.data(), dimb, B.data(), dimb, 0.f, C.data(), dimb );
    auto end_computation2 = std::chrono::high_resolution_clock::now();
    auto duree2 = std::chrono::duration<double>(end_computation2 - beg_computation2).count();
    std::cout << "Temps calcul blas (en secondes) : " << duree2 << std::endl;
    std::cout << "Performance : " << (double(dim)*dim*dim)/duree2/1024./1024./1024. << " Giga flops" << std::endl;

    // On recalcule la transposée de C pour la vérification (merci le Fortran !)
    std::vector<float> Ct(dim*dim);
    for (std::size_t i = 0; i < dim; ++i)
        for (std::size_t j = 0; j < dim; ++j)
            Ct[i*dim+j] = C[j*dim+i];

    std::cout << "Erreur L2 relatif sur le résultat trouvé en blas : ";
    auto error2 = compute_error({A_u,A_vt}, {B_u,B_vt}, Ct);
    std::cout << error2 << std::endl;


    std::cout << "Calcul Vulkan" << std::endl;
    std::cout << "-------------" << std::endl;
    std::vector<float>(dim*dim, 0.f).swap(C);
    std::shared_ptr<kp::TensorT<float>> mat_A = mgr.tensor(A);
    std::shared_ptr<kp::TensorT<float>> mat_B = mgr.tensor(B);
    std::shared_ptr<kp::TensorT<float>> mat_C = mgr.tensor(C);
    
    const std::vector<std::shared_ptr<kp::Memory>> params = { mat_A, mat_B, mat_C };


    std::vector<std::uint32_t> shader(shader::MULMATMAT_COMP_SPV.begin(), shader::MULMATMAT_COMP_SPV.end());
    std::shared_ptr<kp::Algorithm> algo = 
        //mgr.algorithm(params, shader, kp::Workgroup({ (dim+workgroup_size-1)/workgroup_size, (dim+workgroup_size-1)/workgroup_size}), {}, 
        mgr.algorithm(params, shader, kp::Workgroup({ (dim+127)/128, (dim+127)/128}), {}, 
                      std::vector<std::uint32_t>{dim});

    std::shared_ptr<kp::Sequence> sq = mgr.sequence()
        ->record<kp::OpSyncDevice>(params)
        ->record<kp::OpAlgoDispatch>(algo)
        ->record<kp::OpSyncLocal>(params);

    auto beg_computation = std::chrono::high_resolution_clock::now();
    sq->eval();
    auto end_computation = std::chrono::high_resolution_clock::now();
    double duree = std::chrono::duration<double>(end_computation - beg_computation).count();
    std::cout << "Temps calcul matrice matrice (en secondes): " << duree <<  std::endl;
    std::cout << "Performance qui fait flops : " << (double(dim)*dim*dim)/duree/1024./1024./1024. << "Gigo flops (d'après retour vers le futur)" << std::endl;

    std::cout << "Erreur L2 relatif sur le résultat trouvé en Vulkan: ";
    auto C_out = mat_C->vector();
    auto error = compute_error({A_u,A_vt}, {B_u,B_vt}, C_out);
    std::cout << error << std::endl;


    return EXIT_SUCCESS;
}