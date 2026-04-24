#ifndef LAUNCHER_CONV_HPP_
#define LAUNCHER_CONV_HPP_

#include <iostream>

#include "Factory/Tools/Codec/Conv/Codec_conv.hpp"

namespace aff3ct
{
namespace launcher
{
template<class L, typename B = int, typename R = float, typename Q = R>
class Conv : public L
{
  protected:
    factory::Codec_conv* params_cdc;

  public:
    Conv(const int argc, const char** argv, std::ostream& stream = std::cout);
    virtual ~Conv() = default;

  protected:
    virtual void get_description_args();
    virtual void store_args();
};
}
}

#endif /* LAUNCHER_CONV_HPP_ */
