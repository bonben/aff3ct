// SIMD neutral launcher of the AFF3CT simulator: it chooses the best plugin for the running CPU (or the one imposed by
// 'AFF3CT_SIMD=sse4.2|avx2|avx512') and calls its simulator entry point. Compiled without any SIMD option.

#include <cstdlib>
#include <exception>
#include <iostream>

#include "aff3ct_dispatch.hpp"

int
main(int argc, char** argv)
{
    try
    {
        return aff3ct::dispatch::run_simulator(argc, argv);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
