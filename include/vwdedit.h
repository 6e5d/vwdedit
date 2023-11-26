#ifndef INCLUDEGUARD_VWDEDIT_VWDEDITH
#define INCLUDEGUARD_VWDEDIT_VWDEDITH

#include <stdbool.h>
#include <vulkan/vulkan.h>

#include "../../dmgrect/include/dmgrect.h"
#include "../../vkhelper/include/buffer.h"
#include "../../vkhelper/include/desc.h"
#include "../../vkhelper/include/image.h"
#include "../../vkstatic/include/vkstatic.h"

typedef struct {
VkRenderPass rp_edit;
VkPipeline ppl_edit;
VkPipelineLayout ppll_edit;
VkhelperDesc desc;
VkSampler sampler;

Dmgrect dmg_paint;
bool first_setup;
// objects that are destroyed/recreated between focus
VkhelperImage layer;
VkhelperImage paint;
VkhelperBuffer paint_buffer;
VkFramebuffer fb_focus;
} Vwdedit;

void vwdedit_damage_all(Vwdedit *ve);
void vwdedit_init(Vwdedit *ve, VkDevice device);
void vwdedit_setup(Vwdedit *ve, Vkstatic *vks, VkhelperImage *img, void **p);
void vwdedit_deinit(Vwdedit *ve, VkDevice device);
void vwdedit_build_command_upload(Vwdedit *ve,
VkDevice device,
VkCommandBuffer cbuf);
void vwdedit_build_command(Vwdedit *ve, VkDevice device, VkCommandBuffer cbuf);

#endif
