#pragma once

#include <utility>

template <typename Invokable>
class ScopeExit
{
	
public:
    ScopeExit(Invokable&& f)
		: f_(std::forward<Invokable>(f))
	{ }
	
	
    ~ScopeExit()
	{
		f_();
	}
	
	ScopeExit(ScopeExit&&) = default;
	ScopeExit& operator=(ScopeExit&&) = default;

	void set(Invokable&& f)
	{
		f_ = std::forward<Invokable>(f);
	}
	

private: // Non-copyable.
	ScopeExit(const ScopeExit&) = delete;
	ScopeExit& operator=(const ScopeExit&) = delete;
	
private:
    Invokable f_;

};


template <typename Invokable>
ScopeExit<Invokable> makeScopeExit(Invokable&& f) {
    return ScopeExit<Invokable>(std::forward<Invokable>(f));
};
