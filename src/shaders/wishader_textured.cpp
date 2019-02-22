/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_textured.hpp"
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <vulkan/vulkan.hpp>
#include <prosper_util_square_shape.hpp>
#include <wrappers/descriptor_set_group.h>

using namespace wgui;

decltype(ShaderTextured::VERTEX_BINDING_VERTEX) ShaderTextured::VERTEX_BINDING_VERTEX = {Anvil::VertexInputRate::VERTEX};
decltype(ShaderTextured::VERTEX_ATTRIBUTE_POSITION) ShaderTextured::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};

decltype(ShaderTextured::VERTEX_BINDING_UV) ShaderTextured::VERTEX_BINDING_UV = {Anvil::VertexInputRate::VERTEX};
decltype(ShaderTextured::VERTEX_ATTRIBUTE_UV) ShaderTextured::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV,prosper::util::get_square_uv_format()};

decltype(ShaderTextured::DESCRIPTOR_SET_TEXTURE) ShaderTextured::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding {
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
ShaderTextured::ShaderTextured(prosper::Context &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_textured","wgui/fs_wgui_textured")
{}

ShaderTextured::ShaderTextured(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderTextured::Draw(
	const std::shared_ptr<prosper::Buffer> &vertBuffer,const std::shared_ptr<prosper::Buffer> &uvBuffer,
	uint32_t vertCount,Anvil::DescriptorSet &descSetTexture,
	const PushConstants &pushConstants
)
{
	if(
		RecordBindVertexBuffers({&vertBuffer->GetAnvilBuffer(),&uvBuffer->GetAnvilBuffer()}) == false ||
		RecordBindDescriptorSets({&descSetTexture}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(vertCount) == false
	)
		return false;
	return true;
}

void ShaderTextured::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::VERTEX_BIT | Anvil::ShaderStageFlagBits::FRAGMENT_BIT);
}

///////////////////////

ShaderTexturedRect::ShaderTexturedRect(prosper::Context &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_textured_cheap","wgui/fs_wgui_textured_cheap")
{}
ShaderTexturedRect::ShaderTexturedRect(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderTexturedRect::Draw(const PushConstants &pushConstants,Anvil::DescriptorSet &descSet)
{
	auto &dev = WGUI::GetInstance().GetContext().GetDevice();
	if(
		RecordBindVertexBuffers({&prosper::util::get_square_vertex_buffer(dev)->GetAnvilBuffer(),&prosper::util::get_square_uv_buffer(dev)->GetAnvilBuffer()}) == false ||
		RecordBindDescriptorSets({&descSet}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(prosper::util::get_square_vertex_count()) == false
	)
		return false;
	return true;
}

void ShaderTexturedRect::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,ShaderTextured::VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::VERTEX_BIT | Anvil::ShaderStageFlagBits::FRAGMENT_BIT);
}