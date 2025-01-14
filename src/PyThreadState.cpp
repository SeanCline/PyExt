#include "PyThreadState.h"
#include "PyFrame.h"
#include "PyFrameObject.h"
#include "PyInterpreterFrame.h"

#include "fieldAsPyObject.h"
#include "ExtHelpers.h"

#include <engextcpp.hpp>

#include <memory>
using namespace std;

namespace PyExt::Remote {

	PyCFrame::PyCFrame(const RemoteType& remoteType)
		: RemoteType(remoteType)
	{
	}


	PyCFrame::~PyCFrame()
	{
	}


	auto PyCFrame::current_frame() const -> std::unique_ptr<PyInterpreterFrame>
	{
		auto cframe = remoteType().Field("current_frame");
		if (cframe.GetPtr() == 0)
			return { };

		return make_unique<PyInterpreterFrame>(RemoteType(cframe));
	}


	auto PyCFrame::previous() const -> std::unique_ptr<PyCFrame>
	{
		auto previous = remoteType().Field("previous");
		if (previous.GetPtr() == 0)
			return { };

		return make_unique<PyCFrame>(RemoteType(previous));
	}


	PyThreadState::PyThreadState(const RemoteType& remoteType)
		: RemoteType(remoteType)
	{
	}


	PyThreadState::~PyThreadState()
	{
	}


	auto PyThreadState::next() const -> std::unique_ptr<PyThreadState>
	{
		auto next = remoteType().Field("next");
		if (next.GetPtr() == 0)
			return { };

		return make_unique<PyThreadState>(RemoteType(next));
	}


	auto PyThreadState::frame() const -> std::unique_ptr<PyFrameObject>
	{
		return utils::fieldAsPyObject<PyFrameObject>(remoteType(), "frame");
	}


	auto PyThreadState::cframe() const -> std::unique_ptr<PyCFrame>
	{
		auto cframe = remoteType().Field("cframe");
		if (cframe.GetPtr() == 0)
			return { };

		return make_unique<PyCFrame>(RemoteType(cframe));
	}


	auto PyThreadState::tracing() const -> long
	{
		auto field = remoteType().Field("tracing");
		return utils::readIntegral<long>(field);
	}


	auto PyThreadState::thread_id() const -> long
	{
		auto field = remoteType().Field("thread_id");
		return utils::readIntegral<long>(field);
	}


	auto PyThreadState::allFrames() const -> vector<shared_ptr<PyFrame>>
	{
		vector<shared_ptr<PyFrame>> frames;
		shared_ptr<PyFrame> f;
		f = frame();
		if (f == nullptr) {
			f = cframe()->current_frame();
		}
		for (; f != nullptr; f = f->previous()) {
			frames.push_back(f);
		}
		return frames;
	}

}