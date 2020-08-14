#ifndef STAN_MATH_REV_META_ARENA_TYPE_HPP
#define STAN_MATH_REV_META_ARENA_TYPE_HPP

#include <stan/math/prim/meta/is_eigen.hpp>
#include <stan/math/prim/meta/plain_type.hpp>
#include <stan/math/rev/core/arena_allocator.hpp>
#include <stan/math/rev/core/chainable_alloc.hpp>

namespace stan {
namespace math {
// forward declaration
template <typename MatrixType>
class arena_matrix;
}  // namespace math

namespace internal {
// default is used for non-std::vector, non-Eigen, non-trivially destructible
// types
template <typename T, typename = void>
struct arena_type_impl {
  struct type : public stan::math::chainable_alloc, public T{
    template <typename... Args>
    type(Args&&... args) : stan::math::chainable_alloc(), T(std::forward<Args>(args)...) {}
  };
};

template <typename T>
struct arena_type_impl<T, require_t<std::is_base_of<stan::math::chainable_alloc, T>>> {
  using type = T;
};

template <typename T>
struct arena_type_impl<T, require_all_t<std::is_trivially_destructible<T>,
                                        bool_constant<!is_eigen<T>::value>>> {
  using type = T;
};

template <typename T, typename Alloc>
struct arena_type_impl<std::vector<T, Alloc>> {
  using T_ad = typename arena_type_impl<std::decay_t<T>>::type;
  using type = std::vector<T_ad, math::arena_allocator<T_ad>>;
};

template <typename T>
struct arena_type_impl<T, require_eigen_t<T>> {
  using type = math::arena_matrix<plain_type_t<T>>;
};
}  // namespace internal

/**
 * Determines a type that can be used in place of `T` that does any dynamic
 * allocations on the AD stack. This way resulting types are trivially
 * destructible and can be used in vari classes.
 */
template <typename T>
using arena_t = typename internal::arena_type_impl<std::decay_t<T>>::type;

}  // namespace stan

#endif
