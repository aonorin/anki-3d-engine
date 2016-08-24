// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/renderer/MainRenderer.h>
#include <anki/renderer/Renderer.h>
#include <anki/renderer/Is.h>
#include <anki/renderer/Pps.h>
#include <anki/renderer/Dbg.h>
#include <anki/renderer/Ms.h>
#include <anki/renderer/Ir.h>
#include <anki/scene/SceneGraph.h>
#include <anki/scene/Camera.h>
#include <anki/util/Logger.h>
#include <anki/util/File.h>
#include <anki/util/Filesystem.h>
#include <anki/core/Trace.h>
#include <anki/core/App.h>
#include <anki/misc/ConfigSet.h>

namespace anki
{

//==============================================================================
MainRenderer::MainRenderer()
{
}

//==============================================================================
MainRenderer::~MainRenderer()
{
	ANKI_LOGI("Destroying main renderer");
	m_materialShaderSource.destroy(m_alloc);
}

//==============================================================================
Error MainRenderer::create(ThreadPool* threadpool,
	ResourceManager* resources,
	GrManager* gr,
	AllocAlignedCallback allocCb,
	void* allocCbUserData,
	const ConfigSet& config,
	Timestamp* globTimestamp)
{
	ANKI_LOGI("Initializing main renderer");

	m_alloc = HeapAllocator<U8>(allocCb, allocCbUserData);
	m_frameAlloc =
		StackAllocator<U8>(allocCb, allocCbUserData, 1024 * 1024 * 10, 1.0);

	// Init default FB
	m_width = config.getNumber("width");
	m_height = config.getNumber("height");
	FramebufferInitInfo fbInit;
	fbInit.m_colorAttachmentCount = 1;
	fbInit.m_colorAttachments[0].m_loadOperation =
		AttachmentLoadOperation::DONT_CARE;
	m_defaultFb = gr->newInstance<Framebuffer>(fbInit);

	// Init renderer and manipulate the width/height
	ConfigSet config2 = config;
	m_renderingQuality = config.getNumber("renderingQuality");
	UVec2 size(
		m_renderingQuality * F32(m_width), m_renderingQuality * F32(m_height));
	size.x() = getAlignedRoundDown(TILE_SIZE, size.x() / 2) * 2;
	size.y() = getAlignedRoundDown(TILE_SIZE, size.y() / 2) * 2;

	config2.set("width", size.x());
	config2.set("height", size.y());

	m_rDrawToDefaultFb = m_renderingQuality == 1.0;

	m_r.reset(m_alloc.newInstance<Renderer>());
	ANKI_CHECK(m_r->init(threadpool,
		resources,
		gr,
		m_alloc,
		m_frameAlloc,
		config2,
		globTimestamp,
		m_rDrawToDefaultFb));

	// Set the default preprocessor string
	m_materialShaderSource.sprintf(m_alloc,
		"#define ANKI_RENDERER_WIDTH %u\n"
		"#define ANKI_RENDERER_HEIGHT %u\n"
		"#define TILE_SIZE %u\n",
		m_r->getWidth(),
		m_r->getHeight(),
		TILE_SIZE);

	// Init other
	if(!m_rDrawToDefaultFb)
	{
		ANKI_CHECK(m_r->getResourceManager().loadResource(
			"shaders/Final.frag.glsl", m_blitFrag));

		ColorStateInfo colorState;
		colorState.m_attachmentCount = 1;
		m_r->createDrawQuadPipeline(
			m_blitFrag->getGrShader(), colorState, m_blitPpline);

		// Init RC group
		ResourceGroupInitInfo rcinit;
		rcinit.m_textures[0].m_texture = m_r->getPps().getRt();
		m_rcGroup = m_r->getGrManager().newInstance<ResourceGroup>(rcinit);
		ANKI_LOGI("The main renderer will have to blit the offscreen "
				  "renderer's result");
	}

	ANKI_LOGI(
		"Main renderer initialized. Rendering size %ux%u", m_width, m_height);

	return ErrorCode::NONE;
}

//==============================================================================
Error MainRenderer::render(SceneGraph& scene)
{
	ANKI_TRACE_START_EVENT(RENDER);

	// First thing, reset the temp mem pool
	m_frameAlloc.getMemoryPool().reset();

	GrManager& gl = m_r->getGrManager();
	CommandBufferInitInfo cinf;
	cinf.m_hints = m_cbInitHints;
	CommandBufferPtr cmdb = gl.newInstance<CommandBuffer>(cinf);

	// Set some of the dynamic state
	cmdb->setPolygonOffset(0.0, 0.0);

	// Run renderer
	RenderingContext ctx(m_frameAlloc);

	if(m_rDrawToDefaultFb)
	{
		ctx.m_outFb = m_defaultFb;
		ctx.m_outFbWidth = m_width;
		ctx.m_outFbHeight = m_height;
	}

	ctx.m_commandBuffer = cmdb;
	ctx.m_frustumComponent =
		&scene.getActiveCamera().getComponent<FrustumComponent>();
	ANKI_CHECK(m_r->render(ctx));

	// Blit renderer's result to default FB if needed
	if(!m_rDrawToDefaultFb)
	{
		cmdb->beginRenderPass(m_defaultFb);
		cmdb->setViewport(0, 0, m_width, m_height);

		cmdb->bindPipeline(m_blitPpline);
		cmdb->bindResourceGroup(m_rcGroup, 0, nullptr);

		m_r->drawQuad(cmdb);
		cmdb->endRenderPass();
	}

	// Flush the command buffer
	cmdb->flush();

	// Set the hints
	m_cbInitHints = cmdb->computeInitHints();

	ANKI_TRACE_STOP_EVENT(RENDER);

	return ErrorCode::NONE;
}

//==============================================================================
Dbg& MainRenderer::getDbg()
{
	return m_r->getDbg();
}

//==============================================================================
F32 MainRenderer::getAspectRatio() const
{
	return m_r->getAspectRatio();
}

} // end namespace anki