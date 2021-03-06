/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "XLVkPipeline.h"

namespace stappler::xenolith::vk {

Pipeline::~Pipeline() {
	if (_pipeline) {
		log::vtext("VK-Error", "PipelineLayout was not destroyed");
	}
}

bool Pipeline::init(VirtualDevice &dev, const Options &opts, const GraphicsParams &params) {
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = params.viewport.origin.x;
	viewport.y = params.viewport.origin.y;
	viewport.width = params.viewport.size.width;
	viewport.height = params.viewport.size.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset.x = params.scissor.x;
	scissor.offset.y = params.scissor.y;
	scissor.extent.width = params.scissor.width;
	scissor.extent.height = params.scissor.height;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	/*VkDynamicState dynamicStates[] = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;*/

	Vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(opts.shaders.size());
	size_t i = 0;
	for (const Pair<ProgramStage, VkShaderModule> &it : opts.shaders) {
		shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[i].stage = getVkStageBits(it.first);
		shaderStages[i].module = it.second;
		shaderStages[i].pName = "main";

		++ i;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = opts.pipelineLayout;
	pipelineInfo.renderPass = opts.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	return dev.getInstance()->vkCreateGraphicsPipelines(dev.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) == VK_SUCCESS;
}

void Pipeline::invalidate(VirtualDevice &dev) {
	if (_pipeline) {
		dev.getInstance()->vkDestroyPipeline(dev.getDevice(), _pipeline, nullptr);
		_pipeline = VK_NULL_HANDLE;
	}
}

PipelineLayout::~PipelineLayout() {
	if (_pipelineLayout || !_descriptors.empty()) {
		log::vtext("VK-Error", "PipelineLayout was not destroyed");
	}
}

bool PipelineLayout::init(VirtualDevice &dev, Type t, DescriptorCount count) {
	static constexpr uint32_t DescriptorIndexingFlags = VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT
			| VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	switch (t) {
	case None:
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		break;
	case T_0SmI_1USt:
		do {
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;
			VkDescriptorSetLayoutBinding bindings[2];
			bindings[0].binding = 0;
			bindings[0].descriptorCount = count.samplers;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			bindings[0].pImmutableSamplers = nullptr;
			bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorCount = count.sampledImages;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			bindings[1].pImmutableSamplers = nullptr;
			bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorBindingFlags flagsInfo[2];
			flagsInfo[0] = DescriptorIndexingFlags;
			flagsInfo[1] = DescriptorIndexingFlags | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

			VkDescriptorSetLayoutBindingFlagsCreateInfo createInfo;
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.bindingCount = 2;
			createInfo.pBindingFlags = flagsInfo;

			VkDescriptorSetLayoutCreateInfo layoutInfo { };
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = &createInfo;
			layoutInfo.bindingCount = 2;
			layoutInfo.pBindings = bindings;
			layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			if (dev.getInstance()->vkCreateDescriptorSetLayout(dev.getDevice(), &layoutInfo, nullptr, &layout) == VK_SUCCESS) {
				_descriptors.emplace_back(layout);
			}
		} while (0);

		do {
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;
			VkDescriptorSetLayoutBinding bindings[2];
			bindings[0].binding = 0;
			bindings[0].descriptorCount = count.uniformBuffers;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].pImmutableSamplers = nullptr;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			bindings[1].binding = 1;
			bindings[1].descriptorCount = count.storageBuffers;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			bindings[1].pImmutableSamplers = nullptr;
			bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorBindingFlags flagsInfo[2];
			flagsInfo[0] = DescriptorIndexingFlags;
			flagsInfo[1] = DescriptorIndexingFlags | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

			VkDescriptorSetLayoutBindingFlagsCreateInfo createInfo;
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.bindingCount = 2;
			createInfo.pBindingFlags = flagsInfo;

			VkDescriptorSetLayoutCreateInfo layoutInfo { };
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = &createInfo;
			layoutInfo.bindingCount = 2;
			layoutInfo.pBindings = bindings;
			layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			if (dev.getInstance()->vkCreateDescriptorSetLayout(dev.getDevice(), &layoutInfo, nullptr, &layout) == VK_SUCCESS) {
				_descriptors.emplace_back(layout);
			}
		} while (0);

		if (_descriptors.size() != 2) {
			for (VkDescriptorSetLayout &it : _descriptors) {
				dev.getInstance()->vkDestroyDescriptorSetLayout(dev.getDevice(), it, nullptr);
			}
			_descriptors.clear();
			return false;
		}
		break;
	}
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (dev.getInstance()->vkCreatePipelineLayout(dev.getDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) == VK_SUCCESS) {
		return true;
	}

	for (VkDescriptorSetLayout &it : _descriptors) {
		dev.getInstance()->vkDestroyDescriptorSetLayout(dev.getDevice(), it, nullptr);
	}
	_descriptors.clear();
	return false;
}

void PipelineLayout::invalidate(VirtualDevice &dev) {
	if (!_descriptors.empty()) {
		for (VkDescriptorSetLayout &it : _descriptors) {
			dev.getInstance()->vkDestroyDescriptorSetLayout(dev.getDevice(), it, nullptr);
		}
		_descriptors.clear();
	}

	if (_pipelineLayout) {
		dev.getInstance()->vkDestroyPipelineLayout(dev.getDevice(), _pipelineLayout, nullptr);
		_pipelineLayout = VK_NULL_HANDLE;
	}
}


RenderPass::~RenderPass() {
	if (_renderPass) {
		log::vtext("VK-Error", "RenderPass was not destroyed");
	}
}

bool RenderPass::init(VirtualDevice &dev, VkFormat imageFormat) {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	return dev.getInstance()->vkCreateRenderPass(dev.getDevice(), &renderPassInfo, nullptr, &_renderPass) == VK_SUCCESS;
}

void RenderPass::invalidate(VirtualDevice &dev) {
	if (_renderPass) {
		dev.getInstance()->vkDestroyRenderPass(dev.getDevice(), _renderPass, nullptr);
		_renderPass = VK_NULL_HANDLE;
	}
}

}
