#pragma once

#include <cstdlib>
#include <functional>

namespace ss {

////////////////
// function traits
////////////////

template <size_t N, typename T, typename... Ts>
struct decayed_arg_n {
    static_assert(N - 1 != sizeof...(Ts), "index out of range");
    using type = typename decayed_arg_n<N - 1, Ts...>::type;
};

template <typename T, typename... Ts>
struct decayed_arg_n<0, T, Ts...> {
    using type = std::decay_t<T>;
};

template <typename T>
struct function_traits;

template <typename R, typename C, typename Arg>
struct function_traits<std::function<R(C&, const Arg&) const>> {
    using arg_type = Arg;
};

template <typename R, typename... Ts>
struct function_traits<R(Ts...)> {
    using arg0 = typename decayed_arg_n<0, Ts...>::type;
};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) const> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...)&> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) const&> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) &&> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) const&&> : function_traits<R(Ts...)> {};

template <typename MemberFunction>
struct member_wrapper;

template <typename R, typename T>
struct member_wrapper<R T::*> {
    using arg_type = typename function_traits<R>::arg0;
};

////////////////
// has method
////////////////

#define INIT_HAS_METHOD(method)                                                \
    template <typename T>                                                      \
    class has_m_##method {                                                     \
        template <typename C>                                                  \
        static std::true_type test(decltype(&C::method));                      \
                                                                               \
        template <typename C>                                                  \
        static std::false_type test(...);                                      \
                                                                               \
    public:                                                                    \
        constexpr static bool value = decltype(test<T>(0))::value;             \
    };                                                                         \
                                                                               \
    template <typename T>                                                      \
    constexpr bool has_m_##method##_t = has_m_##method<T>::value;

} /* namespace ss */
