// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/util/Thread.h>

namespace anki
{

// Forward
namespace detail
{
class ThreadPoolThread;
}

/// @addtogroup util_thread
/// @{

/// A task assignment for a ThreadPool
/// @memberof ThreadPool
class ThreadPoolTask
{
public:
	virtual ~ThreadPoolTask()
	{
	}

	virtual Error operator()(U32 taskId, PtrSize threadsCount) = 0;

	/// Chose a starting and end index
	static void choseStartEnd(U32 taskId, PtrSize threadsCount, PtrSize elementsCount, PtrSize& start, PtrSize& end)
	{
		F32 tid = taskId;
		F32 div = F32(elementsCount) / threadsCount;
		start = PtrSize(tid * div);
		end = PtrSize((tid + 1.0) * div);
	}
};

/// Parallel task dispatcher. You feed it with tasks and sends them for execution in parallel and then waits for all to
/// finish.
class ThreadPool : public NonCopyable
{
	friend class detail::ThreadPoolThread;

public:
	static constexpr U MAX_THREADS = 32; ///< An absolute limit

	/// Constructor
	ThreadPool(U32 threadsCount);

	~ThreadPool();

	/// Assign a task to a working thread
	/// @param slot The slot of the task
	/// @param task The task. If it's nullptr then a dummy task will be assigned
	void assignNewTask(U32 slot, ThreadPoolTask* task);

	/// Wait for all tasks to finish.
	/// @return The error code in one of the worker threads.
	ANKI_USE_RESULT Error waitForAllThreadsToFinish()
	{
		m_barrier.wait();
		m_tasksAssigned = 0;
		Error err = m_err;
		m_err = ErrorCode::NONE;
		return err;
	}

	PtrSize getThreadsCount() const
	{
		return m_threadsCount;
	}

private:
	/// A dummy task for a ThreadPool
	class DummyTask : public ThreadPoolTask
	{
	public:
		Error operator()(U32 taskId, PtrSize threadsCount)
		{
			(void)taskId;
			(void)threadsCount;
			return ErrorCode::NONE;
		}
	};

	Barrier m_barrier; ///< Synchronization barrier
	detail::ThreadPoolThread* m_threads = nullptr; ///< Threads array
	U m_tasksAssigned = 0;
	U8 m_threadsCount = 0;
	Error m_err = ErrorCode::NONE;
	static DummyTask m_dummyTask;
};
/// @}

} // end namespace anki
