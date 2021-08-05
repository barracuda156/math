#ifndef STAN_MATH_PRIM_ERR_CHECK_CORR_MATRIX_HPP
#define STAN_MATH_PRIM_ERR_CHECK_CORR_MATRIX_HPP

#include <stan/math/prim/fun/Eigen.hpp>
#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/to_ref.hpp>
#include <stan/math/prim/fun/value_of_rec.hpp>
#include <stan/math/prim/err/throw_domain_error.hpp>
#include <stan/math/prim/err/check_pos_definite.hpp>
#include <stan/math/prim/err/check_square.hpp>
#include <sstream>
#include <string>
#include <cmath>

namespace stan {
namespace math {

/**
 * Check if the specified matrix is a valid correlation matrix.
 * A valid correlation matrix is symmetric, has a unit diagonal
 * (all 1 values), and has all values between -1 and 1
 * (inclusive).
 * This function throws exceptions if the variable is not a valid
 * correlation matrix.
 * @tparam Mat Type inheriting from `MatrixBase` with dynamic rows and
 * columns.
 * @param function Name of the function this was called from
 * @param name Name of the variable
 * @param y Matrix to test
 * @throw <code>std::invalid_argument</code> if the matrix is not square
 * @throw <code>std::domain_error</code> if the matrix is non-symmetric,
 *   diagonals not near 1, not positive definite, or any of the
 *   elements nan
 */
template <typename Mat, require_matrix_t<Mat>* = nullptr>
inline void check_corr_matrix(const char* function, const char* name,
                              const Mat& y) {
  auto&& y_ref = to_ref(value_of_rec(y));
  check_square(function, name, y_ref);
  using std::fabs;
  if (y_ref.size() == 0) {
    return;
  }

  for (Eigen::Index k = 0; k < y.rows(); ++k) {
    if (!(fabs(y_ref.coeff(k, k) - 1.0) <= CONSTRAINT_TOLERANCE)) {
      [&y_ref, name, k, function]() STAN_COLD_PATH {
        std::ostringstream msg;
        msg << "is not a valid correlation matrix. " << name << "("
            << stan::error_index::value + k << "," << stan::error_index::value + k
            << ") is ";
        std::string msg_str(msg.str());
        throw_domain_error(function, name, y_ref(k, k), msg_str.c_str(),
                           ", but should be near 1.0");
      }();
    }
  }
  check_pos_definite(function, name, y_ref);
}

template <typename StdVec, require_std_vector_t<StdVec>* = nullptr>
void check_corr_matrix(const char* function, const char* name,
                                const StdVec& y) {
  for (auto&& y_i : y) {
    check_corr_matrix(function, name, y_i);
  }
}

}  // namespace math
}  // namespace stan
#endif
