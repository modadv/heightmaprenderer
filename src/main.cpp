#include <bx/allocator.h>
#include <bx/debug.h>
#include <bx/file.h>
#include <bx/math.h>
#include <cstdio>

#include "bgfx_utils.h"
#include <bgfx/bgfx.h>
#include "camera.h"
#include "common.h"
#include "imgui/imgui.h"

namespace
{
	static const char* s_shaderOptions[] =
	{
		"Normal",
		"Diffuse"
	};

	static const float s_verticesL0[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
	};

	static const uint32_t s_indexesL0[] = { 0u, 1u, 2u };

	static const float s_verticesL1[] =
	{
		0.0f, 1.0f,
		0.5f, 0.5f,
		0.0f, 0.5f,
		0.0f, 0.0f,
		0.5f, 0.0f,
		1.0f, 0.0f,
	};

	static const uint32_t s_indexesL1[] =
	{
		1u, 0u, 2u,
		1u, 2u, 3u,
		1u, 3u, 4u,
		1u, 4u, 5u,
	};

	static const float s_verticesL2[] =
	{
		0.25f, 0.75f,
		0.0f,  1.0f,
		0.0f,  0.75f,
		0.0f,  0.5f,
		0.25f, 0.5f,
		0.5f,  0.5f,

		0.25f, 0.25f,
		0.0f,  0.25f,
		0.0f,  0.0f,
		0.25f, 0.0f,
		0.5f,  0.0f,
		0.5f,  0.25f,
		0.75f, 0.25f,
		0.75f, 0.0f,
		1.0f,  0.0f,
	};

	static const uint32_t s_indexesL2[] =
	{
		0u, 1u, 2u,
		0u, 2u, 3u,
		0u, 3u, 4u,
		0u, 4u, 5u,

		6u, 5u, 4u,
		6u, 4u, 3u,
		6u, 3u, 7u,
		6u, 7u, 8u,

		6u, 8u, 9u,
		6u, 9u, 10u,
		6u, 10u, 11u,
		6u, 11u, 5u,

		12u, 5u, 11u,
		12u, 11u, 10u,
		12u, 10u, 13u,
		12u, 13u, 14u,
	};

	static const float s_verticesL3[] =
	{
		0.25f * 0.5f, 0.75f * 0.5f + 0.5f,
		0.0f * 0.5f, 1.0f * 0.5f + 0.5f,
		0.0f * 0.5f, 0.75f * 0.5f + 0.5f,
		0.0f * 0.5f , 0.5f * 0.5f + 0.5f,
		0.25f * 0.5f, 0.5f * 0.5f + 0.5f,
		0.5f * 0.5f, 0.5f * 0.5f + 0.5f,
		0.25f * 0.5f, 0.25f * 0.5f + 0.5f,
		0.0f * 0.5f, 0.25f * 0.5f + 0.5f,
		0.0f * 0.5f, 0.0f * 0.5f + 0.5f,
		0.25f * 0.5f, 0.0f * 0.5f + 0.5f,
		0.5f * 0.5f, 0.0f * 0.5f + 0.5f,
		0.5f * 0.5f, 0.25f * 0.5f + 0.5f,
		0.75f * 0.5f, 0.25f * 0.5f + 0.5f,
		0.75f * 0.5f, 0.0f * 0.5f + 0.5f,
		1.0f * 0.5f, 0.0f * 0.5f + 0.5f,        //14

		0.375f, 0.375f,
		0.25f, 0.375f,
		0.25f, 0.25f,
		0.375f, 0.25f,
		0.5f, 0.25f,
		0.5f, 0.375f,    //20

		0.125f, 0.375f,
		0.0f, 0.375f,
		0.0f, 0.25f,
		0.125f, 0.25f,    //24

		0.125f, 0.125f,
		0.0f, 0.125f,
		0.0f, 0.0f,
		0.125f, 0.0f,
		0.25f, 0.0f,
		0.25f, 0.125f,    //30

		0.375f, 0.125f,
		0.375f, 0.0f,
		0.5f, 0.0f,
		0.5f, 0.125f,    //34

		0.625f, 0.375f,
		0.625f, 0.25f,
		0.75f, 0.25f,    //37

		0.625f, 0.125f,
		0.625f, 0.0f,
		0.75f, 0.0f,
		0.75f, 0.125f,    //41

		0.875f, 0.125f,
		0.875f, 0.0f,
		1.0f, 0.0f,    //44
	};

	static const uint32_t s_indexesL3[] =
	{
		0u, 1u, 2u,
		0u, 2u, 3u,
		0u, 3u, 4u,
		0u, 4u, 5u,

		6u, 5u, 4u,
		6u, 4u, 3u,
		6u, 3u, 7u,
		6u, 7u, 8u,

		6u, 8u, 9u,
		6u, 9u, 10u,
		6u, 10u, 11u,
		6u, 11u, 5u,

		12u, 5u, 11u,
		12u, 11u, 10u,
		12u, 10u, 13u,
		12u, 13u, 14u,        //End fo first big triangle

		15u, 14u, 13u,
		15u, 13u, 10u,
		15u, 10u, 16u,
		15u, 16u, 17u,
		15u, 17u, 18u,
		15u, 18u, 19u,
		15u, 19u, 20u,
		15u, 20u, 14u,

		21u, 10u, 9u,
		21u, 9u, 8u,
		21u, 8u, 22u,
		21u, 22u, 23u,
		21u, 23u, 24u,
		21u, 24u, 17u,
		21u, 17u, 16u,
		21u, 16u, 10u,

		25u, 17u, 24u,
		25u, 24u, 23u,
		25u, 23u, 26u,
		25u, 26u, 27u,
		25u, 27u, 28u,
		25u, 28u, 29u,
		25u, 29u, 30u,
		25u, 30u, 17u,

		31u, 19u, 18u,
		31u, 18u, 17u,
		31u, 17u, 30u,
		31u, 30u, 29u,
		31u, 29u, 32u,
		31u, 32u, 33u,
		31u, 33u, 34u,
		31u, 34u, 19u,

		35u, 14u, 20u,
		35u, 20u, 19u,
		35u, 19u, 36u,
		35u, 36u, 37u,

		38u, 37u, 36u,
		38u, 36u, 19u,
		38u, 19u, 34u,
		38u, 34u, 33u,
		38u, 33u, 39u,
		38u, 39u, 40u,
		38u, 40u, 41u,
		38u, 41u, 37u,

		42u, 37u, 41u,
		42u, 41u, 40u,
		42u, 40u, 43u,
		42u, 43u, 44u,
	};

