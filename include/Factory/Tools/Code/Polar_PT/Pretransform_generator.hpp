/*!
 * \file
 * \brief Class factory::Pretransform_generator.
 */
#ifndef FACTORY_PRETRANSFORM_GENERATOR_HPP
#define FACTORY_PRETRANSFORM_GENERATOR_HPP

#include <cli.hpp>
#include <map>
#include <string>

#include "Factory/Factory.hpp"
#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator.hpp"
#include "Tools/Factory/Header.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Pretransform_generator_name;
extern const std::string Pretransform_generator_prefix;
class Pretransform_generator : public Factory
{
  public:
    // ----------------------------------------------------------------------------------------------------- PARAMETERS
    // required parameters
    int K = -1;
    int N_cw = -1;

    // optional parameters
    std::string type = "FILE";
    std::string path_fb = "conf/cde/awgn_polar_codes/PT";
    std::string path_pb = "../lib/polar_bounds/bin/polar_bounds";
    std::string dump_channels_path = "";
    float noise = -1.f;

    // -------------------------------------------------------------------------------------------------------- METHODS
    explicit Pretransform_generator(const std::string& p = Pretransform_generator_prefix);
    virtual ~Pretransform_generator() = default;
    Pretransform_generator* clone() const;

    // parameters construction
    void get_description(cli::Argument_map_info& args) const;
    void store(const cli::Argument_map_value& vals);
    void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    tools::Pretransform_generator* build() const;
};
}
}

#endif /* FACTORY_PRETRANSFORM_GENERATOR_HPP */
