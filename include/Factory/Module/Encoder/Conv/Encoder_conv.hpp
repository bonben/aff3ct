/*!
 * \file
 * \brief Class factory::Encoder_conv.
 */
#ifndef FACTORY_ENCODER_CONV_HPP
#define FACTORY_ENCODER_CONV_HPP

#include <cli.hpp>
#include <map>
#include <string>
#include <vector>

#include "Factory/Module/Encoder/Encoder.hpp"
#include "Module/Encoder/Conv/Encoder_conv.hpp"
#include "Tools/Factory/Header.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Encoder_conv_name;
extern const std::string Encoder_conv_prefix;
class Encoder_conv : public Encoder
{
  public:
    // ----------------------------------------------------------------------------------------------------- PARAMETERS
    // optional
    std::vector<int> poly = { 0171, 0133 };

    // -------------------------------------------------------------------------------------------------------- METHODS
    explicit Encoder_conv(const std::string& p = Encoder_conv_prefix);
    virtual ~Encoder_conv() = default;
    Encoder_conv* clone() const;

    // parameters construction
    void get_description(cli::Argument_map_info& args) const;
    void store(const cli::Argument_map_value& vals);
    void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    template<typename B = int>
    module::Encoder_conv<B>* build() const;
};
}
}

#endif /* FACTORY_ENCODER_CONV_HPP */
