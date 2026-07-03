#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>
#include <streampu.hpp>

#include "Tools/Code/LDPC/Update_rule/SPA/Update_rule_SPA.hpp"

namespace aff3ct
{
namespace tools
{
template<typename R>
Update_rule_SPA<R>::Update_rule_SPA(const unsigned max_chk_node_degree)
  : name("SPA")
  , values(max_chk_node_degree)
  , sign(0)
  , product(1)
  , n_zeros(0)
  , n_ite(0)
  , ite(0)
{
    if (max_chk_node_degree == 0)
    {
        std::stringstream message;
        message << "'max_chk_node_degree' has to greater than 0.";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    if (typeid(R) != typeid(double) && typeid(R) != typeid(float))
    {
        std::stringstream message;
        message << "The 'LSPA' update rule supports only 'float' or 'double' datatypes.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }
}

template<typename R>
inline std::string
Update_rule_SPA<R>::get_name() const
{
    return this->name;
}

template<typename R>
inline void
Update_rule_SPA<R>::begin_decoding(const int n_ite)
{
    this->n_ite = n_ite;
}

template<typename R>
inline void
Update_rule_SPA<R>::begin_ite(const int ite)
{
    this->ite = ite;
}

template<typename R>
inline void
Update_rule_SPA<R>::begin_chk_node_in(const int /*chk_id*/, const int chk_degree)
{
    assert(chk_degree <= (int)this->values.size());

    this->sign = 0;
    this->product = 1;
    this->n_zeros = 0;
}

template<typename R>
inline void
Update_rule_SPA<R>::compute_chk_node_in(const int var_id, const R var_val)
{
    const auto var_abs = (R)std::abs(var_val);
    const auto res = (R)std::tanh(var_abs * (R)0.5);
    const auto var_sign = std::signbit((float)var_val) ? -1 : 0;
    const bool is_zero_val = res == (R)0;

    this->sign ^= is_zero_val ? 0 : var_sign;
    this->product *= is_zero_val ? (R)1 : res;
    this->n_zeros = (res == (R)0) ? this->n_zeros + 1 : this->n_zeros;
    this->values[var_id] = res;
}

template<typename R>
inline void
Update_rule_SPA<R>::end_chk_node_in()
{
}

template<typename R>
inline void
Update_rule_SPA<R>::begin_chk_node_out(const int /*chk_id*/, const int /*chk_degree*/)
{
}

template<typename R>
inline R
Update_rule_SPA<R>::compute_chk_node_out(const int var_id, const R var_val)
{
    const bool is_zero_val = this->values[var_id] == (R)0;

    auto res_tmp = is_zero_val ? this->product : this->product / this->values[var_id];
    res_tmp = (res_tmp < (R)1.0) ? res_tmp : (R)1.0 - std::numeric_limits<R>::epsilon();
    const auto res_abs = (R)2.0 * std::atanh(res_tmp);
    const auto res_sng = is_zero_val ? this->sign : this->sign ^ (std::signbit((float)var_val) ? -1 : 0);

    return (this->n_zeros == 0 || (this->n_zeros == 1 && this->values[var_id] == (R)0))
             ? (R)std::copysign(res_abs, res_sng)
             : 0;
}

template<typename R>
inline void
Update_rule_SPA<R>::end_chk_node_out()
{
}

template<typename R>
inline void
Update_rule_SPA<R>::end_ite()
{
}

template<typename R>
inline void
Update_rule_SPA<R>::end_decoding()
{
}
}
}
