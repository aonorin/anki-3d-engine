// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/core/StagingGpuMemoryManager.h>
#include <anki/misc/ConfigSet.h>
#include <anki/gr/GrManager.h>
#include <anki/core/Trace.h>

namespace anki
{

StagingGpuMemoryManager::~StagingGpuMemoryManager()
{
	m_gr->finish();

	for(auto& it : m_perFrameBuffers)
	{
		it.m_buff->unmap();
		it.m_buff = {};
	}
}

Error StagingGpuMemoryManager::init(GrManager* gr, const ConfigSet& cfg)
{
	m_gr = gr;

	m_perFrameBuffers[StagingGpuMemoryType::UNIFORM].m_size = cfg.getNumber("core.uniformPerFrameMemorySize");
	m_perFrameBuffers[StagingGpuMemoryType::STORAGE].m_size = cfg.getNumber("core.storagePerFrameMemorySize");
	m_perFrameBuffers[StagingGpuMemoryType::VERTEX].m_size = cfg.getNumber("core.vertexPerFrameMemorySize");
	m_perFrameBuffers[StagingGpuMemoryType::TRANSFER].m_size = cfg.getNumber("core.transferPerFrameMemorySize");
	m_perFrameBuffers[StagingGpuMemoryType::TEXTURE].m_size = cfg.getNumber("core.textureBufferPerFrameMemorySize");

	U32 alignment;
	PtrSize maxSize;

	gr->getUniformBufferInfo(alignment, maxSize);
	initBuffer(StagingGpuMemoryType::UNIFORM, alignment, maxSize, BufferUsageBit::UNIFORM_ALL, *gr);

	gr->getStorageBufferInfo(alignment, maxSize);
	initBuffer(StagingGpuMemoryType::STORAGE, alignment, maxSize, BufferUsageBit::STORAGE_ALL, *gr);

	initBuffer(StagingGpuMemoryType::VERTEX, 16, MAX_U32, BufferUsageBit::VERTEX, *gr);
	initBuffer(StagingGpuMemoryType::TRANSFER,
		16,
		MAX_U32,
		BufferUsageBit::BUFFER_UPLOAD_SOURCE | BufferUsageBit::TEXTURE_UPLOAD_SOURCE,
		*gr);

	gr->getTextureBufferInfo(alignment, maxSize);
	initBuffer(StagingGpuMemoryType::TEXTURE, alignment, maxSize, BufferUsageBit::TEXTURE_ALL, *gr);

	return ErrorCode::NONE;
}

void StagingGpuMemoryManager::initBuffer(
	StagingGpuMemoryType type, U32 alignment, PtrSize maxAllocSize, BufferUsageBit usage, GrManager& gr)
{
	auto& perframe = m_perFrameBuffers[type];

	perframe.m_buff = gr.newInstance<Buffer>(perframe.m_size, usage, BufferMapAccessBit::WRITE);
	perframe.m_alloc.init(perframe.m_size, alignment, maxAllocSize);
	perframe.m_mappedMem = static_cast<U8*>(perframe.m_buff->map(0, perframe.m_size, BufferMapAccessBit::WRITE));
}

void* StagingGpuMemoryManager::allocateFrame(PtrSize size, StagingGpuMemoryType usage, StagingGpuMemoryToken& token)
{
	PerFrameBuffer& buff = m_perFrameBuffers[usage];
	Error err = buff.m_alloc.allocate(size, token.m_offset);
	if(err)
	{
		ANKI_CORE_LOGF("Out of staging GPU memory. Usage: %u", usage);
	}

	token.m_buffer = buff.m_buff;
	token.m_range = size;
	token.m_type = usage;

	return buff.m_mappedMem + token.m_offset;
}

void* StagingGpuMemoryManager::tryAllocateFrame(PtrSize size, StagingGpuMemoryType usage, StagingGpuMemoryToken& token)
{
	PerFrameBuffer& buff = m_perFrameBuffers[usage];
	Error err = buff.m_alloc.allocate(size, token.m_offset);
	if(!err)
	{
		token.m_buffer = buff.m_buff;
		token.m_range = size;
		token.m_type = usage;
		return buff.m_mappedMem + token.m_offset;
	}
	else
	{
		token = {};
		return nullptr;
	}
}

void StagingGpuMemoryManager::endFrame()
{
	for(StagingGpuMemoryType usage = StagingGpuMemoryType::UNIFORM; usage < StagingGpuMemoryType::COUNT; ++usage)
	{
		PerFrameBuffer& buff = m_perFrameBuffers[usage];

		if(buff.m_mappedMem)
		{
			// Increase the counters
			switch(usage)
			{
			case StagingGpuMemoryType::UNIFORM:
				ANKI_TRACE_INC_COUNTER(STAGING_UNIFORMS_SIZE, buff.m_alloc.getUnallocatedMemorySize());
				break;
			case StagingGpuMemoryType::STORAGE:
				ANKI_TRACE_INC_COUNTER(STAGING_STORAGE_SIZE, buff.m_alloc.getUnallocatedMemorySize());
				break;
			default:
				break;
			}

			buff.m_alloc.endFrame();
		}
	}
}

} // end namespace anki
