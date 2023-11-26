#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "../../vkhelper/include/barrier.h"
#include "../../vkhelper/include/copy.h"
#include "../../vkhelper/include/dynstate.h"
#include "../../vkhelper/include/image.h"
#include "../../vkhelper/include/pipeline.h"
#include "../../vkhelper/include/renderpass.h"
#include "../../vkhelper/include/sampler.h"
#include "../../vkhelper/include/shader.h"
#include "../../vkhelper/include/desc.h"
#include "../../vkstatic/include/vkstatic.h"
#include "../../vkstatic/include/oneshot.h"
#include "../include/vwdedit.h"

void vwdedit_damage_all(Vwdedit *ve) {
	ve->dmg_paint.offset[0] = 0;
	ve->dmg_paint.offset[1] = 0;
	ve->dmg_paint.size[0] = ve->layer.size[0];
	ve->dmg_paint.size[1] = ve->layer.size[1];
}

static void init_pipeline_edit(Vwdedit *ve, VkDevice device) {
	VkhelperPipelineConfig vpc = {0};
	vkhelper_pipeline_config(&vpc, 0, 0, 1);
	vkhelper_pipeline_simple_shader(&vpc, device,
		__FILE__, "../../shader/edit");
	vpc.desc[0] = ve->desc.layout;
	vpc.cba.blendEnable = VK_FALSE;
	vkhelper_pipeline_build(&ve->ppll_edit, &ve->ppl_edit,
		&vpc, ve->rp_edit, device, 0);
	vkhelper_pipeline_config_deinit(&vpc, device);
}


static void init_desc(Vwdedit *ve, VkDevice device) {
	VkhelperDescConfig conf;
	vkhelper_desc_config(&conf, 2);
	vkhelper_desc_config_image(&conf, 0);
	vkhelper_desc_config_image(&conf, 1);
	vkhelper_desc_build(&ve->desc, &conf, device);
	vkhelper_desc_config_deinit(&conf);
}

static void write_desc(Vwdedit *ve, VkDevice device) {
	VkDescriptorImageInfo il, ip;
	VkWriteDescriptorSet w[2];
	vkhelper_desc_write_image(&w[0], &il, ve->desc.set,
		ve->layer.imageview, ve->sampler, 0);
	vkhelper_desc_write_image(&w[1], &ip, ve->desc.set,
		ve->paint.imageview, ve->sampler, 1);
	vkUpdateDescriptorSets(device, 2, w, 0, NULL);
}

void vwdedit_init(Vwdedit *ve, VkDevice device) {
	VkhelperRenderpassConfig renderpass_conf;
	vkhelper_renderpass_config_offscreen(&renderpass_conf, device);
	vkhelper_renderpass_build(&ve->rp_edit, &renderpass_conf, device);
	ve->sampler = vkhelper_sampler(device);
	init_desc(ve, device);
	init_pipeline_edit(ve, device);
	ve->first_setup = true;
}

static void vwdedit_reset(Vwdedit *ve, VkDevice device) {
	vkhelper_image_deinit(&ve->layer, device);
	vkhelper_image_deinit(&ve->paint, device);
	vkhelper_buffer_deinit(&ve->paint_buffer, device);
	vkDestroyFramebuffer(device, ve->fb_focus, NULL);
}

void vwdedit_deinit(Vwdedit *ve, VkDevice device) {
	vwdedit_reset(ve, device);
	vkDestroySampler(device, ve->sampler, NULL);
	vkhelper_desc_deinit(&ve->desc, device);
	vkDestroyPipeline(device, ve->ppl_edit, NULL);
	vkDestroyPipelineLayout(device, ve->ppll_edit, NULL);
	vkDestroyRenderPass(device, ve->rp_edit, NULL);
}

void vwdedit_setup(Vwdedit *ve, Vkstatic *vks,
	VkhelperImage *img, void **p) {
	// 1.deinit
	if (!ve->first_setup) {
		vwdedit_reset(ve, vks->device);
	} else {
		ve->first_setup = false;
	}

	uint32_t w = img->size[0];
	uint32_t h = img->size[1];
	vkhelper_image_new_color(
		&ve->layer, vks->device, vks->memprop, w, h, false,
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkhelper_image_new_color(
		&ve->paint, vks->device, vks->memprop, w, h, false,
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VkCommandBuffer cbuf = vkstatic_oneshot_begin(vks);
	vkhelper_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		&ve->layer);
	vkhelper_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		&ve->paint);
	vkstatic_oneshot_end(cbuf, vks);
	write_desc(ve, vks->device);
	vkhelper_buffer_init_cpu(
		&ve->paint_buffer, w * h * 4,
		vks->device, vks->memprop);
	assert(0 == vkMapMemory(vks->device, ve->paint_buffer.memory, 0,
		ve->paint_buffer.size, 0, p));
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

// upload cpu render data to gpu paint image
void vwdedit_build_command_upload(Vwdedit *ve, VkDevice device,
	VkCommandBuffer cbuf) {
	vkhelper_buffer_texture_copy(cbuf,
		ve->paint_buffer.buffer, &ve->paint, &ve->dmg_paint);
}

void vwdedit_build_command(Vwdedit *ve, VkDevice device,
	VkCommandBuffer cbuf
) {
	// printf("%d %d %u %u\n",
	// 	ve->dmg_paint.offset[0],
	// 	ve->dmg_paint.offset[1],
	// 	ve->dmg_paint.size[0],
	// 	ve->dmg_paint.size[1]);
	if (dmgrect_is_empty(&ve->dmg_paint)) { return; }
	uint32_t width = ve->paint.size[0];
	uint32_t height = ve->paint.size[1];
	vkhelper_viewport_scissor(cbuf, width, height);
	vkhelper_renderpass_begin(cbuf, ve->rp_edit, ve->fb_focus,
		width, height);
	vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ve->ppl_edit);
	vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ve->ppll_edit, 0, 1, &ve->desc.set, 0, NULL);
	vkCmdDraw(cbuf, 6, 1, 0, 0);
	vkCmdEndRenderPass(cbuf);
	dmgrect_init(&ve->dmg_paint);
}
