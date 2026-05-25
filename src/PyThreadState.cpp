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

		// Skip over any incomplete frames, mirroring _PyThreadState_GetFrame and _PyFrame_GetFirstComplete.
		// For now, a frame is "incomplete" when owner >= 3:
		//   Python 3.13: FRAME_OWNED_BY_CSTACK = 3.
		//   Python 3.14: FRAME_OWNED_BY_INTERPRETER = 3, FRAME_OWNED_BY_CSTACK = 4.
		//   Python 3.15: FRAME_OWNED_BY_INTERPRETER = 3 (FRAME_OWNED_BY_CSTACK removed).
		// In all recent versions, a frame with owner >= 3 is incomplete and should be skipped.
		// see https://github.com/python/cpython/blob/3bd942f106aa36c261a2d90104c027026b2a8fb6/Python/traceback.c#L979-L982
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