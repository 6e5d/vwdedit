#ifndef INCLUDEGUARD_VWDEDIT
#define INCLUDEGUARD_VWDEDIT

#include <vulkan/vulkan.h>

#include "../../dmgrect/include/dmgrect.h"
#include "../../vkhelper2/include/vkhelper2.h"
#include "../../vkstatic/include/vkstatic.h"

typedef struct {
	VkRenderPass rp_edit;
	// 0: pen, 1: eraser
	size_t pidx;
	VkPipeline *ppl;
	VkPipelineLayout *ppll;
	Vkhelper2Desc desc;
	VkSampler sampler;
	bool first_setup;
	// objects that are destroyed/recreated between focus
	Vkhelper2Image paint;
	Vkhelper2Image layer;
	Vkhelper2Buffer paint_buffer;
	Vkhelper2Buffer layer_buffer;
	VkFramebuffer fb_focus;
} Vwdedit;

void vwdedit_init(Vwdedit *ve, VkDevice device);
void vwdedit_setup(Vwdedit *ve, Vkstatic *vks,
	Vkhelper2Image *img, void **p_paint, void **p_layer);
void vwdedit_deinit(Vwdedit *ve, VkDevice device);
void vwdedit_blend(Vwdedit *ve, VkCommandBuffer cbuf, Dmgrect *dmg);
void vwdedit_download_layout_layer(
	Vwdedit *ve, VkCommandBuffer cbuf, Vkhelper2Image *src
);
void vwdedit_download_layer(Vwdedit *ve, VkCommandBuffer cbuf, Dmgrect *rect);
void vwdedit_upload_paint(Vwdedit *ve, VkCommandBuffer cbuf, Dmgrect *rect);
void vwdedit_upload_layer(Vwdedit *ve, VkCommandBuffer cbuf, Dmgrect *rect);
void vwdedit_copy(VkCommandBuffer cbuf, Dmgrect *rect,
	Vkhelper2Image *src, Vkhelper2Image *dst);

#endif
