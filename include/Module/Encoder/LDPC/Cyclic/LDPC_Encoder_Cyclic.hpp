#ifndef LDPC_ENCODER_CYCLIC_HPP_
#define LDPC_ENCODER_CYCLIC_HPP_

#include <vector>

#include "Module/Encoder/LDPC/Encoder_LDPC.hpp"
#include "Tools/Interface/Interface_reset.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class LDPC_Encoder_Cyclic: Exploit cyclic properties of Quasi-Cyclic (QC) LDPC codes for faster encoding.
 *
 * \brief
 */
template<typename B = int>
class LDPC_Encoder_Cyclic : public Encoder_LDPC<B>
{
  protected:
    const int Zc;                  /*!< Lifting size  */
    const char* file_name;         /*!< File containing the parity check matrix */
    std::vector<std::vector<B>> G; /*!< To store generator matrix */

  public:
    /*!
     * \brief Constructor.
     *
     * \param K:         Number of information bits.
     * \param N:         Number of encoded bits.
     * \param Zc:        Lifting size.
     * \param file_name: File containing the generator matrix.
     */
    LDPC_Encoder_Cyclic(const int K, const int N, const int Zc, const char* file_name);

    /*!
     * \brief Destructor.
     */
    virtual ~LDPC_Encoder_Cyclic() = default;

    /*!
     * \brief Clone.
     */
    virtual LDPC_Encoder_Cyclic<B>* clone() const;

  protected:
    std::vector<std::vector<B>> read_G_file(const char* file_name);
    void _encode(const B* U_K, B* X_N, const size_t frame_id);
    std::vector<B> _CSRAA(const int Zc, std::vector<B> Gen, const B* vect, const int K, const int N);
    void _MultiplyAdd(std::vector<B>& v, int k, std::vector<B>& r);
};
}
}

#endif /* LDPC_ENCODER_CYCLIC_HPP_ */
