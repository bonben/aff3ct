/*
 * \file
 * \brief Class factory::Encoder_polar_PT
 */

#ifndef FACTORY_ENCODER_POLAR_PT
#define FACTORY_ENCODER_POLAR_PT

#include <cli.hpp>
#include <map>
#include <string>
#include <vector>

#include "Factory/Module/Encoder/Encoder.hpp"
#include "Module/Encoder/Polar_PT/Encoder_polar_PT.hpp"
#include "Tools/Factory/Header.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Encoder_polar_PT_name;
extern const std::string Encoder_polar_PT_prefix;
class Encoder_polar_PT : public Encoder
{
  public:
    std::string pt_path = "NO";
    std::string conv = "NO";
    explicit Encoder_polar_PT(const std::string& p = Encoder_polar_PT_prefix);
    virtual ~Encoder_polar_PT() = default;
    Encoder_polar_PT* clone() const;

    // parameters construction
    void get_description(cli::Argument_map_info& args) const;
    void store(const cli::Argument_map_value& vals);
    void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    template<typename B = int>
    module::Encoder_polar_PT<B>* build(const std::vector<bool>& frozen_bits,
                                       const std::vector<bool>& dynamic_frozen_bits,
                                       const std::map<uint32_t, std::vector<uint32_t>>& pretransform) const;
};
}
}

#endif // FACTORY_ENCODER_POLAR_PT
