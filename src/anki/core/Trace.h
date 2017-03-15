// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/core/Common.h>
#include <anki/util/StdTypes.h>
#include <anki/util/Singleton.h>
#include <anki/util/Array.h>
#include <anki/util/HighRezTimer.h>
#include <anki/util/Thread.h>
#include <anki/util/Atomic.h>
#include <anki/util/Logger.h>
#include <anki/util/File.h>

namespace anki
{

/// @addtogroup core
/// @{

/// Trace event type.
enum class TraceEventType
{
	SCENE_UPDATE,
	SCENE_DELETE_STUFF,
	SCENE_PHYSICS_UPDATE,
	SCENE_NODES_UPDATE,
	SCENE_VISIBILITY_TESTS,
	SCENE_VISIBILITY_TEST,
	SCENE_VISIBILITY_COMBINE_RESULTS,
	SCENE_VISIBILITY_ITERATE_SECTORS,
	SCENE_VISIBILITY_GATHER_TRIANGLES,
	SCENE_VISIBILITY_RASTERIZE,
	SCENE_RASTERIZER_TEST,
	RENDER,
	RENDER_MS,
	RENDER_IS,
	RENDER_SM,
	RENDER_IR,
	RENDER_DRAWER,
	RENDERER_COMMAND_BUFFER_BUILDING,
	RENDERER_LIGHT_BINNING,
	GL_THREAD,
	GL_2ND_LEVEL_CMD_BUFFER,
	GL_BIND_RESOURCES,
	GL_BIND_PPLINE,
	GL_CMD_BUFFER_DESTROY,
	VK_ACQUIRE_IMAGE,
	VK_QUEUE_SUBMIT,
	SWAP_BUFFERS,
	BARRIER_WAIT,

	COUNT
};

/// Trace counter type.
enum class TraceCounterType
{
	GR_DRAWCALLS,
	GR_VERTICES,
	GL_PROGS_SKIPPED,
	VK_PIPELINES_CREATED,
	VK_PIPELINE_BARRIERS,
	VK_CMD_BUFFER_CREATE,
	VK_FENCE_CREATE,
	VK_SEMAPHORE_CREATE,
	RENDERER_LIGHTS,
	RENDERER_SHADOW_PASSES,
	RENDERER_MERGED_DRAWCALLS,
	RENDERER_REFLECTIONS,
	RESOURCE_ASYNC_TASKS,
	SCENE_NODES_UPDATED,
	STAGING_UNIFORMS_SIZE,
	STAGING_STORAGE_SIZE,

	COUNT
};

/// Trace manager.
class TraceManager
{
public:
	TraceManager()
	{
	}

	~TraceManager();

	ANKI_USE_RESULT Error create(HeapAllocator<U8> alloc, const CString& cacheDir);

	void startEvent();

	void stopEvent(TraceEventType type);

	void incCounter(TraceCounterType c, U64 val)
	{
		if(!m_disabled)
		{
			m_perFrameCounters[U(c)].fetchAdd(val);
		}
	}

	void startFrame();

	void stopFrame();

private:
	class Entry
	{
	public:
		TraceEventType m_event;
		HighRezTimer::Scalar m_timestamp; ///< When it started.
		HighRezTimer::Scalar m_duration;
		ThreadId m_tid;
	};

	static const U BUFFERED_ENTRIES = 1024 * 20;
	Array<Entry, BUFFERED_ENTRIES> m_entries;
	Atomic<U32> m_count = {0};
	File m_traceFile;
	File m_perFrameFile;
	File m_perRunFile;
	HighRezTimer::Scalar m_startFrameTime;

	Array<Atomic<U64>, U(TraceEventType::COUNT) + U(TraceCounterType::COUNT)> m_perFrameCounters = {{}};
	Array<Atomic<U64>, U(TraceEventType::COUNT) + U(TraceCounterType::COUNT)> m_perRunCounters = {{}};

	Bool m_disabled = false;

	ANKI_USE_RESULT Error flushCounters();
	ANKI_USE_RESULT Error flushEvents();
};

using TraceManagerSingleton = Singleton<TraceManager>;

/// @name Trace macros.
/// @{

#if ANKI_ENABLE_TRACE

#define ANKI_TRACE_START_EVENT(name_) TraceManagerSingleton::get().startEvent()

#define ANKI_TRACE_STOP_EVENT(name_) TraceManagerSingleton::get().stopEvent(TraceEventType::name_)

#define ANKI_TRACE_INC_COUNTER(name_, val_) TraceManagerSingleton::get().incCounter(TraceCounterType::name_, val_)

#define ANKI_TRACE_START_FRAME() TraceManagerSingleton::get().startFrame()

#define ANKI_TRACE_STOP_FRAME() TraceManagerSingleton::get().stopFrame()

#else

#define ANKI_TRACE_START_EVENT(name_) ((void)0)
#define ANKI_TRACE_STOP_EVENT(name_) ((void)0)
#define ANKI_TRACE_INC_COUNTER(name_, val_) ((void)0)
#define ANKI_TRACE_START_FRAME() ((void)0)
#define ANKI_TRACE_STOP_FRAME() ((void)0)

#endif

#define ANKI_TRACE_START_STOP_EVENT(execute, event)                                                                    \
	do                                                                                                                 \
	{                                                                                                                  \
		ANKI_TRACE_START_EVENT(event);                                                                                 \
		execute;                                                                                                       \
		ANKI_TRACE_STOP_EVENT(event);                                                                                  \
	} while(0)
/// @}

} // end namespace anki
