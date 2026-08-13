#ifndef LAUNCHER_POLAR_PT_HPP_
#define LAUNCHER_POLAR_PT_HPP_

#include <iostream>

#include "Factory/Tools/Codec/Polar_PT/Codec_polar_PT.hpp"

namespace aff3ct
{
namespace launcher
{
template<class L, typename B = int, typename R = float, typename Q = R>
class Polar_PT : public L
{
  protected:
    factory::Codec_polar_PT* params_cdc;

  public:
    Polar_PT(const int argc, const char** argv, std::ostream& stream = std::cout);
    virtual ~Polar_PT() = default;

  protected:
    virtual void get_description_args();
    virtual void store_args();
};
}
}

#endif /* LAUNCHER_POLAR_PT_HPP_ */
