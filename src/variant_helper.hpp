//
// Created by Terry on 5/16/2026.
//

#ifndef LIBGNSS_VARIANT_HELPER_HPP
#define LIBGNSS_VARIANT_HELPER_HPP

#include <tuple>
#include <variant>

namespace libgnss::utils
{

template <typename T>
using Callback = std::function<void(const T&)>;

// primary template
template <typename T>
struct variant_to_tuple;

// partial specialization for std::variant
template <typename... Ts>
struct variant_to_tuple<std::variant<Ts...>> {
  using optionals = std::tuple<std::optional<Ts>...>;
  using callbacks = std::tuple<Callback<Ts>...>;
};

// helper aliases
template <typename T>
using variant_to_tuple_opts = typename variant_to_tuple<T>::optionals;

template <typename T>
using variant_to_tuple_cbs = typename variant_to_tuple<T>::callbacks;

}  // namespace libgnss

#endif  // LIBGNSS_VARIANT_HELPER_HPP
