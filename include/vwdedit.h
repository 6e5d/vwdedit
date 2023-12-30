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
	Vkhelper2(Desc) desc;
	VkSampler sampler;
	bool first_setup;
	// objects that are destroyed/recreated between focus
	Vkhelper2(Image) paint;
	Vkhelper2(Image) layer;
	Vkhelper2(Buffer) paint_buffer;
	Vkhelper2(Buffer) layer_buffer;
	VkFramebuffer fb_focus;
} Vwdedit();

void vwdedit(init)(Vwdedit() *ve, VkDevice device);
void vwdedit(setup)(Vwdedit() *ve, Vkstatic() *vks,
	Vkhelper2(Image) *img, void **p_paint, void **p_layer);
void vwdedit(deinit)(Vwdedit() *ve, VkDevice device);
void vwdedit(blend)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *dmg);
void vwdedit(download_layout_layer)(
	Vwdedit() *ve, VkCommandBuffer cbuf, Vkhelper2(Image) *src
);
void vwdedit(download_layer)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *rect);
void vwdedit(upload_paint)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *rect);
void vwdedit(upload_layer)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *rect);
void vwdedit(copy)(VkCommandBuffer cbuf, Dmgrect() *rect,
	Vkhelper2(Image) *src, Vkhelper2(Image) *dst);
