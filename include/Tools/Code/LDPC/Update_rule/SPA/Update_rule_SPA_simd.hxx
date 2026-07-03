#include <cassert>
#include <limits>
#include <sstream>
#include <streampu.hpp>

#include "Tools/Code/LDPC/Update_rule/SPA/Update_rule_SPA_simd.hpp"

namespace aff3ct
{
namespace tools
{
template<typename R>
Update_rule_SPA_simd<R>::Update_rule_SPA_simd(const unsigned max_chk_node_degree)
  : name("SPA")
  , false_msk(false)
  , zero((R)0)
  , one((R)1)
  , half((R)0.5)
  , two((R)2)
  , epsilon1m((R)1.0 - std::numeric_limits<R>::epsilon())
  , values(max_chk_node_degree)
  , sign(false)
  , product(one)
  , one_zero(false)
  , two_or_more_zeros(false)
  , no_zero(true)
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
        message << "The 'SPA' update rule supports only 'float' or 'double' datatypes.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }
}

template<typename R>
inline std::string
Update_rule_SPA_simd<R>::get_name() const
{
    return this->name;
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::begin_decoding(const int n_ite)
{
    this->n_ite = n_ite;
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::begin_ite(const int ite)
{
    this->ite = ite;
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::begin_chk_node_in(const int chk_id, const int chk_degree)
{
    assert(chk_degree <= (int)this->values.size());

    this->sign = this->false_msk;
    this->product = this->one;

    this->one_zero = this->false_msk;
    this->two_or_more_zeros = this->false_msk;
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::compute_chk_node_in(const int var_id, const mipp::Reg<R> var_val)
{
    const auto var_abs = mipp::abs(var_val);
    const auto res = mipp::tanh(var_abs * this->half);
    const auto var_sign = mipp::sign(var_val);
    auto is_zero_val = res == this->zero;

    this->sign = ((this->sign ^ var_sign) & (~is_zero_val)) | (this->sign & is_zero_val);
    this->product = mipp::blend(this->product, this->product * res, is_zero_val);
    this->two_or_more_zeros = this->two_or_more_zeros | (this->one_zero & is_zero_val);
    this->one_zero = ~this->two_or_more_zeros & (this->one_zero | is_zero_val);
    this->values[var_id] = res;
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::end_chk_node_in()
{
    this->no_zero = ~(this->one_zero | this->two_or_more_zeros);
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::begin_chk_node_out(const int chk_id, const int chk_degree)
{
}

template<typename R>
inline mipp::Reg<R>
Update_rule_SPA_simd<R>::compute_chk_node_out(const int var_id, const mipp::Reg<R> var_val)
{
    auto is_zero_val = this->values[var_id] == this->zero;

    auto res_tmp = mipp::blend(this->product, this->product / this->values[var_id], is_zero_val);
    res_tmp = mipp::blend(res_tmp, this->epsilon1m, res_tmp < this->one);
    const auto res_abs = this->two * mipp::atanh(res_tmp);
    const auto res_sng = ((this->sign ^ mipp::sign(var_val)) & (~is_zero_val)) | (this->sign & is_zero_val);

    return mipp::blend(mipp::copysign(res_abs, res_sng), zero, (this->no_zero | (this->one_zero & is_zero_val)));
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::end_chk_node_out()
{
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::end_ite()
{
}

template<typename R>
inline void
Update_rule_SPA_simd<R>::end_decoding()
{
}
}
}
