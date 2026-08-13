/*!
 * \file
 * \brief Class tools::Pretransform_generator.
 */
#ifndef PRETRANSFORM_GENERATOR_HPP_
#define PRETRANSFORM_GENERATOR_HPP_

#include <cstdint>
#include <map>
#include <memory>
#include <streampu.hpp>
#include <string>
#include <thread>
#include <vector>

#include "Tools/Interface/Interface_get_set_noise.hpp"
#include "Tools/Noise/Noise.hpp"

namespace aff3ct
{
namespace tools
{
/*!
 * \class Pretransform_generator
 * \brief Determines the frozen bit positions in a frame.
 */
class Pretransform_generator
  : public Interface_get_set_noise
  , public spu::tools::Interface_clone
{
  private:
    static std::thread::id master_thread_id;
    const std::string dump_channels_path;
    const bool dump_channels_single_thread;

  protected:
    const int K; /*!< Number of information bits in the frame. */
    const int N; /*!< Codeword size (or frame size). */
    const tools::Noise<>* noise;

    std::vector<uint32_t> best_channels; /*!< The best channels in a codeword sorted by descending order. */

    std::vector<uint32_t> info_bits_loc;
    std::map<uint32_t, std::vector<uint32_t>>
      preTransform; /*!< This map stores the pretransform info and dynamic forzen bits*/

  public:
    /*!
     * \brief Constructor.
     *
     * \param K:     number of information bits in the frame.
     * \param N:     codeword size (or frame size).
     */
    Pretransform_generator(const int K,
                           const int N,
                           const std::string& dump_channels_path = "",
                           const bool dump_channels_single_thread = true);

    /*!
     * \brief Destructor.
     */
    virtual ~Pretransform_generator() = default;

    virtual Pretransform_generator* clone() const = 0;

    int get_K() const;

    int get_N() const;

    /*!
     * \brief Sets the current noise to apply to the input signal
     *
     * \param noise: the current noise to apply to the input signal
     */
    void set_noise(const tools::Noise<>& noise);

    const tools::Noise<>& get_noise() const;

    /*!
     * \brief Generates the frozen bits vector.
     *
     * \param frozen_bits: output vector of frozen bits.
     */
    void generate(std::vector<bool>& frozen_bits);

    /*!
     * \brief Gets the best channels (the most secured bits sorted by descending order).
     *
     * \return a vector of the best channels.
     */
    const std::vector<uint32_t>& get_best_channels() const;

    /*!
     * \brief Gets the pre transform matrix, and the dynamic frozn bits.
     *
     * \return a map keys are the info or the dynamic frozen bits.
     */
    const std::map<uint32_t, std::vector<uint32_t>>& get_pre_transform() const;

    void dump_best_channels(const std::string& dump_channels_full_path) const;

  protected:
    /*!
     * \brief Evaluates the best channels.
     *
     * It is mandatory to compute the best channels to find the position of the frozen bits.
     * This method fills the internal Pretransform_generator::best_channels attribute.
     */
    virtual void evaluate() = 0;

    /*!
     * \brief Check that the noise has the expected type
     */
    virtual void check_noise();
};
}
}

#endif /* PRETRANSFORM_GENERATOR_HPP_ */
