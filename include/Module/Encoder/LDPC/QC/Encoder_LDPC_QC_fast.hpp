#ifndef ENCODER_LDPC_QC_FAST_HPP_
#define ENCODER_LDPC_QC_FAST_HPP_

#include <vector>

#include "Module/Encoder/LDPC/QC/Encoder_LDPC_QC.hpp"
#include "Module/Module.hpp"
#include "Tools/Interface/Interface_reset.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Encoder_LDPC_QC_fast: Improved version of LDPC_Encoder_Cyclic (further exploitation of QC properties).
 *
 * \brief
 */
template<typename B = int>
class Encoder_LDPC_QC_fast : public Encoder_LDPC_QC<B>
{
  protected:
    std::vector<std::vector<B>> rot; /* Matrix for Faster Encoding, filled from G. */

  public:
    /*!
     * \brief Constructor.
     *
     * \param K:         Number of information bits.
     * \param N:         Number of encoded bits.
     * \param Zc:        Lifting size.
     * \param file_name: File containing the parity check matrix
     * \param K_LDPC:    Number of information bits in base graph.
     */
    Encoder_LDPC_QC_fast(const int K, const int N, const int Zc, const char* file_name, const int K_LDPC = -1);

    /*!
     * \brief Destructor.
     */
    virtual ~Encoder_LDPC_QC_fast() = default;

    /*!
     * \brief Clone.
     */
    virtual Encoder_LDPC_QC_fast<B>* clone() const;

  protected:
    /*!
     * \brief Fill Matrix rot from generator matrix.
     */
    void _fill_rot();
    void _encode(const B* U_K, B* X_N, const size_t frame_id);
    std::vector<B> _find_items(std::vector<B> v, int target);
};
}
}

#endif /* ENCODER_LDPC_QC_FAST_HPP_ */
