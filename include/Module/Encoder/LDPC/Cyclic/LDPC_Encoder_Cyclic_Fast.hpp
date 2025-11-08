#ifndef LDPC_ENCODER_CYCLIC_FAST_HPP_
#define LDPC_ENCODER_CYCLIC_FAST_HPP_

#include <vector>

#include "Module/Encoder/LDPC/Cyclic/LDPC_Encoder_Cyclic.hpp"
#include "Module/Module.hpp"
#include "Tools/Interface/Interface_reset.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class LDPC_Encoder_Cyclic_Fast: Improved version of LDPC_Encoder_Cyclic (further exploitation of QC properties).
 *
 * \brief
 */
template<typename B = int>
class LDPC_Encoder_Cyclic_Fast : public LDPC_Encoder_Cyclic<B>
{
  protected:
    std::vector<std::vector<B>> Rot; /* Matrix for Faster Encoding, filled from G. */
    const int K_ldpc;
    std::vector<B> filled_input;

  public:
    /*!
     * \brief Constructor.
     *
     * \param K:         Number of information bits.
     * \param N:         Number of encoded bits.
     * \param Zc:        Lifting size.
     * \param file_name: File containing the parity check matrix
     * \param K_ldpc:    Number of information bits in base graph.
     */
    LDPC_Encoder_Cyclic_Fast(const int K, const int N, const int Zc, const char* file_name, const int K_ldpc);

    /*!
     * \brief Destructor.
     */
    virtual ~LDPC_Encoder_Cyclic_Fast() = default;

    /*!
     * \brief Clone.
     */
    virtual LDPC_Encoder_Cyclic_Fast<B>* clone() const;

  protected:
    /*!
     * \brief Fill Matrix Rot from generator matrix.
     */
    void _fill_Rot();
    void _encode(const B* U_K, B* X_N, const size_t frame_id);
    std::vector<B> _CSRAA(B* vect, const int beg);
    std::vector<B> _findItems(std::vector<B> v, int target);
};
}
}

#endif /* LDPC_ENCODER_CYCLIC_FAST_HPP_ */
