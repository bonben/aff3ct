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
  : Encoder<B>(K, N)
  , m((int)std::log2(N))
  , X_N_tmp(this->N)
  , frozen_bits(frozen_bits)
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
    this->set_frozen_bits(frozen_bits);
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
    // for (const auto& i : frozen_bits)
    //     std::cout << (int)i << ",";
    // std::cout << std::endl;

    // for (const auto& v : this->pre_transform)
    // {
    //     std::cout << v.first << ":";
    //     for (const auto& uu : v.second)
    //         std::cout << uu << ",";
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;
    this->preTransform(U_K, X_N);

    this->light_encode(X_N);
}

template<typename B>
B
Encoder_polar_PT<B>::preTransform1Bit(B cbit, const int index)
{
    return cbit;
}

template<typename B>
void
Encoder_polar_PT<B>::preTransform(const B* U_K, B* X_N)
{
    // std::cout << "Enter the preTransform fucntion\n";

    // std::cout << "Pre-transformed vector:";
    int k = 0;
    for (int i = 0; i < this->N; i++)
    {
        if (this->frozen_bits[i])
        {
            X_N[i] = (B)0;
            if (this->dynamic_frozen_bits[i])
            {
                for (const auto& indx : this->pre_transform[i])
                {
                    X_N[i] = X_N[i] ^ U_K[indx];
                    // std::cout << "indx: " << indx << "," << std::endl;
                }
            }
        }
        else
        {
            X_N[i] = U_K[k++];
        }
        // X_N[i] = (B)0;
        // if (this->pre_transform.find(i) != this->pre_transform.end())
        // {
        //     if (!this->frozen_bits[i]) X_N[i] = U_K[k++];
        //
        //     // std::cout << "i: " << i << ", k: " << k << ", ";
        //     for (const auto& indx : this->pre_transform[i])
        //     {
        //         X_N[i] = X_N[i] ^ U_K[indx];
        //         // std::cout << "indx: " << indx << "," << std::endl;
        //     }
        // }
        // std::cout << X_N[i] << ",";
    }

    // int k = 0;
    // for (int i = 0; i < this->N; i++)
    // {
    //     X_N[i] = (B)0;
    //     if (this->pre_transform.find(i) != this->pre_transform.end())
    //     {
    //         if (!this->frozen_bits[i]) X_N[i] = U_K[k++];
    //
    //         // std::cout << "i: " << i << ", k: " << k << ", ";
    //         for (const auto& indx : this->pre_transform[i])
    //         {
    //             X_N[i] = X_N[i] ^ U_K[indx];
    //             // std::cout << "indx: " << indx << "," << std::endl;
    //         }
    //     }
    //     // std::cout << X_N[i] << ",";
    // }
    // std::cout << std::endl;

    // std::cout << "After the preTransform fucntion\n";
}

template<typename B>
void
Encoder_polar_PT<B>::light_encode(B* bits)
{
    for (auto k = (this->N >> 1); k > 0; k >>= 1)
        for (auto j = 0; j < this->N; j += 2 * k)
            for (auto i = 0; i < k; i++)
                bits[j + i] = bits[j + i] ^ bits[k + j + i];
}

template<typename B>
void
Encoder_polar_PT<B>::convert(const B* U_K, B* U_N)
{
    if (U_K == U_N)
    {
        std::vector<B> U_K_tmp(this->K);
        std::copy(U_K, U_K + this->K, U_K_tmp.begin());

        auto j = 0;
        for (unsigned i = 0; i < frozen_bits.size(); i++)
            U_N[i] = (frozen_bits[i]) ? (B)0 : U_K_tmp[j++];
    }
    else
    {
        auto j = 0;
        for (unsigned i = 0; i < frozen_bits.size(); i++)
            U_N[i] = (frozen_bits[i]) ? (B)0 : U_K[j++];
    }
    /*for (int i = 0; i < this->N; i++)*/
    /*{*/
    /*    std::cout << ((frozen_bits[i]) ? (B)0 : i) << ",";*/
    /*}*/
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
Encoder_polar_PT<B>::set_frozen_bits(const std::vector<bool>& frozen_bits)
{
    // std::cout << __FILE__ << "," << __func__ << std::endl;
    aff3ct::tools::fb_assert(frozen_bits, this->K, this->N);
    std::copy(frozen_bits.begin(), frozen_bits.end(), this->frozen_bits.begin());
    auto k = 0;
    for (auto n = 0; n < this->N; n++)
        if (!this->frozen_bits[n]) this->info_bits_pos[k++] = n;
}

template<typename B>
void
Encoder_polar_PT<B>::set_pretransform(const std::map<uint32_t, std::vector<uint32_t>>& pre_transform)
{
    // std::copy(this->pre_transform.begin(), this->pre_transform.end(), pre_transform.begin());
}

template<typename B>
const std::vector<bool>&
Encoder_polar_PT<B>::get_frozen_bits() const
{
    return this->frozen_bits;
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
