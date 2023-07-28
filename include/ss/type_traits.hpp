#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <cstdlib>
#include <tuple>

namespace ss {

////////////////
// tup merge/cat
////////////////

template <typename T, typename Ts>
struct tup_cat;

template <typename... Ts, typename... Us>
struct tup_cat<std::tuple<Ts...>, std::tuple<Us...>> {
    using type = std::tuple<Ts..., Us...>;
};

template <typename T, typename... Ts>
struct tup_cat<T, std::tuple<Ts...>> {
    using type = std::tuple<T, Ts...>;
};

template <typename... Ts>
using tup_cat_t = typename tup_cat<Ts...>::type;

////////////////
// tup first/head
////////////////

template <size_t N, typename T, typename... Ts>
struct left_of_impl;

template <size_t N, typename T, typename... Ts>
struct left_of_impl {
    static_assert(N < 128, "recursion limit reached");
    static_assert(N != 0, "cannot take the whole tuple");
    using type = tup_cat_t<T, typename left_of_impl<N - 1, Ts...>::type>;
};

template <typename T, typename... Ts>
struct left_of_impl<0, T, Ts...> {
    using type = std::tuple<T>;
};

template <size_t N, typename... Ts>
using left_of_t = typename left_of_impl<N, Ts...>::type;

template <typename... Ts>
using first_t = typename left_of_impl<sizeof...(Ts) - 2, Ts...>::type;

template <typename... Ts>
using head_t = typename left_of_impl<0, Ts...>::type;

////////////////
// tup tail/last
////////////////

template <size_t N, typename T, typename... Ts>
struct right_of_impl;

template <size_t N, typename T, typename... Ts>
struct right_of_impl {
    using type = typename right_of_impl<N - 1, Ts...>::type;
};

template <typename T, typename... Ts>
struct right_of_impl<0, T, Ts...> {
    using type = std::tuple<T, Ts...>;
};

template <size_t N, typename... Ts>
using right_of_t = typename right_of_impl<N, Ts...>::type;

template <typename... Ts>
using tail_t = typename right_of_impl<1, Ts...>::type;

template <typename... Ts>
using last_t = typename right_of_impl<sizeof...(Ts) - 1, Ts...>::type;

////////////////
// apply trait
////////////////

template <template <typename...> class Trait, typename T>
struct apply_trait;

template <template <typename...> class Trait, typename T, typename... Ts>
struct apply_trait<Trait, std::tuple<T, Ts...>> {
    using type =
        tup_cat_t<typename Trait<T>::type,
                  typename apply_trait<Trait, std::tuple<Ts...>>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_trait {
    using type = std::tuple<typename Trait<T>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_trait<Trait, std::tuple<T>> {
    using type = std::tuple<typename Trait<T>::type>;
};

template <template <typename...> class Trait, typename... Ts>
using apply_trait_t = typename apply_trait<Trait, Ts...>::type;

////////////////
// apply optional trait
////////////////

// type is T if true, and std::false_type othervise
template <typename T, typename U>
struct optional_trait;

template <typename U>
struct optional_trait<std::true_type, U> {
    using type = U;
};

template <typename U>
struct optional_trait<std::false_type, U> {
    using type = std::false_type;
};

template <template <typename...> class Trait, typename T>
struct apply_optional_trait;

template <template <typename...> class Trait, typename T, typename... Ts>
struct apply_optional_trait<Trait, std::tuple<T, Ts...>> {
    using type = tup_cat_t<
        typename optional_trait<typename Trait<T>::type, T>::type,
        typename apply_optional_trait<Trait, std::tuple<Ts...>>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_optional_trait {
    using type =
        std::tuple<typename optional_trait<typename Trait<T>::type, T>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_optional_trait<Trait, std::tuple<T>> {
    using type =
        std::tuple<typename optional_trait<typename Trait<T>::type, T>::type>;
};

template <template <typename...> class Trait, typename... Ts>
using apply_trait_optional_t = apply_optional_trait<Trait, Ts...>;

////////////////
// filter false_type
////////////////

template <typename T, typename... Ts>
struct remove_false {
    using type = tup_cat_t<T, typename remove_false<Ts...>::type>;
};

template <typename... Ts>
struct remove_false<std::false_type, Ts...> {
    using type = typename remove_false<Ts...>::type;
};

template <typename T, typename... Ts>
struct remove_false<std::tuple<T, Ts...>> {
    using type = tup_cat_t<T, typename remove_false<Ts...>::type>;
};

template <typename... Ts>
struct remove_false<std::tuple<std::false_type, Ts...>> {
    using type = typename remove_false<Ts...>::type;
};

template <typename T>
struct remove_false<T> {
    using type = std::tuple<T>;
};

template <typename T>
struct remove_false<std::tuple<T>> {
    using type = std::tuple<T>;
};

template <>
struct remove_false<std::false_type> {
    using type = std::tuple<>;
};

////////////////
// negate trait
////////////////

template <template <typename...> class Trait>
struct negate_impl {
    template <typename... Ts>
    using type = std::integral_constant<bool, !Trait<Ts...>::value>;
};

template <template <typename...> class Trait>
using negate_impl_t = typename negate_impl<Trait>::type;

////////////////
// filter by trait
////////////////

template <template <typename...> class Trait, typename... Ts>
struct filter_if {
    using type = typename filter_if<Trait, std::tuple<Ts...>>::type;
};

template <template <typename...> class Trait, typename... Ts>
struct filter_if<Trait, std::tuple<Ts...>> {

    using type = typename remove_false<
        typename apply_optional_trait<Trait, std::tuple<Ts...>>::type>::type;
};

template <template <typename...> class Trait, typename... Ts>
using filter_if_t = typename filter_if<Trait, Ts...>::type;

template <template <typename...> class Trait, typename... Ts>
struct filter_not {
    using type = typename filter_not<Trait, std::tuple<Ts...>>::type;
};

template <template <typename...> class Trait, typename... Ts>
struct filter_not<Trait, std::tuple<Ts...>> {

    using type = typename remove_false<typename apply_optional_trait<
        negate_impl<Trait>::template type, std::tuple<Ts...>>::type>::type;
};

template <template <typename...> class Trait, typename... Ts>
using filter_not_t = typename filter_not<Trait, Ts...>::type;

////////////////
// count
////////////////

template <template <typename...> class Trait, typename... Ts>
struct count;

template <template <typename...> class Trait, typename T, typename... Ts>
struct count<Trait, T, Ts...> {
    static constexpr size_t value =
        std::tuple_size<filter_if_t<Trait, T, Ts...>>::value;
};

template <template <typename...> class Trait, typename T>
struct count<Trait, T> {
    static constexpr size_t value = Trait<T>::value;
};

template <template <typename...> class Trait>
struct count<Trait> {
    static constexpr size_t value = 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr size_t count_v = count<Trait, Ts...>::value;

////////////////
// count not
////////////////

template <template <typename...> class Trait, typename... Ts>
struct count_not;

template <template <typename...> class Trait, typename T, typename... Ts>
struct count_not<Trait, T, Ts...> {
    static constexpr size_t value =
        std::tuple_size<filter_not_t<Trait, T, Ts...>>::value;
};

template <template <typename...> class Trait, typename T>
struct count_not<Trait, T> {
    static constexpr size_t value = !Trait<T>::value;
};

template <template <typename...> class Trait>
struct count_not<Trait> {
    static constexpr size_t value = 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr size_t count_not_v = count_not<Trait, Ts...>::value;

////////////////
// all of
////////////////

template <template <typename...> class Trait, typename... Ts>
struct all_of {
    static constexpr bool value = count_v<Trait, Ts...> == sizeof...(Ts);
};

template <template <typename...> class Trait, typename... Ts>
struct all_of<Trait, std::tuple<Ts...>> {
    static constexpr bool value = count_v<Trait, Ts...> == sizeof...(Ts);
};

template <template <typename...> class Trait, typename... Ts>
constexpr bool all_of_v = all_of<Trait, Ts...>::value;

////////////////
// any of
////////////////

template <template <typename...> class Trait, typename... Ts>
struct any_of {
    static_assert(sizeof...(Ts) > 0);
    static constexpr bool value = count_v<Trait, Ts...> > 0;
};

template <template <typename...> class Trait, typename... Ts>
struct any_of<Trait, std::tuple<Ts...>> {
    static_assert(sizeof...(Ts) > 0);
    static constexpr bool value = count_v<Trait, Ts...> > 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr bool any_of_v = any_of<Trait, Ts...>::value;

////////////////
// none  of
////////////////

template <template <typename...> class Trait, typename... Ts>
struct none_of {
    static constexpr bool value = count_v<Trait, Ts...> == 0;
};

template <template <typename...> class Trait, typename... Ts>
struct none_of<Trait, std::tuple<Ts...>> {
    static constexpr bool value = count_v<Trait, Ts...> == 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr bool none_of_v = none_of<Trait, Ts...>::value;

////////////////
// is instance of
////////////////

template <template <typename...> class Template, typename T>
struct is_instance_of {
    constexpr static bool value = false;
};

template <template <typename...> class Template, typename... Ts>
struct is_instance_of<Template, Template<Ts...>> {
    constexpr static bool value = true;
};

template <template <typename...> class Template, typename... Ts>
constexpr bool is_instance_of_v = is_instance_of<Template, Ts...>::value;

////////////////
// tuple to struct
////////////////

template <class T, std::size_t... Is, class U>
T to_object_impl(std::index_sequence<Is...>, U&& data) {
    return {std::get<Is>(std::forward<U>(data))...};
}

template <class T, class U>
T to_object(U&& data) {
    using NoRefU = std::decay_t<U>;
    if constexpr (is_instance_of_v<std::tuple, NoRefU>) {
        return to_object_impl<
            T>(std::make_index_sequence<std::tuple_size<NoRefU>{}>{},
               std::forward<U>(data));
    } else {
        return T{std::forward<U>(data)};
    }
}

} /* trait */
