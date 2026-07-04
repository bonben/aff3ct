#ifndef STD_5G_BASE_GRAPH_HPP_
#define STD_5G_BASE_GRAPH_HPP_

#include <memory>
#include <vector>

namespace aff3ct
{
namespace tools
{

struct Std_5G_base_graph
{
    int Bg;
    int Zc;
    int index_list;
    int K_LDPC;
    int N_LDPC;
};

Std_5G_base_graph
build_5G_base_graph(const int K, const int N);

}
}

#endif // STD_5G_BASE_GRAPH_HPP_
