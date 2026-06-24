#include <cmath>
#include <cstdio>
#include <ios>
#include <sstream>
#include <streampu.hpp>
#include <utility>

#include "Factory/Module/Encoder/Conv/Encoder_conv.hpp"
#include "Tools/Documentation/documentation.h"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Encoder_conv_name = "Encoder CONV";
const std::string aff3ct::factory::Encoder_conv_prefix = "enc";

Encoder_conv ::Encoder_conv(const std::string& prefix)
  : Encoder(Encoder_conv_name, prefix)
{
    this->type = "CONV";
    this->systematic = false;
}

Encoder_conv*
Encoder_conv ::clone() const
{
    return new Encoder_conv(*this);
}

void
Encoder_conv ::get_description(cli::Argument_map_info& args) const
{
    Encoder::get_description(args);

    auto p = this->get_prefix();
    const std::string class_name = "factory::Encoder_conv::";

    args.erase({ p + "-cw-size", "N" });

    cli::add_options(args.at({ p + "-type" }), 0, "CONV");

    tools::add_arg(args, p, class_name + "p+poly", cli::Text());
}

void
Encoder_conv ::store(const cli::Argument_map_value& vals)
{
    Encoder::store(vals);

    auto p = this->get_prefix();

    if (vals.exist({ p + "-poly" }))
    {
        auto poly_str = vals.at({ p + "-poly" });

#ifdef _MSC_VER
        sscanf_s(poly_str.c_str(), "{%o,%o}", &this->poly[0], &this->poly[1]);
#else
        std::sscanf(poly_str.c_str(), "{%o,%o}", &this->poly[0], &this->poly[1]);
#endif
    }

    const int n_ff = (int)std::floor(std::log2((float)std::max(this->poly[0], this->poly[1])));
    this->tail_length = 2 * n_ff;
    this->N_cw = 2 * this->K + this->tail_length;
    this->R = (float)this->K / (float)this->N_cw;
}

void
Encoder_conv ::get_headers(std::map<std::string, tools::header_list>& headers, const bool full) const
{
    Encoder::get_headers(headers, full);

    auto p = this->get_prefix();

    if (this->tail_length) headers[p].push_back(std::make_pair("Tail length", std::to_string(this->tail_length)));

    std::stringstream poly;
    poly << "{0" << std::oct << this->poly[0] << ",0" << std::oct << this->poly[1] << "}";
    headers[p].push_back(std::make_pair(std::string("Polynomials"), poly.str()));
}

template<typename B>
module::Encoder_conv<B>*
Encoder_conv ::build() const
{
    if (this->type == "CONV") return new module::Encoder_conv<B>(this->K, this->N_cw, this->poly);

    throw spu::tools::cannot_allocate(__FILE__, __LINE__, __func__);
}

// ==================================================================================== explicit template instantiation
#include "Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template aff3ct::module::Encoder_conv<B_8>*
aff3ct::factory::Encoder_conv::build<B_8>() const;
template aff3ct::module::Encoder_conv<B_16>*
aff3ct::factory::Encoder_conv::build<B_16>() const;
template aff3ct::module::Encoder_conv<B_32>*
aff3ct::factory::Encoder_conv::build<B_32>() const;
template aff3ct::module::Encoder_conv<B_64>*
aff3ct::factory::Encoder_conv::build<B_64>() const;
#else
template aff3ct::module::Encoder_conv<B>*
aff3ct::factory::Encoder_conv::build<B>() const;
#endif
// ==================================================================================== explicit template instantiation
