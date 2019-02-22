/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_coloredline.hpp"
#include "wgui/wielementdata.hpp"
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_command_buffer.hpp>
#include <vulkan/vulkan.hpp>
#include <wrappers/descriptor_set_group.h>

using namespace wgui;

decltype(ShaderColoredLine::VERTEX_BINDING_VERTEX) ShaderColoredLine::VERTEX_BINDING_VERTEX = {Anvil::VertexInputRate::VERTEX};
decltype(ShaderColoredLine::VERTEX_ATTRIBUTE_COLOR) ShaderColoredLine::VERTEX_ATTRIBUTE_COLOR = {VERTEX_BINDING_VERTEX,Anvil::Format::R32G32B32A32_SFLOAT};
ShaderColoredLine::ShaderColoredLine(prosper::Context &context,const std::string &identifier)
	: ShaderColored(context,identifier,"wgui/vs_wgui_colored_vertex","wgui/fs_wgui_colored_vertex")
{
	SetBaseShader<ShaderColored>();
}

void ShaderColoredLine::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderColored::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	pipelineInfo.set_primitive_topology(Anvil::PrimitiveTopology::LINE_LIST);
	pipelineInfo.toggle_dynamic_states(true,{Anvil::DynamicState::LINE_WIDTH});
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_COLOR);
}

bool ShaderColoredLine::Draw(
	const std::shared_ptr<prosper::Buffer> &vertBuffer,const std::shared_ptr<prosper::Buffer> &colorBuffer,
	uint32_t vertCount,float lineWidth,const wgui::ElementData &pushConstants
)
{
	auto drawCmd = GetCurrentCommandBuffer();
	if(drawCmd == nullptr)
		return false;
	drawCmd->GetAnvilCommandBuffer().record_set_line_width(lineWidth);
	if(
		RecordBindVertexBuffers({&vertBuffer->GetAnvilBuffer(),&colorBuffer->GetAnvilBuffer()}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(vertCount) == false
	)
		return false;
	return true;
}