/*!
 * \file
 * \brief Class tools::Pretransform_generator_file.
 */
#ifndef PRETRANSFORM_GENERATOR_FILE_HPP_
#define PRETRANSFORM_GENERATOR_FILE_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "Tools/Code/Polar/Pretransform_generator/Pretransform_generator.hpp"

namespace aff3ct
{
namespace tools
{
class Pretransform_generator_file : public Pretransform_generator
{
  protected:
    const std::string filename;

  protected:
    Pretransform_generator_file(const int K, const int N);

  public:
    Pretransform_generator_file(const int K, const int N, const std::string& filename);

    virtual ~Pretransform_generator_file() = default;

    virtual Pretransform_generator_file* clone() const;

  protected:
    void evaluate();
    bool load_channels_file(const std::string& filename, std::vector<uint32_t>& best_channels);
    bool load_pretransform_file(const std::string& filename,
                                std::map<uint32_t, std::vector<uint32_t>>& pretransform,
                                std::vector<uint32_t>& info_bits_loc);
    virtual void check_noise();
};
}
}

#endif /* PRETRANSFORM_GENERATOR_FILE_HPP_ */
