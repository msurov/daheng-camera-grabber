#pragma once
#include <stdexcept>
#include <sstream>

inline void format_args(std::ostream& os) {}

template <class Arg, class ... Args>
inline void format_args(std::ostream& os, Arg const& first, Args const& ... tail)
{
    os << first;
    format_args(os, tail...);
}

template <class ... Args>
[[noreturn]] inline void throw_runtime_error(Args const& ... args)
{
    std::stringstream ss;
    format_args(ss, args...);
    throw std::runtime_error(ss.str());
}
