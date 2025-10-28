/*!
 * \file
 * \brief Class factory::Puncturer_5G.
 */
#ifndef FACTORY_PUNCTURER_5G_HPP
#define FACTORY_PUNCTURER_5G_HPP

#include <cli.hpp>
#include <map>
#include <string>

#include "Factory/Module/Puncturer/Puncturer.hpp"
#include "Module/Puncturer/Puncturer.hpp"
#include "Tools/Factory/Header.hpp"

namespace aff3ct
{
namespace factory
{
extern const std::string Puncturer_5G_name;
extern const std::string Puncturer_5G_prefix;
class Puncturer_5G : public Puncturer
{
  public:
    // ----------------------------------------------------------------------------------------------------- PARAMETERS
    // optional parameters
    std::vector<bool> pattern;

    // -------------------------------------------------------------------------------------------------------- METHODS
    explicit Puncturer_5G(const std::string& p = Puncturer_5G_prefix);
    virtual ~Puncturer_5G() = default;
    Puncturer_5G* clone() const;

    // parameters construction
    void get_description(cli::Argument_map_info& args) const;
    void store(const cli::Argument_map_value& vals);
    void get_headers(std::map<std::string, tools::header_list>& headers, const bool full = true) const;

    // builder
    template<typename B = int, typename Q = float>
    module::Puncturer<B, Q>* build() const;
};
}
}

#endif /* FACTORY_PUNCTURER_5G_HPP */
