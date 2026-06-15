/*!
 * \file
 * \brief Class tools::Interface_get_set_pretransform.
 */

#ifndef Interface_get_set_pretransform_HPP__
#define Interface_get_set_pretransform_HPP__

#include <cstdint>
#include <map>
#include <vector>

namespace aff3ct
{
namespace tools
{
class Interface_get_set_pretransform
{
  public:
    virtual void set_pretransform(const std::map<uint32_t, std::vector<uint32_t>>& pretransform) = 0;
    virtual const std::map<uint32_t, std::vector<uint32_t>>& get_pretransform() const = 0;
};
}
}

#endif // Interface_get_set_pretransform_HPP__
