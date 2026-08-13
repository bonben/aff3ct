#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <streampu.hpp>
#include <string>

#include "Module/Encoder/Polar_PT/Encoder_polar_PT.hpp"
#include "Tools/Code/Polar/fb_assert.h"
#include "Tools/Exception/invalid_argument/invalid_argument.hpp"

#include "Tools/general_utils.h"
using namespace aff3ct::module;

template<typename B>
Encoder_polar_PT<B>::Encoder_polar_PT(const int& K,
                                      const int& N,
                                      const std::vector<bool>& frozen_bits,
                                      const std::vector<bool>& dynamic_frozen_bits,
                                      const std::map<uint32_t, std::vector<uint32_t>>& pre_transform)
  : Encoder_polar<B>(K, N, frozen_bits)
  , dynamic_frozen_bits(dynamic_frozen_bits)
  , pre_transform(pre_transform)
{
    const std::string name = "Encoder_polar_PT";
    this->set_name(name);
    for (auto& t : this->tasks)
        t->set_replicability(true);
    this->set_sys(false);

    if (this->N != (int)frozen_bits.size())
    {
        std::stringstream message;
        message << "'frozen_bits.size()' has to be equal to 'N' "
                   "('frozen_bits.size()' = "
                << frozen_bits.size() << ", 'N' = " << N << ").";
        throw spu::tools::length_error(__FILE__, __LINE__, __func__, message.str());
    }

    info_bits_loc.resize(this->K, 0);
}

template<typename B>
Encoder_polar_PT<B>*
Encoder_polar_PT<B>::clone() const
{
    auto m = new Encoder_polar_PT(*this);
    m->deep_copy(*this);
    return m;
}

template<typename B>
void
Encoder_polar_PT<B>::_encode(const B* U_K, B* X_N, const size_t /*frame_id*/)
{
    this->preTransform(U_K, X_N);

    this->light_encode(X_N);
}

template<typename B>
void
Encoder_polar_PT<B>::preTransform(const B* U_K, B* X_N)
{

    int k = 0;
    for (int i = 0; i < this->N; i++)
    {
        if (this->frozen_bits[i])
        {
            X_N[i] = (B)0;
            if (this->dynamic_frozen_bits[i])
            {
                auto it = this->pre_transform.find((uint32_t)i);

                if (it == this->pre_transform.end())
                {
                    std::stringstream message;
                    message << "Missing pre-transform for dynamically frozen bit ('i' = " << i << ").";
                    throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
                }
                for (const auto& indx : it->second)
                {
                    X_N[i] = X_N[i] ^ U_K[indx];
                }
            }
        }
        else
        {
            X_N[i] = U_K[k++];
        }
    }
}

template<typename B>
bool
Encoder_polar_PT<B>::is_codeword(const B* X_N)
{
    std::copy(X_N, X_N + this->N, this->X_N_tmp.data());

    for (auto k = (this->N >> 1); k > 0; k >>= 1)
        for (auto j = 0; j < this->N; j += 2 * k)
        {
            for (auto i = 0; i < k; i++)
                this->X_N_tmp[j + i] = this->X_N_tmp[j + i] ^ this->X_N_tmp[k + j + i];

            if (this->frozen_bits[j + k - 1] && this->X_N_tmp[j + k - 1]) return false;
        }

    return true;
}

template<typename B>
void
Encoder_polar_PT<B>::set_dynamic_frozen_bits(const std::vector<bool>& dfb)
{

    std::copy(dfb.begin(), dfb.end(), this->dynamic_frozen_bits.begin());
}

template<typename B>
void
Encoder_polar_PT<B>::set_pretransform(const std::map<uint32_t, std::vector<uint32_t>>& pre_transform)
{
    // std::copy(this->pre_transform.begin(), this->pre_transform.end(), pre_transform.begin());
}

template<typename B>
const std::map<uint32_t, std::vector<uint32_t>>&
Encoder_polar_PT<B>::get_pretransform() const
{
    return this->pre_transform;
}

template<typename B>
const std::vector<bool>&
Encoder_polar_PT<B>::get_dynamic_frozen_bits() const
{
    return this->dynamic_frozen_bits;
}

// ====================================================================================
// explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Encoder_polar_PT<B_8>;
template class aff3ct::module::Encoder_polar_PT<B_16>;
template class aff3ct::module::Encoder_polar_PT<B_32>;
template class aff3ct::module::Encoder_polar_PT<B_64>;
#else
template class aff3ct::module::Encoder_polar_PT<B>;
#endif
// ====================================================================================
// explicit template instantiation
