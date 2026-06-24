#include <memory>
#include <sstream>
#include <streampu.hpp>

#include "Factory/Module/Encoder/Encoder.hpp"
#include "Factory/Module/Puncturer/Puncturer.hpp"
#include "Tools/Codec/Conv/Codec_conv.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template<typename B, typename Q>
Codec_conv<B, Q>::Codec_conv(const factory::Encoder_conv& enc_params,
                             const factory::Decoder_conv& dec_params,
                             const module::CRC<B>* crc)
  : Codec_SIHO<B, Q>(enc_params.K, enc_params.N_cw, enc_params.N_cw)
  , trellis(new std::vector<std::vector<int>>())
{
    // ----------------------------------------------------------------------------------------------------- exceptions
    if (enc_params.K != dec_params.K)
    {
        std::stringstream message;
        message << "'enc_params.K' has to be equal to 'dec_params.K' ('enc_params.K' = " << enc_params.K
                << ", 'dec_params.K' = " << dec_params.K << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    if (enc_params.N_cw != dec_params.N_cw)
    {
        std::stringstream message;
        message << "'enc_params.N_cw' has to be equal to 'dec_params.N_cw' ('enc_params.N_cw' = " << enc_params.N_cw
                << ", 'dec_params.N_cw' = " << dec_params.N_cw << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    // ---------------------------------------------------------------------------------------------------------- tools
    auto enc_cpy = enc_params;
    enc_cpy.type = "CONV";

    std::unique_ptr<module::Encoder_conv<B>> encoder_conv(enc_cpy.build<B>());
    *trellis = encoder_conv->get_trellis();

    // ---------------------------------------------------------------------------------------------------- allocations
    factory::Puncturer pct_params;
    pct_params.type = "NO";
    pct_params.K = enc_params.K;
    pct_params.N = enc_params.N_cw;
    pct_params.N_cw = enc_params.N_cw;

    this->set_puncturer(pct_params.build<B, Q>());
    try
    {
        this->set_encoder(enc_params.build<B>());
    }
    catch (spu::tools::cannot_allocate const&)
    {
        this->set_encoder(static_cast<const factory::Encoder*>(&enc_params)->build<B>());
    }

    this->set_decoder_siho(dec_params.build<B, Q>(*trellis, crc, &this->get_encoder()));
}

template<typename B, typename Q>
Codec_conv<B, Q>*
Codec_conv<B, Q>::clone() const
{
    auto t = new Codec_conv(*this);
    t->deep_copy(*this);
    return t;
}

template<typename B, typename Q>
const std::vector<std::vector<int>>&
Codec_conv<B, Q>::get_trellis() const
{
    return *this->trellis;
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Codec_conv<B_8, Q_8>;
template class aff3ct::tools::Codec_conv<B_16, Q_16>;
template class aff3ct::tools::Codec_conv<B_32, Q_32>;
template class aff3ct::tools::Codec_conv<B_64, Q_64>;
#else
template class aff3ct::tools::Codec_conv<B, Q>;
#endif
// ==================================================================================== explicit template instantiation
