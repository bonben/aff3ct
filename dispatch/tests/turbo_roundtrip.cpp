// Test of the SIMD neutral API of AFF3CT ('aff3ct_dispatch.hpp').
//
// This program is compiled WITHOUT any SIMD option and does not include MIPP. It builds an LTE turbo codec through
// the plugin chosen at runtime (or imposed by 'AFF3CT_SIMD'), chains it with StreamPU modules compiled here
// (Initializer, a home made BPSK -> LLR module and a Finalizer), runs the chain task by task and then through a
// StreamPU sequence, and checks that the decoded bits are equal to the encoded ones (no noise).

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <streampu.hpp>

#include "aff3ct_dispatch.hpp"

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__) || defined(__SSE4_2__) || defined(MIPP_H_)
#error "This test has to be compiled without any SIMD option and without MIPP."
#endif

namespace
{
// a StreamPU module compiled in this program: maps a bit to a saturated LLR (bit 0 -> +A, bit 1 -> -A)
template<typename B, typename Q>
class Bpsk_llr : public spu::module::Stateful
{
  public:
    Bpsk_llr(const size_t N, const Q amplitude)
      : spu::module::Stateful()
      , N(N)
      , amplitude(amplitude)
    {
        this->set_name("Bpsk_llr");
        this->set_short_name("Bpsk_llr");
        auto& t = this->create_task("modulate");
        auto s_in = this->template create_socket_in<B>(t, "X_N", N);
        auto s_out = this->template create_socket_out<Q>(t, "Y_N", N);
        this->create_codelet(t,
                             [s_in, s_out](spu::module::Module& m, spu::runtime::Task& tsk, const size_t) -> int
                             {
                                 auto& self = static_cast<Bpsk_llr&>(m);
                                 const B* in = tsk[s_in].template get_dataptr<const B>();
                                 Q* out = tsk[s_out].template get_dataptr<Q>();
                                 for (size_t i = 0; i < self.N; i++)
                                     out[i] = in[i] ? (Q)-self.amplitude : self.amplitude;
                                 return spu::runtime::status_t::SUCCESS;
                             });
    }

    Bpsk_llr* clone() const
    {
        auto m = new Bpsk_llr(*this);
        m->deep_copy(*this);
        return m;
    }

  private:
    const size_t N;
    const Q amplitude;
};

template<typename B>
size_t
count_errors(const std::vector<std::vector<B>>& ref, const std::vector<std::vector<B>>& out)
{
    size_t n_err = 0;
    for (size_t f = 0; f < ref.size(); f++)
        for (size_t i = 0; i < ref[f].size(); i++)
            n_err += (ref[f][i] != out[f][i]) ? 1 : 0;
    return n_err;
}
} // namespace

int
main(int argc, char** argv)
{
    using B = int16_t;
    using Q = int16_t;

    const int K = argc > 1 ? std::atoi(argv[1]) : 424;
    const size_t n_waves = argc > 2 ? (size_t)std::atoi(argv[2]) : 4;
    const size_t n_runs = argc > 3 ? (size_t)std::atoi(argv[3]) : 200;

    try
    {
        std::cout << "# variant: " << aff3ct::dispatch::isa() << " (MIPP: " << aff3ct::dispatch::simd_name()
                  << "), plugin: " << aff3ct::dispatch::plugin_path() << std::endl;
        std::cout << "# available on this CPU:";
        for (auto& v : aff3ct::dispatch::available_variants())
            std::cout << " " << v;
        std::cout << std::endl;

        aff3ct::dispatch::Codec codec(
          "TURBO", 16, "-K " + std::to_string(K) + " --enc-sub-std LTE --dec-sub-simd INTER -i 6");
        const int N = codec.get_N();

        const size_t lanes = aff3ct::dispatch::simd_lanes<Q>();
        const size_t n_frames = lanes * n_waves;
        codec.set_n_frames(n_frames);

        spu::module::Module& enc = codec.encoder();
        spu::module::Module& dec = codec.decoder_siho();
        std::cout << "# codec: K = " << K << ", N = " << N << ", int16 lanes = " << lanes
                  << ", n_frames = " << n_frames << ", decoder n_frames_per_wave = " << dec.get_n_frames_per_wave()
                  << " (" << dec.get_name() << ")" << std::endl;

        spu::module::Initializer<B> init(K);
        Bpsk_llr<B, Q> bpsk(N, (Q)8);
        spu::module::Finalizer<B> fin(K);
        init.set_n_frames(n_frames);
        bpsk.set_n_frames(n_frames);
        fin.set_n_frames(n_frames);

        std::mt19937 gen(42);
        std::bernoulli_distribution bit(0.5);
        std::vector<std::vector<B>> U(n_frames, std::vector<B>(K));
        for (auto& f : U)
            for (auto& b : f)
                b = bit(gen) ? 1 : 0;
        init.set_init_data(U);

        // binding of the sockets: host modules <-> plugin modules, through the StreamPU interface only
        enc["encode::U_K"] = init["initialize::out"];
        bpsk["modulate::X_N"] = enc["encode::X_N"];
        dec["decode_siho::Y_N"] = bpsk["modulate::Y_N"];
        fin["finalize::in"] = dec["decode_siho::V_K"];

        // 1) task by task
        init("initialize").exec();
        enc("encode").exec();
        bpsk("modulate").exec();
        dec("decode_siho").exec();
        fin("finalize").exec();
        const size_t err1 = count_errors(U, fin.get_final_data());
        std::cout << "# task by task: " << err1 << " bit error(s) on " << n_frames * K << " bits" << std::endl;

        // 2) StreamPU sequence (1 thread, the modules are not cloned)
        for (auto& f : U) // new frames, so that the check does not see the results of 1)
            for (auto& b : f)
                b = bit(gen) ? 1 : 0;
        init.set_init_data(U);
        spu::runtime::Sequence seq(init("initialize"), 1);
        auto t0 = std::chrono::steady_clock::now();
        for (size_t r = 0; r < n_runs; r++)
            seq.exec_seq();
        auto t1 = std::chrono::steady_clock::now();
        const size_t err2 = count_errors(U, fin.get_final_data());
        const double sec = std::chrono::duration<double>(t1 - t0).count();
        const double mbps = (double)(n_runs * n_frames * K) / sec / 1e6;
        std::cout << "# sequence: " << err2 << " bit error(s), " << n_runs << " runs, " << mbps
                  << " Mb/s (info. bits, 1 thread, encoder + decoder)" << std::endl;

        // 3) a C++ exception thrown under the plugin frames and caught here by its StreamPU type
        bool caught = false;
        try
        {
            dec.set_n_frames(0);
        }
        catch (spu::tools::invalid_argument& e)
        {
            caught = true;
        }
        std::cout << "# exception through the plugin: " << (caught ? "caught" : "NOT caught") << std::endl;

        // 4) an error of the plugin reported through the C ABI
        bool reported = false;
        try
        {
            aff3ct::dispatch::Codec bad("TURBO", 16, "-K 424 --dec-sub-simd NOPE");
        }
        catch (std::runtime_error& e)
        {
            reported = true;
        }
        std::cout << "# invalid codec arguments: " << (reported ? "reported" : "NOT reported") << std::endl;

        const bool ok = err1 == 0 && err2 == 0 && caught && reported;
        std::cout << (ok ? "OK" : "FAILED") << std::endl;
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch (std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
