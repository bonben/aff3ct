#ifndef ENCODER_LDPC_QC_HPP_
#define ENCODER_LDPC_QC_HPP_

#include <vector>

#include "Module/Encoder/LDPC/Encoder_LDPC.hpp"
#include "Tools/Interface/Interface_reset.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Encoder_LDPC_QC: Exploit cyclic properties of Quasi-Cyclic (QC) LDPC codes for faster encoding.
 *
 * \brief
 */
template<typename B = int>
class Encoder_LDPC_QC : public Encoder_LDPC<B>
{
  protected:
    const int Zc;                  /*!< Lifting size  */
    const char* file_name;         /*!< File containing the parity check matrix */
    std::vector<std::vector<B>> G; /*!< To store generator matrix */
    const int K_LDPC;

  public:
    /*!
     * \brief Constructor.
     *
     * \param K:         Number of information bits.
     * \param N:         Number of encoded bits.
     * \param Zc:        Lifting size.
     * \param file_name: File containing the generator matrix.
     * \param K_LDPC:    Number of information bits in base graph.
     */
    Encoder_LDPC_QC(const int K, const int N, const int Zc, const char* file_name, const int K_LDPC = -1);

    /*!
     * \brief Destructor.
     */
    virtual ~Encoder_LDPC_QC() = default;

    /*!
     * \brief Clone.
     */
    virtual Encoder_LDPC_QC<B>* clone() const;

  protected:
    std::vector<std::vector<B>> read_G_file(const char* file_name);
    void _encode(const B* U_K, B* X_N, const size_t frame_id);
    std::vector<B> _CSRAA(const int Zc, std::vector<B> Gen, const B* vect, const int K, const int N);
    void _multiply_add(std::vector<B>& v, int k, std::vector<B>& r);
};
}
}

#endif /* ENCODER_LDPC_QC_HPP_ */
