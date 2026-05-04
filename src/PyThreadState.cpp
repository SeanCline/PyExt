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


	auto PyThreadState::currentFrame() const -> std::unique_ptr<PyFrame>
	{
		auto frameObject = utils::fieldAsPyObject<PyFrameObject>(remoteType(), "frame");
		if (frameObject != nullptr)
			return frameObject;  // Python < 3.11

		auto frameContainer = remoteType();  // Python 3.13+
		if (remoteType().HasField("cframe")) {
			// Python 3.11, 3.12
			frameContainer = remoteType().Field("cframe");
			if (frameContainer.GetPtr() == 0)
				return { };
		}

		auto frame = frameContainer.Field("current_frame");

		// Python 3.14 may set current_frame to an incomplete frame (owner >= 3:
		// FRAME_OWNED_BY_INTERPRETER = 3 or FRAME_OWNED_BY_CSTACK = 4).  Skip to
		// the first complete frame, mirroring _PyThreadState_GetFrame / _PyFrame_GetFirstComplete.
		while (frame.GetPtr() != 0) {
			auto ownerRaw = frame.Field("owner");
			auto owner = utils::readIntegral<int8_t>(ownerRaw);
			if (owner < 3)
				break;
			frame = frame.Field("previous");
		}

		if (frame.GetPtr() == 0)
			return { };
		return make_unique<PyInterpreterFrame>(RemoteType(frame));
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
		f = currentFrame();
		for (; f != nullptr; f = f->previous()) {
			frames.push_back(f);
		}
		return frames;
	}

}