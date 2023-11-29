#include <vulkan/vulkan.h>

#include "../../vkhelper2/include/vkhelper2.h"
#include "../include/layout.h"
#include "../include/vwdedit.h"

// TODO: a proper vkhelper2_image that handles
// stages, layouts, levels automatically
void vwdedit_download_layout_layer(
	Vwdedit *ve, VkCommandBuffer cbuf, Vkhelper2Image src
) {
	VkImageSubresourceLayers src_layer = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkImageSubresourceLayers dst_layer = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkOffset3D offset = {0, 0, 0};
	VkExtent3D extent = {ve->layer.size[0], ve->layer.size[1], 1};
	VkImageCopy icopy = {
		.srcSubresource = src_layer,
		.dstSubresource = dst_layer,
		.srcOffset = offset,
		.dstOffset = offset,
		.extent = extent,
	};
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		&src);
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		&ve->layer);
	vkCmdCopyImage(cbuf,
		src.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		ve->layer.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &icopy);
	vkhelper2_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		&ve->layer);
	vkhelper2_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		&src);
}
