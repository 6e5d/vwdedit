#include <vulkan/vulkan.h>

#include "../../vkhelper2/include/vkhelper2.h"
#include "../../vkstatic/include/vkstatic.h"
#include "../include/vwdedit.h"

#define VWDEDIT_PPL_COUNT 2

static void init_pipeline_edit(Vwdedit *ve, VkDevice device,
	size_t idx, char *name
) {
	Vkhelper2PipelineConfig vpc = {0};
	vkhelper2_pipeline_config(&vpc, 0, 0, 1);
	char path[256];
	snprintf(path, 256, "../../shader/%s_frag.spv", name);
	vkhelper2_pipeline_simple_shader2(&vpc, device, __FILE__,
		"../../shader/edit_vert.spv", path);
	vpc.desc[0] = ve->desc.layout;
	vpc.cba.blendEnable = VK_FALSE;
	vkhelper2_pipeline_build(&ve->ppll[idx], &ve->ppl[idx],
		&vpc, ve->rp_edit, device, 0);
	vkhelper2_pipeline_config_deinit(&vpc, device);
}


static void init_desc(Vwdedit *ve, VkDevice device) {
	Vkhelper2DescConfig conf;
	vkhelper2_desc_config(&conf, 2);
	vkhelper2_desc_config_image(&conf, 0);
	vkhelper2_desc_config_image(&conf, 1);
	vkhelper2_desc_build(&ve->desc, &conf, device);
	vkhelper2_desc_config_deinit(&conf);
}

static void write_desc(Vwdedit *ve, VkDevice device) {
	VkDescriptorImageInfo il, ip;
	VkWriteDescriptorSet w[2];
	vkhelper2_desc_write_image(&w[0], &il, ve->desc.set,
		ve->layer.imageview, ve->sampler, 0);
	vkhelper2_desc_write_image(&w[1], &ip, ve->desc.set,
		ve->paint.imageview, ve->sampler, 1);
	vkUpdateDescriptorSets(device, 2, w, 0, NULL);
}

void vwdedit_init(Vwdedit *ve, VkDevice device) {
	ve->pidx = 0;
	Vkhelper2RenderpassConfig renderpass_conf;
	vkhelper2_renderpass_config_offscreen(&renderpass_conf);
	vkhelper2_renderpass_build(&ve->rp_edit, &renderpass_conf, device);
	ve->sampler = vkhelper2_sampler(device);
	init_desc(ve, device);
	ve->ppl = malloc(VWDEDIT_PPL_COUNT * sizeof(VkPipeline));
	ve->ppll = malloc(VWDEDIT_PPL_COUNT * sizeof(VkPipelineLayout));
	init_pipeline_edit(ve, device, 0, "pen");
	init_pipeline_edit(ve, device, 1, "eraser");
	ve->first_setup = true;
}

static void vwdedit_reset(Vwdedit *ve, VkDevice device) {
	vkhelper2_image_deinit(&ve->layer, device);
	vkhelper2_image_deinit(&ve->paint, device);
	vkhelper2_buffer_deinit(&ve->paint_buffer, device);
	vkhelper2_buffer_deinit(&ve->layer_buffer, device);
	vkDestroyFramebuffer(device, ve->fb_focus, NULL);
}

void vwdedit_deinit(Vwdedit *ve, VkDevice device) {
	vwdedit_reset(ve, device);
	vkDestroySampler(device, ve->sampler, NULL);
	vkhelper2_desc_deinit(&ve->desc, device);
	for (size_t i = 0; i < VWDEDIT_PPL_COUNT; i += 1) {
		vkDestroyPipeline(device, ve->ppl[i], NULL);
		vkDestroyPipelineLayout(device, ve->ppll[i], NULL);
	}
	free(ve->ppl);
	free(ve->ppll);
	vkDestroyRenderPass(device, ve->rp_edit, NULL);
}

void vwdedit_setup(Vwdedit *ve, Vkstatic *vks,
	Vkhelper2Image *img, void **p_paint, void **p_layer) {
	// 1.deinit
	if (!ve->first_setup) {
		vwdedit_reset(ve, vks->device);
	} else {
		ve->first_setup = false;
	}

	uint32_t w = img->size[0];
	uint32_t h = img->size[1];
	vkhelper2_image_new_color(
		&ve->layer, vks->device, vks->memprop, w, h, false,
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	vkhelper2_image_new_color(
		&ve->paint, vks->device, vks->memprop, w, h, false,
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VkCommandBuffer cbuf = vkstatic_oneshot_begin(vks);
	vkhelper2_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		&ve->layer);
	vkhelper2_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		&ve->paint);
	vkstatic_oneshot_end(cbuf, vks);
	write_desc(ve, vks->device);
	vkhelper2_buffer_init_cpu(&ve->paint_buffer, w * h * 4,
		vks->device, vks->memprop);
	vkhelper2_buffer_init_cpu(&ve->layer_buffer, w * h * 4,
		vks->device, vks->memprop);
	assert(0 == vkMapMemory(vks->device, ve->paint_buffer.memory, 0,
		ve->paint_buffer.size, 0, p_paint));
	assert(0 == vkMapMemory(vks->device, ve->layer_buffer.memory, 0,
		ve->layer_buffer.size, 0, p_layer));
	VkFramebufferCreateInfo fbci = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = ve->rp_edit,
		.attachmentCount = 1,
		.pAttachments = &img->imageview,
		.width = w,
		.height = h,
		.layers = 1
	};
	assert(0 == vkCreateFramebuffer(
		vks->device, &fbci, NULL, &ve->fb_focus));
}

void vwdedit_blend(Vwdedit *ve, VkCommandBuffer cbuf, Dmgrect *dmg) {
	if (dmgrect_is_empty(dmg)) { return; }
	uint32_t width = ve->paint.size[0];
	uint32_t height = ve->paint.size[1];
	vkhelper2_dynstate_vs(cbuf, width, height);
	vkhelper2_renderpass_begin(cbuf, ve->rp_edit, ve->fb_focus,
		width, height);
	vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ve->ppl[ve->pidx]);
	vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ve->ppll[ve->pidx], 0, 1, &ve->desc.set, 0, NULL);
	vkCmdDraw(cbuf, 6, 1, 0, 0);
	vkCmdEndRenderPass(cbuf);
}
