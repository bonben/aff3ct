/*!
 * \file
 * \brief Class factory::Codec_conv.
 */
#ifndef FACTORY_CODEC_CONV_HPP
#define FACTORY_CODEC_CONV_HPP

#include <cli.hpp>
#include <map>
#include <string>

#include "Factory/Tools/Codec/Codec_SIHO.hpp"
#include "Module/CRC/CRC.hpp"
#include "Tools/Codec/Conv/Codec_conv.hpp"
#include "Tools/Factory/Header.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Codec_conv_name;
extern const std::string Codec_conv_prefix;
class Codec_conv : public Codec_SIHO
{
  public:
    explicit Codec_conv(const std::string& p = Codec_conv_prefix);
    virtual ~Codec_conv() = default;
    Codec_conv* clone() const;

    // parameters construction
    void get_description(cli::Argument_map_info& args) const;
    void store(const cli::Argument_map_value& vals);
    void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    template<typename B = int, typename Q = float>
    tools::Codec_conv<B, Q>* build(const module::CRC<B>* crc = nullptr) const;
};
}
}

#endif /* FACTORY_CODEC_CONV_HPP */
