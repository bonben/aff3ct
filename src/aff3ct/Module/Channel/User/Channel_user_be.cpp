#include <sstream>
#include <mipp.h>

#include "aff3ct/Tools/Exception/exception.hpp"

#include "Channel_user_be.hpp"

using namespace aff3ct;
using namespace aff3ct::module;

template <typename R>
Channel_user_be<R>
::Channel_user_be(const int N, const std::string &filename, const int n_frames)
: Channel_user<R>(N, filename, false, n_frames)
{
	const std::string name = "Channel_user_be";
	this->set_name(name);
}

template <typename R>
void Channel_user_be<R>
::add_noise(const R *X_N, R *Y_N, const int frame_id)
{
	Channel<R>::add_noise(X_N, Y_N, frame_id);
}

template <typename R>
void Channel_user_be<R>
::_add_noise(const R *X_N, R *Y_N, const int frame_id)
{
	const mipp::Reg<R> r_erased = tools::unknown_symbol_val<R>();
	const mipp::Reg<E> r_false  = (E)false;

	this->set_noise(frame_id);


	auto event_draw = (E*)(this->noise.data() + this->N * frame_id);

	const auto vec_loop_size = (this->N / mipp::nElReg<R>()) * mipp::nElReg<R>();

	for (auto i = 0; i < vec_loop_size; i += mipp::nElReg<R>())
	{
		const mipp::Reg<R> r_in    = X_N + i;
		const mipp::Reg<E> r_event = event_draw + i;
		const auto r_out   = mipp::blend(r_in, r_erased, r_event == r_false);
		r_out.store(Y_N + i);
	}

	for (auto i = vec_loop_size; i < this->N; i++)
		Y_N[i] = event_draw[i] ? tools::unknown_symbol_val<R>() : X_N[i];
}

// ==================================================================================== explicit template instantiation
#include "aff3ct/Tools/types.h"
#ifdef AFF3CT_MULTI_PREC
template class aff3ct::module::Channel_user_be<R_32>;
template class aff3ct::module::Channel_user_be<R_64>;
#else
template class aff3ct::module::Channel_user_be<R>;
#endif
// ==================================================================================== explicit template instantiation
