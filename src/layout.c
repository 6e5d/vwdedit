#include <vulkan/vulkan.h>

#include "../../vkhelper/include/barrier.h"
#include "../include/layout.h"
#include "../include/vwdedit.h"

void vwdedit_download_layout_layer(
	Vwdedit *ve, VkCommandBuffer cbuf, VkImage image
) {
	VkImageSubresourceLayers layers = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkOffset3D offset = {0, 0, 0};
	VkExtent3D extent = {ve->layer.size[0], ve->layer.size[1], 1};
	VkImageCopy icopy = {
		.srcSubresource = layers,
		.dstSubresource = layers,
		.srcOffset = offset,
		.dstOffset = offset,
		.extent = extent,
	};
	vkhelper_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		image);
	vkhelper_barrier(cbuf,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		ve->layer.image);
	vkCmdCopyImage(cbuf,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		ve->layer.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &icopy);
	vkhelper_barrier(cbuf,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		ve->layer.image);
	vkhelper_barrier(cbuf,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		image);
}
