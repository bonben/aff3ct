#include <streampu.hpp>

#include "Factory/Module/Encoder/Encoder.hpp"
#include "Factory/Module/Encoder/Polar_PT/Encoder_polar_PT.hpp"
#include "Module/Encoder/Polar_PT/Encoder_polar_PT.hpp"
#include "Tools/Documentation/documentation.h"
#include "Tools/Exception/cannot_allocate/cannot_allocate.hpp"
#include "Tools/Factory/Header.hpp"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Encoder_polar_PT_name = "Encoder Polar PT";
const std::string aff3ct::factory::Encoder_polar_PT_prefix = "enc";

Encoder_polar_PT::Encoder_polar_PT(const std::string& prefix)
  : Encoder(Encoder_polar_PT_name, prefix)
{
    this->type = "POLAR_PT";
}

Encoder_polar_PT*
Encoder_polar_PT::clone() const
{
    return new Encoder_polar_PT(*this);
}

void
Encoder_polar_PT::get_description(cli::Argument_map_info& args) const
{
    Encoder::get_description(args);

    auto p = this->get_prefix();
    const std::string class_name = "factory::Encoder_polar_PT::";

    cli::add_options(args.at({ p + "-type" }), 0, "POLAR_PT");

    // tools::add_arg(args, p, class_name + "p+pt", cli::Text());
}

void
Encoder_polar_PT::store(const cli::Argument_map_value& vals)
{
    Encoder::store(vals);

    auto p = this->get_prefix();
    // std::cout << "prefix: " << p << __FILE__ << std::endl;

    // if (vals.exist({ p + "-pt" })) this->pt_path = vals.at({ p + "-pt" });
    this->systematic = false;
}

void
Encoder_polar_PT::get_headers(std::map<std::string, tools::header_list>& headers, const bool full) const
{
    Encoder::get_headers(headers, full);
    auto p = this->get_prefix();

    // headers[p].push_back(std::make_pair("Pre-Transform", this->pt_path));
}

template<typename B>
module::Encoder_polar_PT<B>*
Encoder_polar_PT::build(const std::vector<bool>& frozen_bits,
                        const std::vector<bool>& dynamic_frozen_bits,
                        const std::map<uint32_t, std::vector<uint32_t>>& pretransform) const
{
    if (this->type == "POLAR_PT")
        return new module::Encoder_polar_PT<B>(this->K, this->N_cw, frozen_bits, dynamic_frozen_bits, pretransform);
    throw spu::tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template aff3ct::module::Encoder_polar_PT<B_8>*
aff3ct::factory::Encoder_polar_PT::build<B_8>(const std::vector<bool>&,
                                              const std::vector<bool>&,
                                              const std::map<uint32_t, std::vector<uint32_t>>&) const;
template aff3ct::module::Encoder_polar_PT<B_16>*
aff3ct::factory::Encoder_polar_PT::build<B_16>(const std::vector<bool>&,
                                               const std::vector<bool>&,
                                               const std::map<uint32_t, std::vector<uint32_t>>&) const;
template aff3ct::module::Encoder_polar_PT<B_32>*
aff3ct::factory::Encoder_polar_PT::build<B_32>(const std::vector<bool>&,
                                               const std::vector<bool>&,
                                               const std::map<uint32_t, std::vector<uint32_t>>&) const;
template aff3ct::module::Encoder_polar_PT<B_64>*
aff3ct::factory::Encoder_polar_PT::build<B_64>(const std::vector<bool>&,
                                               const std::vector<bool>&,
                                               const std::map<uint32_t, std::vector<uint32_t>>&) const;
#else
template aff3ct::module::Encoder_polar_PT<B>*
aff3ct::factory::Encoder_polar_PT::build<B>(const std::vector<bool>&,
                                            const std::vector<bool>&,
                                            const std::map<uint32_t, std::vector<uint32_t>>&) const;
#endif
// ==================================================================================== explicit template instantiation
