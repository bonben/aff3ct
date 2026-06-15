#include <algorithm>
#include <exception>
#include <sstream>
#include <streampu.hpp>

#include "Factory/Module/Encoder/Encoder.hpp"
#include "Factory/Module/Puncturer/Puncturer.hpp"
#include "Factory/Tools/Code/Polar_PT/Pretransform_generator.hpp"
#include "Module/Extractor/Polar/Extractor_polar.hpp"
#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator.hpp"
#include "Tools/Codec/Polar/Codec_polar.hpp"
#include "Tools/Codec/Polar_PT/Codec_polar_PT.hpp"
#include "Tools/Noise/Event_probability.hpp"
#include "Tools/Noise/Noise.hpp"
#include "Tools/Noise/Sigma.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template<typename B, typename Q>
Codec_polar_PT<B, Q>::Codec_polar_PT(const factory::Pretransform_generator& pt_params,
                                     const factory::Frozenbits_generator& fb_params,
                                     const factory::Encoder_polar_PT& enc_params,
                                     const factory::Decoder_polar_PT& dec_params,
                                     const module::CRC<B>* crc)
  : Codec_SISO<B, Q>(enc_params.K, enc_params.N_cw, enc_params.N_cw)
  , adaptive_fb(fb_params.noise == -1.0f)
  , frozen_bits(new std::vector<bool>(pt_params.N_cw, true))
  , dynamic_frozen_bits(new std::vector<bool>(pt_params.N_cw, false))
  , preTransform(new std::map<uint32_t, std::vector<uint32_t>>)
  , generated_decoder((dec_params.implem.find("_SNR") != std::string::npos))
  , pt_encoder(nullptr)
  , pt_decoder(nullptr)
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
    if (!generated_decoder)
    {
        // build the frozen bits generator
        pt_generator.reset(pt_params.build());
        fb_generator.reset(fb_params.build());
    }
    else if (this->N_cw != this->N)
    {
        std::stringstream message;
        message << "'N_cw' has to be equal to 'N' ('N_cw' = " << this->N_cw << ", 'N' = " << this->N << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }

    // ---------------------------------------------------------------------------------------------------- allocations
    std::fill(frozen_bits->begin(), frozen_bits->begin() + this->K, false);
    if (!adaptive_fb)
    {

        if (pt_params.type == "BEC")
        {
            Event_probability<> ep(pt_params.noise);
            pt_generator->set_noise(ep);
            pt_generator->generate(*frozen_bits);
        }
        else /* type = GA, TV or FILE */
        {
            Sigma<> sigma(fb_params.noise);
            fb_generator->set_noise(sigma);
            fb_generator->generate(*frozen_bits);
        }

        this->set_frozen_bits(*frozen_bits);
    }

    fb_generator->generate(*frozen_bits);
    pt_generator->generate(*dynamic_frozen_bits);
    (*preTransform) = pt_generator->get_pre_transform();
    // for (const auto v : *preTransform)
    // {
    //     std::cout << v.first << std::endl;
    // }

    factory::Puncturer pctno_params;
    pctno_params.type = "NO";
    pctno_params.K = enc_params.K;
    pctno_params.N = enc_params.N_cw;
    pctno_params.N_cw = enc_params.N_cw;

    this->set_puncturer(pctno_params.build<B, Q>());
    try
    {
        // this->set_encoder(enc_params.build<B>(*frozen_bits));
        // fb_encoder = dynamic_cast<Interface_get_set_frozen_bits*>(&this->get_encoder());
        this->set_encoder(enc_params.build<B>(*frozen_bits, *dynamic_frozen_bits, *preTransform));
        pt_encoder = dynamic_cast<Interface_get_set_pretransform*>(&this->get_encoder());
        dfb_encoder = dynamic_cast<Interface_get_set_dynamic_frozen_bits*>(&this->get_encoder());
    }
    catch (spu::tools::cannot_allocate const&)
    {
        this->set_encoder(static_cast<const factory::Encoder*>(&enc_params)->build<B>());
    }
    /*std::cout << "Calling the decoder constructor" << __FILE__ << std::endl;*/

    try
    {
        /*this->set_decoder_siso(dec_params.build_siso<B, Q>(*frozen_bits, &this->get_encoder()));*/
        this->set_decoder_siho(
          dec_params.build<B, Q>(*frozen_bits, *dynamic_frozen_bits, *preTransform, crc, &this->get_encoder()));
    }
    catch (const std::exception&)
    {
        /*if (generated_decoder)*/
        /*    this->set_decoder_siho(dec_params.build_gen<B, Q>(crc, &this->get_encoder()));*/
        /*else*/
        this->set_decoder_siho(
          dec_params.build<B, Q>(*frozen_bits, *dynamic_frozen_bits, *preTransform, crc, &this->get_encoder()));
    }
    /*std::cout << "Done constructing the decoder" << __FILE__ << std::endl;*/

    try
    {
        this->pt_decoder = dynamic_cast<Interface_get_set_pretransform*>(&this->get_decoder_siho());
        this->dfb_decoder = dynamic_cast<Interface_get_set_dynamic_frozen_bits*>(&this->get_decoder_siho());
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
}

template<typename B, typename Q>
const std::vector<bool>&
Codec_polar_PT<B, Q>::get_dynamic_frozen_bits() const
{
    return *this->frozen_bits;
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
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::set_frozen_bits(const std::vector<bool>& frozen_bits)
{
    // std::cout << "In codec_polar_PT\n";
    // for (const auto& f : frozen_bits)
    // std::cout << (int)f << ",";
    // std::cout << std::endl;
    if (this->fb_decoder) this->fb_decoder->set_frozen_bits(frozen_bits);
    if (this->fb_encoder) this->fb_encoder->set_frozen_bits(frozen_bits);
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::notify_noise_update()
{
    Codec<B, Q>::notify_noise_update();
    if (this->adaptive_fb && !this->generated_decoder)
    {
        this->fb_generator->set_noise(*this->noise);
        this->fb_generator->generate(*this->frozen_bits);
        this->set_frozen_bits(*this->frozen_bits);
    }
}

template<typename B, typename Q>
void
Codec_polar_PT<B, Q>::check_noise()
{
    Codec<B, Q>::check_noise();
    if (!this->noise->is_of_type(Noise_type::SIGMA) && !this->noise->is_of_type(Noise_type::EP))
    {
        std::stringstream message;
        message << "Incompatible noise type, expected noise types are SIGMA or EP ('noise->get_type()' = "
                << Noise<>::type_to_str(this->noise->get_type()) << ").";
        throw spu::tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
    }
}

template<typename B, typename Q>
const std::vector<bool>&
Codec_polar_PT<B, Q>::get_frozen_bits() const
{
    return *this->frozen_bits;
}

template<typename B, typename Q>
bool
Codec_polar_PT<B, Q>::is_adaptive_frozen_bits() const
{
    return this->adaptive_fb;
}

template<typename B, typename Q>
bool
Codec_polar_PT<B, Q>::is_generated_decoder() const
{
    return this->generated_decoder;
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
        message << "'fb_generator' can't be nullptr.";
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
