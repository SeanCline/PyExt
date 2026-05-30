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
		auto tlbc = tlbcIndex();
		auto interpAddr = interpreterStateOffset();

		// Skip over any incomplete frames. Python does this using _PyFrame_GetFirstComplete.
		while (frame.GetPtr() != 0) {
			PyInterpreterFrame candidate{ RemoteType(frame), tlbc, interpAddr };
			if (!candidate.isIncomplete())
				break;
			frame = frame.Field("previous");
		}

		if (frame.GetPtr() == 0)
			return { };
		return make_unique<PyInterpreterFrame>(RemoteType(frame), tlbc, interpAddr);
	}


	auto PyThreadState::tlbcIndex() const -> std::optional<int>
	{
		// Added in Python 3.13 under Py_GIL_DISABLED. Absent on GIL builds.
		if (!remoteType().HasField("tlbc_index"))
			return std::nullopt;
		auto field = remoteType().Field("tlbc_index");
		return utils::readIntegral<int>(field);
	}


	auto PyThreadState::interpreterStateOffset() const -> std::optional<uint64_t>
	{
		if (!remoteType().HasField("interp"))
			return std::nullopt;
		auto interp = remoteType().Field("interp").GetPtr();
		if (interp == 0)
			return std::nullopt;
		return interp;
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