	enum
	{
		PROGRAM_TERRAIN_NORMAL,
		PROGRAM_TERRAIN,

		SHADING_COUNT
	};

	enum
	{
		BUFFER_SUBD
	};

	enum
	{
		PROGRAM_SUBD_CS_LOD,
		PROGRAM_UPDATE_INDIRECT,
		PROGRAM_INIT_INDIRECT,
		PROGRAM_UPDATE_DRAW,
		PROGRAM_GENERATE_SMAP,

		PROGRAM_COUNT
	};

	enum
	{
		TERRAIN_DMAP_SAMPLER,
		TERRAIN_SMAP_SAMPLER,
		TERRAIN_DIFFUSE_SAMPLER,

		SAMPLER_COUNT
	};

	enum
	{
		TEXTURE_DMAP,
		TEXTURE_SMAP,
		TEXTURE_DIFFUSE,

		TEXTURE_COUNT
	};

	constexpr int32_t kNumVec4 = 2;

	struct Uniforms
	{
		void init()
		{
			u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, kNumVec4);
			u_aspectParams = bgfx::createUniform("u_aspectParams", bgfx::UniformType::Vec4);

			cull = 1;
			freeze = 0;
			gpuSubd = 3;
			terrainHalfWidth = 1.0f;
			terrainHalfHeight = 1.0f;
		}

		void submit()
		{
			bgfx::setUniform(u_params, params, kNumVec4);

			float aspectParams[4] = { terrainHalfWidth, terrainHalfHeight, 0.0f,  0.0f };
			bgfx::setUniform(u_aspectParams, aspectParams);
		}

		void destroy()
		{
			bgfx::destroy(u_params);
			bgfx::destroy(u_aspectParams);
		}

		union
		{
			struct
			{
				float dmapFactor;
				float lodFactor;
				float cull;
				float freeze;

				float gpuSubd;
				float padding0;
				float padding1;
				float padding2;

				float terrainHalfWidth;
				float terrainHalfHeight;
			};

			float params[kNumVec4 * 4];
		};

