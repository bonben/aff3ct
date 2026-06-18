/*!
 * \file
 * \brief Class tools::Codec_conv.
 */
#ifndef CODEC_CONV_HPP_
#define CODEC_CONV_HPP_

#include <memory>
#include <vector>

#include "Factory/Module/Decoder/Conv/Decoder_conv.hpp"
#include "Factory/Module/Encoder/Conv/Encoder_conv.hpp"
#include "Module/CRC/CRC.hpp"
#include "Tools/Codec/Codec_SIHO.hpp"

namespace aff3ct
{
namespace tools
{
template<typename B = int, typename Q = float>
class Codec_conv : public Codec_SIHO<B, Q>
{
  protected:
    std::shared_ptr<std::vector<std::vector<int>>> trellis;

  public:
    Codec_conv(const factory::Encoder_conv& enc_params,
               const factory::Decoder_conv& dec_params,
               const module::CRC<B>* crc = nullptr);
    virtual ~Codec_conv() = default;

    virtual Codec_conv<B, Q>* clone() const;

    const std::vector<std::vector<int>>& get_trellis() const;
};
}
}

#endif /* CODEC_CONV_HPP_ */
