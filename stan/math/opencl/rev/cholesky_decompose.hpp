#ifndef STAN_MATH_OPENCL_REV_CHOLESKY_DECOMPOSE_HPP
#define STAN_MATH_OPENCL_REV_CHOLESKY_DECOMPOSE_HPP
#ifdef STAN_OPENCL

#include <stan/math/opencl/prim/cholesky_decompose.hpp>
#include <stan/math/opencl/err/check_nan.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/rev/fun/value_of.hpp>
#include <stan/math/rev/functor/reverse_pass_callback.hpp>

namespace stan {
namespace math {
/**
 * Returns the sum of the coefficients of the specified
 * matrix on the OpenCL device.
 *
 * @param x Specified var_value containing a matrix.
 * @return Sum of coefficients of matrix.
 */
inline var_value<matrix_cl<double>> cholesky_decompose(
    const var_value<matrix_cl<double>>& A) {
  check_nan("cholesky_decompose", "A", A.val());
  var_value<matrix_cl<double>> L_A = cholesky_decompose(A.val());

  reverse_pass_callback([A, L_A]() mutable {
    int M_ = A.rows();
    int block_size
        = M_ / opencl_context.tuning_opts().cholesky_rev_block_partition;
    block_size = std::max(block_size, 8);
    block_size = std::min(
        block_size, opencl_context.tuning_opts().cholesky_rev_min_block_size);
    matrix_cl<double> A_adj = L_A.adj();
    for (int k = M_; k > 0; k -= block_size) {
      const int j = std::max(0, k - block_size);
      const int k_j_ind = k - j;
      const int m_k_ind = M_ - k;

      auto&& R_val = block(L_A.val(), j, 0, k_j_ind, j);
      auto&& R_adj = block(A_adj, j, 0, k_j_ind, j);
      matrix_cl<double> D_val = block(L_A.val(), j, j, k_j_ind, k_j_ind);
      matrix_cl<double> D_adj = block(A_adj, j, j, k_j_ind, k_j_ind);
      auto&& B_val = block(L_A.val(), k, 0, m_k_ind, j);
      auto&& B_adj = block(A_adj, k, 0, m_k_ind, j);
      auto&& C_val = block(L_A.val(), k, j, m_k_ind, k_j_ind);
      auto&& C_adj = block(A_adj, k, j, m_k_ind, k_j_ind);

      C_adj = C_adj * tri_inverse(D_val);
      B_adj = B_adj - C_adj * R_val;
      D_adj = D_adj - transpose(C_adj) * C_val;

      D_adj = transpose(D_val) * D_adj;
      D_adj.triangular_transpose<TriangularMapCL::LowerToUpper>();
      D_val = transpose(tri_inverse(D_val));
      D_adj = D_val * transpose(D_val * D_adj);
      D_adj.triangular_transpose<TriangularMapCL::LowerToUpper>();

      R_adj = R_adj - transpose(C_adj) * B_val - D_adj * R_val;
      D_adj = diagonal_multiply(D_adj, 0.5);

      block(A_adj, j, j, k_j_ind, k_j_ind) = D_adj;
    }
    A_adj.view(matrix_cl_view::Lower);
    A.adj() = A.adj() + A_adj;
  });

  return L_A;
}

}  // namespace math
}  // namespace stan

#endif
#endif