		bgfx::UniformHandle u_params;
		bgfx::UniformHandle u_aspectParams;
	};

	class ExampleHeightmap : public entry::AppI
	{
	public:
		ExampleHeightmap(const char* _name, const char* _description, const char* _url)
			: entry::AppI(_name, _description, _url)
			, m_terrainAspectRatio(1.0f)
		{
		}

		void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
		{
			// 记录加载开始时间
			m_loadStartTime = bx::getHPCounter();
			m_firstFrameRendered = false;
			m_showLoadTime = true;
			m_cpuSmapGenTime = 0.0f;
			m_gpuSmapGenTime = 0.0f;
			m_useGpuSmap = true;  // 默认使用GPU生成

			// 初始化加载历史
			m_loadHistoryCount = 0;
			m_showLoadHistory = true;

			Args args(_argc, _argv);

			m_width = _width;
			m_height = _height;
			m_debug = BGFX_DEBUG_NONE;
			m_reset = BGFX_RESET_NONE;

			bgfx::Init init;
			init.type = args.m_type;
			init.vendorId = args.m_pciId;
			init.resolution.width = m_width;
			init.resolution.height = m_height;
			init.resolution.reset = m_reset;

			// 关键：添加平台数据设置
			init.platformData.nwh = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
			init.platformData.ndt = entry::getNativeDisplayHandle();

			bgfx::init(init);

			// 初始化纹理选项
			initTextureOptions();

			// 默认选择第一个选项
			m_selectedHeightmap = 0;
			m_selectedDiffuse = 0;

			// 更新实际路径
			updateTexturePaths();

			m_dmap = { m_heightmapPath, 0.80f };
			m_computeThreadCount = 5;
			m_shading = PROGRAM_TERRAIN;
			m_primitivePixelLengthTarget = 1.0f;
			m_fovy = 60.0f;
			m_pingPong = 0;
			m_restart = true;
			m_texturesNeedReload = false;

			// Enable m_debug text.
			bgfx::setDebug(m_debug);

			// Set view 0 clear state.
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);

			bgfx::setViewClear(1
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);

			// Imgui.
			imguiCreate();

			m_timeOffset = bx::getHPCounter();

			m_oldWidth = 0;
			m_oldHeight = 0;
			m_oldReset = m_reset;

			cameraCreate();
			cameraSetPosition({ 0.0f, 0.9f, -1.3f });
			cameraSetVerticalAngle(0);

			m_wireframe = false;
			m_freeze = false;
			m_cull = true;

			loadPrograms();
			loadTextures();
			loadBuffers();

			createAtomicCounters();

			m_dispatchIndirect = bgfx::createIndirectBuffer(2);
		}

		virtual int shutdown() override
		{
			// Cleanup.
			cameraDestroy();
			imguiDestroy();

			m_uniforms.destroy();

			bgfx::destroy(m_bufferCounter);
			bgfx::destroy(m_bufferCulledSubd);
			bgfx::destroy(m_bufferSubd[0]);
			bgfx::destroy(m_bufferSubd[1]);
			bgfx::destroy(m_dispatchIndirect);
			bgfx::destroy(m_geometryIndices);
			bgfx::destroy(m_geometryVertices);
			bgfx::destroy(m_instancedGeometryIndices);
			bgfx::destroy(m_instancedGeometryVertices);
			bgfx::destroy(u_smapParams);

			for (uint32_t i = 0; i < PROGRAM_COUNT; ++i)
			{
				bgfx::destroy(m_programsCompute[i]);
			}

			for (uint32_t i = 0; i < SHADING_COUNT; ++i)
			{
				bgfx::destroy(m_programsDraw[i]);
			}

			for (uint32_t i = 0; i < SAMPLER_COUNT; ++i)
			{
				bgfx::destroy(m_samplers[i]);
			}

			for (uint32_t i = 0; i < TEXTURE_COUNT; ++i)
			{
				bgfx::destroy(m_textures[i]);
			}

			// Shutdown bgfx.
			bgfx::shutdown();

			return 0;
		}

		bool update() override
		{
			if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
			{
				int64_t now = bx::getHPCounter();
				static int64_t last = now;
				const int64_t frameTime = now - last;
				last = now;
				const double freq = double(bx::getHPFrequency());
				const float deltaTime = float(frameTime / freq);

				// 计算第一帧加载时间
				if (!m_firstFrameRendered)
				{
					m_loadTime = float((now - m_loadStartTime) / freq * 1000.0); // 毫秒
					m_firstFrameRendered = true;
					printf("Loading time: %.2f ms\n", m_loadTime);
					// 添加到加载历史
					if (m_loadHistoryCount < MAX_LOAD_HISTORY)
					{
						// 数组未满，直接添加
						m_loadHistory[m_loadHistoryCount].loadTimeMs = m_loadTime;
						m_loadHistory[m_loadHistoryCount].timestamp = now;
						bx::strCopy(m_loadHistory[m_loadHistoryCount].heightmapName,
							sizeof(m_loadHistory[m_loadHistoryCount].heightmapName),
							m_heightmapOptions[m_selectedHeightmap].name);
						bx::strCopy(m_loadHistory[m_loadHistoryCount].diffuseName,
							sizeof(m_loadHistory[m_loadHistoryCount].diffuseName),
							m_diffuseOptions[m_selectedDiffuse].name);
						m_loadHistoryCount++;
					}
					else
					{
						// 数组已满，移动元素
						for (int i = 0; i < MAX_LOAD_HISTORY - 1; i++)
						{
							m_loadHistory[i] = m_loadHistory[i + 1];
						}

						// 添加新记录到末尾
						m_loadHistory[MAX_LOAD_HISTORY - 1].loadTimeMs = m_loadTime;
						m_loadHistory[MAX_LOAD_HISTORY - 1].timestamp = now;
						bx::strCopy(m_loadHistory[MAX_LOAD_HISTORY - 1].heightmapName,
							sizeof(m_loadHistory[MAX_LOAD_HISTORY - 1].heightmapName),
							m_heightmapOptions[m_selectedHeightmap].name);
						bx::strCopy(m_loadHistory[MAX_LOAD_HISTORY - 1].diffuseName,
							sizeof(m_loadHistory[MAX_LOAD_HISTORY - 1].diffuseName),
							m_diffuseOptions[m_selectedDiffuse].name);
					}
				}

				imguiBeginFrame(
					m_mouseState.m_mx
					, m_mouseState.m_my
					, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
					| (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
					| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
					, m_mouseState.m_mz
					, uint16_t(m_width)
					, uint16_t(m_height)
				);

				showExampleDialog(this);

				ImGui::SetNextWindowPos(
					ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f)
					, ImGuiCond_FirstUseEver
				);
				ImGui::SetNextWindowSize(
					ImVec2(m_width / 5.0f, m_height / 3.0f)
					, ImGuiCond_FirstUseEver
				);

				ImGui::Begin("Settings", NULL, 0);

				if (m_showLoadTime)
				{
					ImGui::Text("Loading time: %.2f ms", m_loadTime);
				}

				if (m_showSmapStats)
				{
					ImGui::Separator();
					ImGui::Text("SMap Generation Stats:");

					if (m_cpuSmapGenTime > 0.0f)
						ImGui::Text("CPU Time: %.2f ms", m_cpuSmapGenTime);

					if (m_gpuSmapGenTime > 0.0f)
						ImGui::Text("GPU Time: %.2f ms", m_gpuSmapGenTime);

					if (m_cpuSmapGenTime > 0.0f && m_gpuSmapGenTime > 0.0f)
					{
						float speedup = m_cpuSmapGenTime / m_gpuSmapGenTime;
						ImGui::Text("GPU Speedup: %.1fx", speedup);

						float maxTime = bx::max(m_cpuSmapGenTime, m_gpuSmapGenTime);
						ImGui::Text("Relative Performance:");
						ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
						ImGui::ProgressBar(m_cpuSmapGenTime / maxTime, ImVec2(-1, 0), "CPU");
						ImGui::PopStyleColor();
						ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
						ImGui::ProgressBar(m_gpuSmapGenTime / maxTime, ImVec2(-1, 0), "GPU");
						ImGui::PopStyleColor();
					}
				}

				ImGui::Separator();

				ImGui::Text("Terrain Textures");

				if (ImGui::Checkbox("Use GPU SMap Generation", &m_useGpuSmap))
				{
					if (bgfx::isValid(m_textures[TEXTURE_SMAP]))
					{
						bgfx::destroy(m_textures[TEXTURE_SMAP]);
					}

					m_loadStartTime = bx::getHPCounter();
					m_firstFrameRendered = false;
					if (m_useGpuSmap) {
						loadSmapTextureGPU();
					}
					else {
						loadSmapTexture();
					}
				}

				if (ImGui::Button("Compare CPU/GPU Performance", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
				{
					if (bgfx::isValid(m_textures[TEXTURE_SMAP]))
					{
						bgfx::destroy(m_textures[TEXTURE_SMAP]);
					}
					loadSmapTextureGPU();

					bgfx::destroy(m_textures[TEXTURE_SMAP]);
					loadSmapTexture();

					bgfx::destroy(m_textures[TEXTURE_SMAP]);
					if (m_useGpuSmap) {
						loadSmapTextureGPU();
					}
					else {
						loadSmapTexture();
					}

				}

				ImGui::Text("Heightmap:");
				if (ImGui::BeginCombo("##HeightmapSelector", m_heightmapOptions[m_selectedHeightmap].name))
				{
					for (int i = 0; i < MAX_HEIGHTMAP_OPTIONS; i++)
					{
						bool isSelected = (m_selectedHeightmap == i);
						if (ImGui::Selectable(m_heightmapOptions[i].name, isSelected))
						{
							m_selectedHeightmap = i;
							updateTexturePaths();
							m_texturesNeedReload = true;


							m_selectedDiffuse = i;
							updateTexturePaths();
							m_texturesNeedReload = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				// 显示当前路径
				ImGui::TextWrapped("Heightmap: %s", m_heightmapPath);
				ImGui::TextWrapped("Diffuse: %s", m_diffuseTexturePath);

				// 重新加载按钮
				if (ImGui::Button("Reload Textures", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
				{
					m_texturesNeedReload = true;
				}

				if (ImGui::Checkbox("Debug wireframe", &m_wireframe))
				{
					bgfx::setDebug(m_wireframe
						? BGFX_DEBUG_WIREFRAME
						: BGFX_DEBUG_NONE
					);
				}

				ImGui::SameLine();

				if (ImGui::Checkbox("Cull", &m_cull))
				{
					m_uniforms.cull = m_cull ? 1.0f : 0.0f;
				}

				ImGui::SameLine();

				if (ImGui::Checkbox("Freeze subdividing", &m_freeze))
				{
					m_uniforms.freeze = m_freeze ? 1.0f : 0.0f;
				}


				ImGui::SliderFloat("Pixels per edge", &m_primitivePixelLengthTarget, 1, 20);

				int gpuSlider = (int)m_uniforms.gpuSubd;

				if (ImGui::SliderInt("Triangle Patch level", &gpuSlider, 0, 3))
				{
					m_restart = true;
					m_uniforms.gpuSubd = float(gpuSlider);
				}

				ImGui::Combo("Shading", &m_shading, s_shaderOptions, 2);

				ImGui::Text("Some variables require rebuilding the subdivide buffers and causes a stutter.");

				ImGui::End();

				// 处理纹理重新加载
				if (m_texturesNeedReload)
				{
					m_texturesNeedReload = false;

					// 记录重新加载开始时间
					m_loadStartTime = bx::getHPCounter();
					m_firstFrameRendered = false;
					m_showLoadTime = true;

					// 更新DMap路径
					m_dmap.pathToFile = m_heightmapPath;

					// 清理旧的纹理资源
					if (bgfx::isValid(m_textures[TEXTURE_DMAP]))
					{
						bgfx::destroy(m_textures[TEXTURE_DMAP]);
					}

					if (bgfx::isValid(m_textures[TEXTURE_SMAP]))
					{
						bgfx::destroy(m_textures[TEXTURE_SMAP]);
					}

					if (bgfx::isValid(m_textures[TEXTURE_DIFFUSE]))
					{
						bgfx::destroy(m_textures[TEXTURE_DIFFUSE]);
					}

					// 重新加载纹理
					loadDmapTexture();

					if (m_useGpuSmap) {
						loadSmapTextureGPU();
					}
					else {
						loadSmapTexture();
					}
					loadDiffuseTexture();

					// 更新地形几何
					bgfx::destroy(m_geometryVertices);
					bgfx::destroy(m_geometryIndices);
					loadGeometryBuffers();

					// 重建细分缓冲区
					m_restart = true;
				}

				// Update camera.
				cameraUpdate(deltaTime * 0.01f, m_mouseState);

				bgfx::touch(0);
				bgfx::touch(1);

				configureUniforms();

				cameraGetViewMtx(m_viewMtx);

				float model[16];

				bx::mtxRotateX(model, bx::toRad(90));

				bx::mtxProj(m_projMtx, m_fovy, float(m_width) / float(m_height), 0.0001f, 2000.0f, bgfx::getCaps()->homogeneousDepth);

				// Set view 0
				bgfx::setViewTransform(0, m_viewMtx, m_projMtx);

				// Set view 1
				bgfx::setViewRect(1, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewTransform(1, m_viewMtx, m_projMtx);

				m_uniforms.submit();

				// update the subd buffers
				if (m_restart)
				{
					m_pingPong = 1;

					bgfx::destroy(m_instancedGeometryVertices);
					bgfx::destroy(m_instancedGeometryIndices);

					bgfx::destroy(m_bufferSubd[BUFFER_SUBD]);
					bgfx::destroy(m_bufferSubd[BUFFER_SUBD + 1]);
					bgfx::destroy(m_bufferCulledSubd);

					loadInstancedGeometryBuffers();
					loadSubdivisionBuffers();

					//init indirect
					bgfx::setBuffer(1, m_bufferSubd[m_pingPong], bgfx::Access::ReadWrite);
					bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::ReadWrite);
					bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
					bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
					bgfx::setBuffer(8, m_bufferSubd[1 - m_pingPong], bgfx::Access::ReadWrite);
					bgfx::dispatch(0, m_programsCompute[PROGRAM_INIT_INDIRECT], 1, 1, 1);


					m_restart = false;
				}
				else
				{
					// update batch
					bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
					bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
					bgfx::dispatch(0, m_programsCompute[PROGRAM_UPDATE_INDIRECT], 1, 1, 1);
				}

				bgfx::setBuffer(1, m_bufferSubd[m_pingPong], bgfx::Access::ReadWrite);
				bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::ReadWrite);
				bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
				bgfx::setBuffer(6, m_geometryVertices, bgfx::Access::Read);
				bgfx::setBuffer(7, m_geometryIndices, bgfx::Access::Read);
				bgfx::setBuffer(8, m_bufferSubd[1 - m_pingPong], bgfx::Access::Read);
				bgfx::setTransform(model);

				bgfx::setTexture(0, m_samplers[TERRAIN_DMAP_SAMPLER], m_textures[TEXTURE_DMAP], BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

				m_uniforms.submit();

				// update the subd buffer
				bgfx::dispatch(0, m_programsCompute[PROGRAM_SUBD_CS_LOD], m_dispatchIndirect, 1);

				// update draw
				bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
				bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);

				m_uniforms.submit();

				bgfx::dispatch(1, m_programsCompute[PROGRAM_UPDATE_DRAW], 1, 1, 1);

				// render the terrain
				bgfx::setTexture(0, m_samplers[TERRAIN_DMAP_SAMPLER], m_textures[TEXTURE_DMAP], BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
				bgfx::setTexture(1, m_samplers[TERRAIN_SMAP_SAMPLER], m_textures[TEXTURE_SMAP], BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC);

				if (bgfx::isValid(m_textures[TEXTURE_DIFFUSE]))
				{
					uint32_t diffuseSamplerFlags = BGFX_SAMPLER_UVW_MIRROR
						| BGFX_SAMPLER_MIN_ANISOTROPIC
						| BGFX_SAMPLER_MAG_ANISOTROPIC
						| BGFX_SAMPLER_MIP_POINT;

					bgfx::setTexture(5, m_samplers[TERRAIN_DIFFUSE_SAMPLER], m_textures[TEXTURE_DIFFUSE], diffuseSamplerFlags);
				}

				bgfx::setTransform(model);
				bgfx::setVertexBuffer(0, m_instancedGeometryVertices);
				bgfx::setIndexBuffer(m_instancedGeometryIndices);
				bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::Read);
				bgfx::setBuffer(3, m_geometryVertices, bgfx::Access::Read);
				bgfx::setBuffer(4, m_geometryIndices, bgfx::Access::Read);
				bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);

				m_uniforms.submit();

				bgfx::submit(1, m_programsDraw[m_shading], m_dispatchIndirect);

				m_pingPong = 1 - m_pingPong;

				imguiEndFrame();

				// Advance to next frame. Rendering thread will be kicked to
				// process submitted rendering primitives.
				bgfx::frame(false);
				return true;
			}

			return false;
		}

		void createAtomicCounters()
		{
			m_bufferCounter = bgfx::createDynamicIndexBuffer(3, BGFX_BUFFER_INDEX32 | BGFX_BUFFER_COMPUTE_READ_WRITE);
		}

		void configureUniforms()
		{
			float lodFactor = 2.0f * bx::tan(bx::toRad(m_fovy) / 2.0f)
				/ m_width * (1 << (int)m_uniforms.gpuSubd)
				* m_primitivePixelLengthTarget
				;

			m_uniforms.lodFactor = lodFactor;
			m_uniforms.dmapFactor = m_dmap.scale;

			// Set aspect ratio parameters for the uniform
			m_uniforms.terrainHalfWidth = m_terrainAspectRatio;
			m_uniforms.terrainHalfHeight = 1.0f;
		}



		void loadPrograms()
		{
			m_samplers[TERRAIN_DMAP_SAMPLER] = bgfx::createUniform("u_DmapSampler", bgfx::UniformType::Sampler);
			m_samplers[TERRAIN_SMAP_SAMPLER] = bgfx::createUniform("u_SmapSampler", bgfx::UniformType::Sampler);
			m_samplers[TERRAIN_DIFFUSE_SAMPLER] = bgfx::createUniform("u_DiffuseSampler", bgfx::UniformType::Sampler);

			m_uniforms.init();

			m_programsDraw[PROGRAM_TERRAIN] = loadProgram("vs_terrain_render", "fs_terrain_render");
			m_programsDraw[PROGRAM_TERRAIN_NORMAL] = loadProgram("vs_terrain_render", "fs_terrain_render_normal");

			m_programsCompute[PROGRAM_SUBD_CS_LOD] = bgfx::createProgram(loadShader("cs_terrain_lod"), true);
			m_programsCompute[PROGRAM_UPDATE_INDIRECT] = bgfx::createProgram(loadShader("cs_terrain_update_indirect"), true);
			m_programsCompute[PROGRAM_UPDATE_DRAW] = bgfx::createProgram(loadShader("cs_terrain_update_draw"), true);
			m_programsCompute[PROGRAM_INIT_INDIRECT] = bgfx::createProgram(loadShader("cs_terrain_init"), true);

			m_programsCompute[PROGRAM_GENERATE_SMAP] = bgfx::createProgram(loadShader("cs_generate_smap"), true);
			u_smapParams = bgfx::createUniform("u_smapParams", bgfx::UniformType::Vec4);

		}

		void loadSmapTextureGPU()
		{
			int64_t startTime = bx::getHPCounter();
			if (!dmap || dmap->m_width == 0 || dmap->m_height == 0)
			{
				const bgfx::Memory* mem = bgfx::alloc(2 * sizeof(float));
				float* defaultSlopeData = (float*)mem->data;
				defaultSlopeData[0] = 0.0f;
				defaultSlopeData[1] = 0.0f;

				m_textures[TEXTURE_SMAP] = bgfx::createTexture2D(
					1, 1, false, 1, bgfx::TextureFormat::RG32F,
					BGFX_TEXTURE_NONE, mem
				);
				m_gpuSmapGenTime = 0.0f;
				return;
			}

			uint16_t w = static_cast<uint16_t>(dmap->m_width);
			uint16_t h = static_cast<uint16_t>(dmap->m_height);
			int mipcnt = dmap->m_numMips;

			m_textures[TEXTURE_SMAP] = bgfx::createTexture2D(
				(uint16_t)w,
				(uint16_t)h,
				mipcnt > 1,
				1,
				bgfx::TextureFormat::RG32F,
				BGFX_TEXTURE_COMPUTE_WRITE
			);

			float smapParams[4] = { (float)w, (float)h, m_terrainAspectRatio, 1.0f };
			bgfx::setUniform(u_smapParams, smapParams);

			bgfx::setTexture(0, m_samplers[TERRAIN_DMAP_SAMPLER], m_textures[TEXTURE_DMAP], BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

			bgfx::setImage(1, m_textures[TEXTURE_SMAP], 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA32F);

			uint16_t groupsX = (w + 15) / 16;
			uint16_t groupsY = (h + 15) / 16;
			bgfx::dispatch(0, m_programsCompute[PROGRAM_GENERATE_SMAP], groupsX, groupsY, 1);
			bgfx::frame();


			int64_t endTime = bx::getHPCounter();
			m_gpuSmapGenTime = float((endTime - startTime) / double(bx::getHPFrequency()) * 1000.0);


			printf("+++++++++++++++++++++++++++++++++++++++  GPU SMap generation time: %.2f ms (for %dx%d heightmap)\n", m_gpuSmapGenTime, w, h);

			if (m_cpuSmapGenTime > 0.0f) {
				float speedup = m_cpuSmapGenTime / m_gpuSmapGenTime;
				printf("/////////////////////////////////  GPU vs CPU speedup: %.1fx\n", speedup);
			}
		}

		void loadSmapTexture()
		{
			int64_t startTime = bx::getHPCounter();
			if (!dmap || dmap->m_width == 0 || dmap->m_height == 0)
			{
				const bgfx::Memory* mem = bgfx::alloc(2 * sizeof(float));
				float* defaultSlopeData = (float*)mem->data;
				defaultSlopeData[0] = 0.0f;
				defaultSlopeData[1] = 0.0f;

				m_textures[TEXTURE_SMAP] = bgfx::createTexture2D(
					1, 1, false, 1, bgfx::TextureFormat::RG32F,
					BGFX_TEXTURE_NONE, mem
				);
				m_cpuSmapGenTime = 0.0f;
				return;
			}

			int w = dmap->m_width;
			int h = dmap->m_height;

			const uint16_t* texels = (const uint16_t*)dmap->m_data;

			int mipcnt = dmap->m_numMips;

			const bgfx::Memory* mem = bgfx::alloc(w * h * 2 * sizeof(float));
			float* smap = (float*)mem->data;

			for (int j = 0; j < h; ++j)
			{
				for (int i = 0; i < w; ++i)
				{
					int i1 = bx::max(0, i - 1);
					int i2 = bx::min(w - 1, i + 1);
					int j1 = bx::max(0, j - 1);
					int j2 = bx::min(h - 1, j + 1);
					uint16_t px_l = texels[i1 + w * j]; // in [0,2^16-1]
					uint16_t px_r = texels[i2 + w * j]; // in [0,2^16-1]
					uint16_t px_b = texels[i + w * j1]; // in [0,2^16-1]
					uint16_t px_t = texels[i + w * j2]; // in [0,2^16-1]
					float z_l = (float)px_l / 65535.0f; // in [0, 1]
					float z_r = (float)px_r / 65535.0f; // in [0, 1]
					float z_b = (float)px_b / 65535.0f; // in [0, 1]
					float z_t = (float)px_t / 65535.0f; // in [0, 1]
					float slope_x = (float)w * 0.5f * (z_r - z_l);
					float slope_y = (float)h * 0.5f * (z_t - z_b);

					smap[2 * (i + w * j)] = slope_x;
					smap[1 + 2 * (i + w * j)] = slope_y;
				}
			}

			m_textures[TEXTURE_SMAP] = bgfx::createTexture2D(
				(uint16_t)w
				, (uint16_t)h
				, mipcnt > 1
				, 1
				, bgfx::TextureFormat::RG32F
				, BGFX_TEXTURE_NONE
				, mem
			);

			int64_t endTime = bx::getHPCounter();
			m_cpuSmapGenTime = float((endTime - startTime) / double(bx::getHPFrequency()) * 1000.0);

			printf("+++++++++++++++++++++++  CPU SMap generation time: %.2f ms\n", m_cpuSmapGenTime);
		}

		void loadDmapTexture()
		{
			// 尝试加载高度图文件
			dmap = imageLoad(m_dmap.pathToFile.getCPtr(), bgfx::TextureFormat::R16);

			// 检查是否加载成功
			if (!dmap)
			{
				// 加载失败，使用一个默认的1x1纹理
				printf("Failed to load heightmap: %s\n", m_dmap.pathToFile.getCPtr());

				// 创建默认1x1白色高度图
				const bgfx::Memory* mem = bgfx::alloc(sizeof(uint16_t));
				uint16_t* defaultHeightData = (uint16_t*)mem->data;
				*defaultHeightData = 0; // 黑色，高度为0

				m_textures[TEXTURE_DMAP] = bgfx::createTexture2D(
					1, 1, false, 1, bgfx::TextureFormat::R16,
					BGFX_TEXTURE_NONE, mem
				);

				m_terrainAspectRatio = 1.0f;
				return;
			}

			if (dmap->m_height > 0)  // Avoid division by zero
			{
				m_terrainAspectRatio = (float)dmap->m_width / (float)dmap->m_height;
			}
			else
			{
				m_terrainAspectRatio = 1.0f;
			}

			m_textures[TEXTURE_DMAP] = bgfx::createTexture2D(
				(uint16_t)dmap->m_width
				, (uint16_t)dmap->m_height
				, false
				, 1
				, bgfx::TextureFormat::R16
				, BGFX_TEXTURE_NONE
				, bgfx::makeRef(dmap->m_data, dmap->m_size)
			);
		}

		void loadDiffuseTexture()
		{
			// 使用成员变量中的路径
			const char* filePath = m_diffuseTexturePath;
			uint64_t textureFlags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_UVW_BORDER
				| BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIP_SHIFT;

			// 尝试加载表面纹理
			m_textures[TEXTURE_DIFFUSE] = loadTexture(filePath, textureFlags);

			// 处理加载失败的情况
			if (!bgfx::isValid(m_textures[TEXTURE_DIFFUSE])) {
				BX_TRACE("Failed to load diffuse texture: %s, using default texture", filePath);

				// 可以选择加载一个默认纹理
				// 这里简单地创建一个1x1的灰色纹理
				const bgfx::Memory* mem = bgfx::alloc(4); // RGBA8 格式需要4字节
				uint8_t* data = mem->data;
				data[0] = data[1] = data[2] = 128; // 灰色
				data[3] = 255; // 不透明

				m_textures[TEXTURE_DIFFUSE] = bgfx::createTexture2D(
					1, 1, false, 1, bgfx::TextureFormat::RGBA8,
					BGFX_TEXTURE_NONE, mem
				);
			}
			else {
				BX_TRACE("Loaded diffuse texture: %s", filePath);
			}
		}

		bool m_useGpuSmap = true;
		void loadTextures()
		{
			loadDmapTexture();
			if (m_useGpuSmap) {
				loadSmapTextureGPU();
			}
			else {
				loadSmapTexture();
			}
			loadDiffuseTexture();
		}


		void loadGeometryBuffers()
		{
			// 计算新的 X 坐标范围
			const float halfWidth = m_terrainAspectRatio;  // Aspect ratio is width relative to height=1;
			const float halfHeight = 1.0f;

			const float vertices[] =
			{
				-halfWidth, -halfHeight, 0.0f, 1.0f, // 左下 (-2.27, -1.0)
				+halfWidth, -halfHeight, 0.0f, 1.0f, // 右下 (+2.27, -1.0)
				+halfWidth, +halfHeight, 0.0f, 1.0f, // 右上 (+2.27, +1.0)
				-halfWidth, +halfHeight, 0.0f, 1.0f, // 左上 (-2.27, +1.0)
			};

			const uint32_t indices[] = { 0, 1, 3, 2, 3, 1 };

			m_geometryLayout.begin().add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float).end();

			m_geometryVertices = bgfx::createVertexBuffer(
				bgfx::copy(vertices, sizeof(vertices))
				, m_geometryLayout
				, BGFX_BUFFER_COMPUTE_READ
			);
			m_geometryIndices = bgfx::createIndexBuffer(
				bgfx::copy(indices, sizeof(indices))
				, BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_INDEX32
			);
		}

		void loadSubdivisionBuffers()
		{
			const uint32_t bufferCapacity = 1 << 27;

			m_bufferSubd[BUFFER_SUBD] = bgfx::createDynamicIndexBuffer(
				bufferCapacity
				, BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
			);

			m_bufferSubd[BUFFER_SUBD + 1] = bgfx::createDynamicIndexBuffer(
				bufferCapacity
				, BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
			);

			m_bufferCulledSubd = bgfx::createDynamicIndexBuffer(
				bufferCapacity
				, BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
			);
		}


		void loadBuffers()
		{
			loadSubdivisionBuffers();
			loadGeometryBuffers();
			loadInstancedGeometryBuffers();
		}


		void loadInstancedGeometryBuffers()
		{
			const float* vertices;
			const uint32_t* indexes;

			switch (int32_t(m_uniforms.gpuSubd))
			{
			case 0:
				m_instancedMeshVertexCount = 3;
				m_instancedMeshPrimitiveCount = 1;
				vertices = s_verticesL0;
				indexes = s_indexesL0;
				break;

			case 1:
				m_instancedMeshVertexCount = 6;
				m_instancedMeshPrimitiveCount = 4;
				vertices = s_verticesL1;
				indexes = s_indexesL1;
				break;

			case 2:
				m_instancedMeshVertexCount = 15;
				m_instancedMeshPrimitiveCount = 16;
				vertices = s_verticesL2;
				indexes = s_indexesL2;
				break;

			default:
				m_instancedMeshVertexCount = 45;
				m_instancedMeshPrimitiveCount = 64;
				vertices = s_verticesL3;
				indexes = s_indexesL3;
				break;
			}

			m_instancedGeometryLayout
				.begin()
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.end();

			m_instancedGeometryVertices = bgfx::createVertexBuffer(
				bgfx::makeRef(vertices, sizeof(float) * 2 * m_instancedMeshVertexCount)
				, m_instancedGeometryLayout
			);

			m_instancedGeometryIndices = bgfx::createIndexBuffer(
				bgfx::makeRef(indexes, sizeof(uint32_t) * m_instancedMeshPrimitiveCount * 3)
				, BGFX_BUFFER_INDEX32
			);
		}

		void initTextureOptions()
		{
			m_heightmapOptions[0] = { "0049", "textures/0049_16bit.png" };
			m_heightmapOptions[1] = { "1972", "textures/1972_16bit.png" };

			m_diffuseOptions[0] = { "0049", "textures/0049.jpg" };
			m_diffuseOptions[1] = { "1972", "textures/1972.png" };
		}

		void updateTexturePaths()
		{
			bx::strCopy(m_heightmapPath, BX_COUNTOF(m_heightmapPath),
				m_heightmapOptions[m_selectedHeightmap].path);
			bx::strCopy(m_diffuseTexturePath, BX_COUNTOF(m_diffuseTexturePath),
				m_diffuseOptions[m_selectedDiffuse].path);
		}

		Uniforms m_uniforms;

		bgfx::ProgramHandle m_programsCompute[PROGRAM_COUNT];
		bgfx::ProgramHandle m_programsDraw[SHADING_COUNT];
		bgfx::TextureHandle m_textures[TEXTURE_COUNT];
		bgfx::UniformHandle m_samplers[SAMPLER_COUNT];

		bgfx::DynamicIndexBufferHandle m_bufferSubd[2];
		bgfx::DynamicIndexBufferHandle m_bufferCulledSubd;

		bgfx::DynamicIndexBufferHandle m_bufferCounter;

		bgfx::IndexBufferHandle m_geometryIndices;
		bgfx::VertexBufferHandle m_geometryVertices;
		bgfx::VertexLayout m_geometryLayout;

		bgfx::IndexBufferHandle m_instancedGeometryIndices;
		bgfx::VertexBufferHandle m_instancedGeometryVertices;
		bgfx::VertexLayout m_instancedGeometryLayout;

		bgfx::IndirectBufferHandle m_dispatchIndirect;

		bimg::ImageContainer* dmap;

		float m_terrainAspectRatio;

		float m_viewMtx[16];
		float m_projMtx[16];

		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_debug;
		uint32_t m_reset;

		uint32_t m_oldWidth;
		uint32_t m_oldHeight;
		uint32_t m_oldReset;

		uint32_t m_instancedMeshVertexCount;
		uint32_t m_instancedMeshPrimitiveCount;

		entry::MouseState m_mouseState;

		int64_t m_timeOffset;

		struct DMap
		{
			bx::FilePath pathToFile;
			float scale;
		};

		DMap m_dmap;

		int m_computeThreadCount;
		int m_shading;
		int m_gpuSubd;
		int m_pingPong;

		float m_primitivePixelLengthTarget;
		float m_fovy;

		bool m_restart;
		bool m_wireframe;
		bool m_cull;
		bool m_freeze;

		int64_t m_loadStartTime;
		bool m_firstFrameRendered;
		float m_loadTime;
		bool m_showLoadTime;

		struct TextureOption {
			const char* name;
			const char* path;
		};

		static const int MAX_HEIGHTMAP_OPTIONS = 2;
		static const int MAX_DIFFUSE_OPTIONS = 2;

		TextureOption m_heightmapOptions[MAX_HEIGHTMAP_OPTIONS];
		TextureOption m_diffuseOptions[MAX_DIFFUSE_OPTIONS];

		int m_selectedHeightmap;
		int m_selectedDiffuse;
		bool m_texturesNeedReload;

		char m_heightmapPath[256];
		char m_diffuseTexturePath[256];

		struct LoadTimeRecord {
			float loadTimeMs;
			char heightmapName[64];
			char diffuseName[64];
			int64_t timestamp;
		};

		static const int MAX_LOAD_HISTORY = 5;
		LoadTimeRecord m_loadHistory[MAX_LOAD_HISTORY];
		int m_loadHistoryCount;
		bool m_showLoadHistory;

		bgfx::UniformHandle u_smapParams;

		float m_cpuSmapGenTime = 0.0f;    // CPU生成SMap的时间（毫秒）
		float m_gpuSmapGenTime = 0.0f;    // GPU生成SMap的时间（毫秒）
		bool m_showSmapStats = true;      // 是否显示SMap生成统计信息
		bool m_measureCpuTime = false;    // 是否测量CPU时间（用于比较）
		bool m_measureGpuTime = false;    // 是否测量GPU时间（用于比较）
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	ExampleHeightmap
	, "Heightmap"
	, "Adaptive GPU Tessellation."
	, "https://bkaradzic.github.io/bgfx/examples.html#tess"
);
