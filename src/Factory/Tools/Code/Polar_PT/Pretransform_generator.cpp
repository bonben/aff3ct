#include <streampu.hpp>
#include <utility>

#include "Factory/Tools/Code/Polar_PT/Pretransform_generator.hpp"
#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator_file.hpp"
#include "Tools/Documentation/documentation.h"

using namespace aff3ct;
using namespace aff3ct::factory;

const std::string aff3ct::factory::Pretransform_generator_name = "Pre Transform generator";
const std::string aff3ct::factory::Pretransform_generator_prefix = "ptg";

Pretransform_generator ::Pretransform_generator(const std::string& prefix)
  : Factory(Pretransform_generator_name, Pretransform_generator_name, prefix)
{
    if (!cli::Is_path::check(this->path_fb))
    {
        auto new_path_fb = cli::modify_path<cli::Is_path>(this->path_fb);
        if (!new_path_fb.empty()) this->path_fb = new_path_fb;
    }
}

Pretransform_generator*
Pretransform_generator ::clone() const
{
    return new Pretransform_generator(*this);
}

void
Pretransform_generator ::get_description(cli::Argument_map_info& args) const
{
    auto p = this->get_prefix();
    const std::string class_name = "factory::Pretransform_generator::";

    // tools::add_arg(
    // args, p, class_name + "p+info-bits,K", cli::Integer(cli::Positive(), cli::Non_zero()), cli::arg_rank::REQ);

    // tools::add_arg(
    //   args, p, class_name + "p+cw-size,N", cli::Integer(cli::Positive(), cli::Non_zero()), cli::arg_rank::REQ);

    // tools::add_arg(args, p, class_name + "p+noise", cli::Real(cli::Positive(), cli::Non_zero()));

    tools::add_arg(args, p, class_name + "p+gen-method", cli::Text(cli::Including_set("FILE")));

    // tools::add_arg(args, p, class_name + "p+awgn-path", cli::Path(cli::openmode::read));

    // tools::add_arg(args, p, class_name + "p+dump-path", cli::Folder(cli::openmode::write));

    tools::add_arg(args, p, class_name + "p+path", cli::Text());
}

void
Pretransform_generator ::store(const cli::Argument_map_value& vals)
{
    auto p = this->get_prefix();
    // std::cout << "prefix: " << p << ", in " << __FILE__ << std::endl;

    // if (vals.exist({ p + "-info-bits", "K" })) this->K = vals.to_int({ p + "-info-bits", "K" });
    // if (vals.exist({ p + "-cw-size", "N" })) this->N_cw = vals.to_int({ p + "-cw-size", "N" });
    // if (vals.exist({ p + "-noise" })) this->noise = vals.to_float({ p + "-noise" });
    if (vals.exist({ p + "-path" })) this->path_fb = vals.to_path({ p + "-path" });
    if (vals.exist({ p + "-gen-method" })) this->type = vals.at({ p + "-gen-method" });
    if (vals.exist({ p + "-dump-path" })) this->dump_channels_path = vals.to_folder({ p + "-dump-path" });
}

void
Pretransform_generator ::get_headers(std::map<std::string, tools::header_list>& headers, const bool full) const
{
    auto p = this->get_prefix();

    headers[p].push_back(std::make_pair("Type", this->type));
    // if (full) headers[p].push_back(std::make_pair("Info. bits (K)", std::to_string(this->K)));
    // if (full) headers[p].push_back(std::make_pair("Codeword size (N)", std::to_string(this->N_cw)));
    // headers[p].push_back(std::make_pair("Noise", this->noise == -1.0f ? "adaptive" : std::to_string(this->noise)));
    if (this->type == "FILE") headers[p].push_back(std::make_pair("Path", this->path_fb));
    // if (!this->dump_channels_path.empty() && (this->type == "GA" || this->type == "BEC"))
    //     headers[p].push_back(std::make_pair("Dump channels path", this->dump_channels_path));
}

tools::Pretransform_generator*
Pretransform_generator ::build() const
{
    if (this->type == "FILE") return new tools::Pretransform_generator_file(this->K, this->N_cw, this->path_fb);

    throw spu::tools::cannot_allocate(__FILE__, __LINE__, __func__);
}
