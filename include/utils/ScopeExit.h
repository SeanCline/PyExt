#pragma once

#include <utility>

namespace utils {

	template <typename Invokable>
	class ScopeExit
	{

	public:
		ScopeExit(Invokable&& f)
			: f_{ std::forward<Invokable>(f) },
			  armed_{ true }
		{ }

		~ScopeExit()
		{
			if (armed_)
				f_();
		}

		ScopeExit(ScopeExit&& other)
			: f_{ std::move(other.f_) },
			armed_{ true }
		{
			other.reset();
		};

		ScopeExit& operator=(ScopeExit&&)
		{
			f_ = std::move(other.f_);
			other.reset();
		}

		void set(Invokable&& f)
		{
			f_ = std::forward<Invokable>(f);
			armed_ = true;
		}

		void reset()
		{
			armed_ = false;
		}


	private: // Non-copyable.
		ScopeExit(const ScopeExit&) = delete;
		ScopeExit& operator=(const ScopeExit&) = delete;

	private:
		Invokable f_;
		bool armed_;

	};


	template <typename Invokable>
	ScopeExit<Invokable> makeScopeExit(Invokable&& f) {
		return ScopeExit<Invokable>(std::forward<Invokable>(f));
	};

}