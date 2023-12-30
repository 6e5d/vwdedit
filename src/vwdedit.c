#include "../include/vwdedit.h"

const static size_t ppl_count = 2;

static void init_pipeline_edit(Vwdedit() *ve, VkDevice device,
	size_t idx, char *name
) {
	Vkhelper2(PipelineConfig) vpc = {0};
	vkhelper2(pipeline_config)(&vpc, 0, 0, 1);
	char path[256];
	snprintf(path, 256, "../../vwdraw_shaders/build/%s_frag.spv", name);
	vkhelper2(pipeline_simple_shader2)(&vpc, device, __FILE__,
		"../../vwdraw_shaders/build/edit_vert.spv", path);
	vpc.desc[0] = ve->desc.layout;
	vpc.cba.blendEnable = VK_FALSE;
	vkhelper2(pipeline_build)(&ve->ppll[idx], &ve->ppl[idx],
		&vpc, ve->rp_edit, device, 0);
	vkhelper2(pipeline_config_deinit)(&vpc, device);
}


static void init_desc(Vwdedit() *ve, VkDevice device) {
	Vkhelper2(DescConfig) conf;
	vkhelper2(desc_config)(&conf, 2);
	vkhelper2(desc_config_image)(&conf, 0);
	vkhelper2(desc_config_image)(&conf, 1);
	vkhelper2(desc_build)(&ve->desc, &conf, device);
	vkhelper2(desc_config_deinit)(&conf);
}

static void write_desc(Vwdedit() *ve, VkDevice device) {
	VkDescriptorImageInfo il, ip;
	VkWriteDescriptorSet w[2];
	vkhelper2(desc_write_image)(&w[0], &il, ve->desc.set,
		ve->layer.imageview, ve->sampler, 0);
	vkhelper2(desc_write_image)(&w[1], &ip, ve->desc.set,
		ve->paint.imageview, ve->sampler, 1);
	vkUpdateDescriptorSets(device, 2, w, 0, NULL);
}

void vwdedit(init)(Vwdedit() *ve, VkDevice device) {
	ve->pidx = 0;
	Vkhelper2(RenderpassConfig) renderpass_conf;
	vkhelper2(renderpass_config_offscreen)(&renderpass_conf);
	vkhelper2(renderpass_build)(&ve->rp_edit, &renderpass_conf, device);
	ve->sampler = vkhelper2(sampler)(device);
	init_desc(ve, device);
	ve->ppl = malloc(ppl_count * sizeof(VkPipeline));
	ve->ppll = malloc(ppl_count * sizeof(VkPipelineLayout));
	init_pipeline_edit(ve, device, 0, "pen");
	init_pipeline_edit(ve, device, 1, "eraser");
	ve->first_setup = true;
}

static void vwdedit(reset)(Vwdedit() *ve, VkDevice device) {
	vkhelper2(image_deinit)(&ve->layer, device);
	vkhelper2(image_deinit)(&ve->paint, device);
	vkhelper2(buffer_deinit)(&ve->paint_buffer, device);
	vkhelper2(buffer_deinit)(&ve->layer_buffer, device);
	vkDestroyFramebuffer(device, ve->fb_focus, NULL);
}

void vwdedit(deinit)(Vwdedit() *ve, VkDevice device) {
	vwdedit(reset)(ve, device);
	vkDestroySampler(device, ve->sampler, NULL);
	vkhelper2(desc_deinit)(&ve->desc, device);
	for (size_t i = 0; i < ppl_count; i += 1) {
		vkDestroyPipeline(device, ve->ppl[i], NULL);
		vkDestroyPipelineLayout(device, ve->ppll[i], NULL);
	}
	free(ve->ppl);
	free(ve->ppll);
	vkDestroyRenderPass(device, ve->rp_edit, NULL);
}

void vwdedit(setup)(Vwdedit() *ve, Vkstatic() *vks,
	Vkhelper2(Image) *img, void **p_paint, void **p_layer) {
	// 1.deinit
	if (!ve->first_setup) {
		vwdedit(reset)(ve, vks->device);
	} else {
		ve->first_setup = false;
	}

	uint32_t w = img->size[0];
	uint32_t h = img->size[1];
	vkhelper2(image_new_color)(
		&ve->layer, vks->device, vks->memprop, w, h, false,
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	vkhelper2(image_new_color)(
		&ve->paint, vks->device, vks->memprop, w, h, false,
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VkCommandBuffer cbuf = vkstatic(oneshot_begin)(vks);
	vkhelper2(barrier_shader)(cbuf, &ve->layer);
	vkhelper2(barrier_shader)(cbuf, &ve->paint);
	vkstatic(oneshot_end)(cbuf, vks);
	write_desc(ve, vks->device);
	vkhelper2(buffer_init_cpu)(&ve->paint_buffer, w * h * 4,
		vks->device, vks->memprop);
	vkhelper2(buffer_init_cpu)(&ve->layer_buffer, w * h * 4,
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

void vwdedit(blend)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *dmg) {
	if (dmgrect(is_empty)(dmg)) { return; }
	uint32_t width = ve->paint.size[0];
	uint32_t height = ve->paint.size[1];
	vkhelper2(dynstate_vs)(cbuf, width, height);
	vkhelper2(renderpass_begin)(cbuf, ve->rp_edit, ve->fb_focus,
		width, height);
	vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ve->ppl[ve->pidx]);
	vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ve->ppll[ve->pidx], 0, 1, &ve->desc.set, 0, NULL);
	vkCmdDraw(cbuf, 6, 1, 0, 0);
	vkCmdEndRenderPass(cbuf);
}
