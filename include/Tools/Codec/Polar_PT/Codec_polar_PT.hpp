/*!
 * \file
 * \brief Class tools::Codec_polar_PT.
 */
#ifndef CODEC_POLAR_PT_HPP_
#define CODEC_POLAR_PT_HPP_

#include <memory>
#include <vector>

#include "Factory/Module/Decoder/Polar_PT/Decoder_polar_PT.hpp"
#include "Factory/Module/Encoder/Polar_PT/Encoder_polar_PT.hpp"
#include "Factory/Tools/Code/Polar/Frozenbits_generator.hpp"
#include "Factory/Tools/Code/Polar_PT/Pretransform_generator.hpp"
#include "Module/CRC/CRC.hpp"
#include "Tools/Code/Polar/Frozenbits_generator/Frozenbits_generator.hpp"
#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator.hpp"
#include "Tools/Codec/Codec_SISO.hpp"
#include "Tools/Interface/Interface_get_set_dynamic_frozen_bits.hpp"
#include "Tools/Interface/Interface_get_set_frozen_bits.hpp"
#include "Tools/Interface/Interface_get_set_pretransform.hpp"

namespace aff3ct
{
namespace tools
{
template<typename B = int, typename Q = float>
class Codec_polar_PT
  : public Codec_SISO<B, Q>
  , public Interface_get_set_pretransform
  , public Interface_get_set_frozen_bits
  , public Interface_get_set_dynamic_frozen_bits
{
  protected:
    const bool adaptive_fb;
    std::shared_ptr<std::vector<bool>> frozen_bits; // known bits (alias frozen bits) are set to true
    std::shared_ptr<std::vector<bool>> dynamic_frozen_bits;
    std::shared_ptr<std::map<uint32_t, std::vector<uint32_t>>> preTransform;
    const bool generated_decoder;

    std::shared_ptr<Frozenbits_generator> fb_generator;
    std::shared_ptr<Pretransform_generator> pt_generator;

    Interface_get_set_frozen_bits* fb_decoder;
    Interface_get_set_frozen_bits* fb_encoder;

    Interface_get_set_pretransform* pt_decoder;
    Interface_get_set_pretransform* pt_encoder;

    Interface_get_set_dynamic_frozen_bits* dfb_encoder;
    Interface_get_set_dynamic_frozen_bits* dfb_decoder;

  public:
    Codec_polar_PT(const factory::Pretransform_generator& pt_params,
                   const factory::Frozenbits_generator& fb_params,
                   const factory::Encoder_polar_PT& enc_par,
                   const factory::Decoder_polar_PT& dec_par,
                   const module::CRC<B>* crc = nullptr);
    virtual ~Codec_polar_PT() = default;

    virtual Codec_polar_PT<B, Q>* clone() const;

    const std::vector<bool>& get_frozen_bits() const;
    const std::map<uint32_t, std::vector<uint32_t>>& get_pretransform() const;
    const std::vector<bool>& get_dynamic_frozen_bits() const;

    bool is_adaptive_frozen_bits() const;
    bool is_generated_decoder() const;
    const Frozenbits_generator& get_frozen_bits_generator() const;
    const Pretransform_generator& get_pretransform_generator() const;

    void set_frozen_bits(const std::vector<bool>& frozen_bits);
    void set_pretransform(const std::map<uint32_t, std::vector<uint32_t>>& pretransform);
    void set_dynamic_frozen_bits(const std::vector<bool>& dfb);
    virtual void notify_noise_update();

  protected:
    virtual void deep_copy(const Codec_polar_PT<B, Q>& t);
    void check_noise();
};
}
}

#endif /* CODEC_POLAR_PT_HPP_ */
