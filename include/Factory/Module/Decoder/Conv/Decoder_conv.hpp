/*!
 * \file
 * \brief Class factory::Decoder_conv.
 */
#ifndef FACTORY_DECODER_CONV_HPP
#define FACTORY_DECODER_CONV_HPP

#include <cli.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Factory/Module/Decoder/Decoder.hpp"
#include "Module/CRC/CRC.hpp"
#include "Module/Decoder/Decoder_SIHO.hpp"
#include "Module/Encoder/Encoder.hpp"
#include "Tools/Factory/Header.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Decoder_conv_name;
extern const std::string Decoder_conv_prefix;
class Decoder_conv : public Decoder
{
  public:
    // ----------------------------------------------------------------------------------------------------- PARAMETERS
    // optional parameters
    std::vector<int> poly = { 0171, 0133 };
    unsigned int L = 8;

    // -------------------------------------------------------------------------------------------------------- METHODS
    explicit Decoder_conv(const std::string& p = Decoder_conv_prefix);
    virtual ~Decoder_conv() = default;
    Decoder_conv* clone() const;

    // parameters construction
    void get_description(cli::Argument_map_info& args) const;
    void store(const cli::Argument_map_value& vals);
    void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    template<typename B = int, typename Q = float>
    module::Decoder_SIHO<B, Q>* build(const std::vector<std::vector<int>>& trellis,
                                      const module::CRC<B>* crc = nullptr,
                                      module::Encoder<B>* encoder = nullptr) const;

    template<typename B = int, typename Q = float>
    module::Decoder_SIHO<B, Q>* build_viterbi(const std::vector<std::vector<int>>& trellis) const;

    template<typename B = int, typename Q = float>
    module::Decoder_SIHO<B, Q>* build_viterbi_list(const std::vector<std::vector<int>>& trellis,
                                                   const module::CRC<B>* crc) const;
};
}
}

#endif /* FACTORY_DECODER_CONV_HPP */
