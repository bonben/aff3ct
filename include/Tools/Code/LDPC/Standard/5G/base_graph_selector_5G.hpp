/*!
 * \file
 * \brief Struct tools::base_graph_selector.
 *
 */
#ifndef BASE_GRAPH_SELECTOR_5G_HPP_
#define BASE_GRAPH_SELECTOR_5G_HPP_

#include <memory>
#include <vector>

namespace aff3ct
{
namespace tools
{

struct base_graph_5G
{
    int Bg;
	int Zc;
	int index_list;
	int Kldpc;
	int Nldpc;
};

base_graph_5G
build_5G_base_graph(const int K, const int N);

}
}

#endif // DVBS2_CONSTANTS_HPP_
