#include <algorithm>
#include <exception>
#include <sstream>
#include <streampu.hpp>

#include "Factory/Module/Encoder/Encoder.hpp"
#include "Factory/Module/Puncturer/Puncturer.hpp"
#include "Factory/Tools/Code/Polar_PT/Pretransform_generator.hpp"
#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator.hpp"
#include "Tools/Codec/Polar_PT/Codec_polar_PT.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template<typename B, typename Q>
Codec_polar_PT<B, Q>::Codec_polar_PT(const factory::Pretransform_generator& pt_params,
                                     const factory::Frozenbits_generator& fb_params,
                                     const factory::Encoder_polar_PT& enc_params,
                                     const factory::Decoder_polar_PT& dec_params,
                                     const module::CRC<B>* crc)
  : Codec_SISO<B, Q>(enc_params.K, enc_params.N_cw, enc_params.N_cw)
  , frozen_bits(new std::vector<bool>(pt_params.N_cw, true))
  , dynamic_frozen_bits(new std::vector<bool>(pt_params.N_cw, false))
  , preTransform(new std::map<uint32_t, std::vector<uint32_t>>)
  , pt_encoder(nullptr)
  , pt_decoder(nullptr)
  , fb_encoder(nullptr)
  , fb_decoder(nullptr)
  , dfb_encoder(nullptr)
  , dfb_decoder(nullptr)
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
    // build the frozen bits generator
    pt_generator.reset(pt_params.build());
    fb_generator.reset(fb_params.build());

    // ---------------------------------------------------------------------------------------------------- allocations
    std::fill(frozen_bits->begin(), frozen_bits->begin() + this->K, false);

    fb_generator->generate(*frozen_bits);
    pt_generator->generate(*dynamic_frozen_bits);
    (*preTransform) = pt_generator->get_pre_transform();

    factory::Puncturer pctno_params;
    pctno_params.type = "NO";
    pctno_params.K = enc_params.K;
    pctno_params.N = enc_params.N_cw;
    pctno_params.N_cw = enc_params.N_cw;

    this->set_puncturer(pctno_params.build<B, Q>());
    try
    {
        this->set_encoder(enc_params.build<B>(*frozen_bits, *dynamic_frozen_bits, *preTransform));
        this->pt_encoder = dynamic_cast<Interface_get_set_pretransform*>(&this->get_encoder());
        this->dfb_encoder = dynamic_cast<Interface_get_set_dynamic_frozen_bits*>(&this->get_encoder());
        this->fb_encoder = dynamic_cast<Interface_get_set_frozen_bits*>(&this->get_encoder());
    }
    catch (spu::tools::cannot_allocate const&)
    {
        this->set_encoder(static_cast<const factory::Encoder*>(&enc_params)->build<B>());
    }

    this->set_decoder_siho(
      dec_params.build<B, Q>(*frozen_bits, *dynamic_frozen_bits, *preTransform, crc, &this->get_encoder()));

    try
    {
        this->pt_decoder = dynamic_cast<Interface_get_set_pretransform*>(&this->get_decoder_siho());
        this->dfb_decoder = dynamic_cast<Interface_get_set_dynamic_frozen_bits*>(&this->get_decoder_siho());
        this->fb_decoder = dynamic_cast<Interface_get_set_frozen_bits*>(&this->get_decoder_siho());
    }
    catch (std::exception&)
    {
    }

    // this->set_frozen_bits(*frozen_bits);
    // ------------------------------------------------------------------------------------------------- frozen bit gen
}

template<typename B, typename Q>
const std::map<uint32_t, std::vector<uint32_t>>&
Codec_polar_PT<B, Q>::get_pretransform() const
{
    return *this->preTransform;
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::set_pretransform(const std::map<uint32_t, std::vector<uint32_t>>& pretransform)
{
    if (this->pt_decoder) this->pt_decoder->set_pretransform(pretransform);
    if (this->pt_encoder) this->pt_encoder->set_pretransform(pretransform);
}

template<typename B, typename Q>
const std::vector<bool>&
Codec_polar_PT<B, Q>::get_dynamic_frozen_bits() const
{
    return *this->dynamic_frozen_bits;
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::set_dynamic_frozen_bits(const std::vector<bool>& dfb)
{
    if (this->dfb_decoder) this->dfb_decoder->set_dynamic_frozen_bits(dfb);
    if (this->dfb_encoder) this->dfb_encoder->set_dynamic_frozen_bits(dfb);
}

template<typename B, typename Q>
Codec_polar_PT<B, Q>*
Codec_polar_PT<B, Q>::clone() const
{
    auto t = new Codec_polar_PT(*this);
    t->deep_copy(*this);
    return t;
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::deep_copy(const Codec_polar_PT<B, Q>& t)
{
    Codec_SISO<B, Q>::deep_copy(t);
    if (t.pt_encoder != nullptr) this->pt_encoder = dynamic_cast<Interface_get_set_pretransform*>(&this->get_encoder());
    if (t.pt_decoder != nullptr)
        this->pt_decoder = dynamic_cast<Interface_get_set_pretransform*>(&this->get_decoder_siho());

    if (t.dfb_encoder != nullptr)
        this->dfb_encoder = dynamic_cast<Interface_get_set_dynamic_frozen_bits*>(&this->get_encoder());
    if (t.dfb_decoder != nullptr)
        this->dfb_decoder = dynamic_cast<Interface_get_set_dynamic_frozen_bits*>(&this->get_decoder_siho());

    if (t.fb_encoder != nullptr) this->fb_encoder = dynamic_cast<Interface_get_set_frozen_bits*>(&this->get_encoder());
    if (t.fb_decoder != nullptr)
        this->fb_decoder = dynamic_cast<Interface_get_set_frozen_bits*>(&this->get_decoder_siho());
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::set_frozen_bits(const std::vector<bool>& frozen_bits)
{
    if (this->fb_decoder) this->fb_decoder->set_frozen_bits(frozen_bits);
    if (this->fb_encoder) this->fb_encoder->set_frozen_bits(frozen_bits);
}

template<typename B, typename Q>
const std::vector<bool>&
Codec_polar_PT<B, Q>::get_frozen_bits() const
{
    return *this->frozen_bits;
}

template<typename B, typename Q>
const Frozenbits_generator&
Codec_polar_PT<B, Q>::get_frozen_bits_generator() const
{
    if (this->fb_generator == nullptr)
    {
        std::stringstream message;
        message << "'fb_generator' can't be nullptr.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }

    return *this->fb_generator.get();
}

template<typename B, typename Q>
const Pretransform_generator&
Codec_polar_PT<B, Q>::get_pretransform_generator() const
{
    if (this->pt_generator == nullptr)
    {
        std::stringstream message;
        message << "'pt_generator' can't be nullptr.";
        throw spu::tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
    }

    return *this->pt_generator.get();
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::tools::Codec_polar_PT<B_8, Q_8>;
template class aff3ct::tools::Codec_polar_PT<B_16, Q_16>;
template class aff3ct::tools::Codec_polar_PT<B_32, Q_32>;
template class aff3ct::tools::Codec_polar_PT<B_64, Q_64>;
#else
template class aff3ct::tools::Codec_polar_PT<B, Q>;
#endif
// ==================================================================================== explicit template instantiation
