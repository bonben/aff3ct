/*!
 *  \file
 *  \brief factory::Decoder_polar_PT.
 */

#ifndef FACTORY_DECODER_POLAR_PT_HPP
#define FACTORY_DECODER_POLAR_PT_HPP

#include <cli.hpp>
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
extern const std::string Decoder_polar_PT_name;
extern const std::string Decoder_polar_PT_prefix;
class Decoder_polar_PT : public Decoder
{
  public:
    std::string pt_path = "NO";
    // optional parameters
    std::string simd_strategy = "";
    // std::string polar_nodes = "{R0,R0L,R1,REP,REPL,SPC}";
    std::string polar_nodes = "{R0,R1}";
    bool full_adaptive = true;
    std::string conv = "NO";
    int L = 8;
    int n_ite = 1;

    explicit Decoder_polar_PT(const std::string& p = Decoder_polar_PT_prefix);
    virtual ~Decoder_polar_PT() = default;
    Decoder_polar_PT* clone() const;

    // parameters construction
    virtual void get_description(cli::Argument_map_info& args) const;
    virtual void store(const cli::Argument_map_value& vals);
    virtual void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    template<typename B = int, typename Q = float>
    module::Decoder_SIHO<B, Q>* build(const std::vector<bool>& frozen_bits,
                                      const std::vector<bool>& dynamic_frozen_bits,
                                      const std::map<uint32_t, std::vector<uint32_t>>& pretransform,
                                      const module::CRC<B>* crc = nullptr,
                                      module::Encoder<B>* encoder = nullptr) const;

  protected:
    Decoder_polar_PT(const std::string& n, const std::string& p);
};

}
}

#endif /* FACTORY_DECODER_POLAR_PT_HPP*/
