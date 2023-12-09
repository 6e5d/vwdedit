#include <vulkan/vulkan.h>

#include "../../vkhelper2/include/vkhelper2.h"
#include "../include/vwdedit.h"

void vwdedit_copy(VkCommandBuffer cbuf, Dmgrect *rect,
	Vkhelper2Image *src, Vkhelper2Image *dst
) {
	VkImageSubresourceLayers src_layer = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkImageSubresourceLayers dst_layer = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkOffset3D offset = {rect->offset[0], rect->offset[1], 0};
	VkExtent3D extent = {rect->size[0], rect->size[1], 1};
	VkImageCopy icopy = {
		.srcSubresource = src_layer,
		.dstSubresource = dst_layer,
		.srcOffset = offset,
		.dstOffset = offset,
		.extent = extent,
	};
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		src);
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		dst);
	vkCmdCopyImage(cbuf,
		src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &icopy);
}

// for patch generation
void vwdedit_download_layer(Vwdedit *ve, VkCommandBuffer cbuf) {
	Vkhelper2Image *src = &ve->layer;
	VkBuffer dst = ve->layer_buffer.buffer;
	Dmgrect *rect = &ve->dmg_paint;

	Dmgrect window = {0};
	window.size[0] = src->size[0];
	window.size[1] = src->size[1];
	dmgrect_intersection(&window, rect);
	if (dmgrect_is_empty(&window)) { return; }

	VkImageSubresourceLayers layers = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkOffset3D offset = {window.offset[0], window.offset[1], 0};
	VkExtent3D extent = {window.size[0], window.size[1], 1};
	VkDeviceSize buffer_offset =
		4 * ((VkDeviceSize)window.offset[0] +
		(VkDeviceSize)window.offset[1] * src->size[0]);
	VkBufferImageCopy icopy = {
		.bufferOffset = buffer_offset,
		.bufferRowLength = src->size[0],
		.bufferImageHeight = src->size[1],
		.imageSubresource = layers,
		.imageOffset = offset,
		.imageExtent = extent,
	};
	vkhelper2_barrier_src(cbuf, src);
	vkCmdCopyImageToBuffer(cbuf,
		src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst,
		1, &icopy);
}

static void vwdedit_upload(VkCommandBuffer cbuf,
	VkBuffer src, Vkhelper2Image *dst, Dmgrect *rect
) {
	Dmgrect window = {0};
	window.size[0] = dst->size[0];
	window.size[1] = dst->size[1];
	dmgrect_intersection(&window, rect);
	if (dmgrect_is_empty(&window)) { return; }

	VkImageSubresourceLayers layers = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.layerCount = 1,
	};
	VkOffset3D offset = {window.offset[0], window.offset[1], 0};
	VkExtent3D extent = {window.size[0], window.size[1], 1};
	VkDeviceSize buffer_offset =
		4 * ((VkDeviceSize)window.offset[0] +
		(VkDeviceSize)window.offset[1] * dst->size[0]);
	VkBufferImageCopy icopy = {
		.bufferOffset = buffer_offset,
		.bufferRowLength = dst->size[0],
		.bufferImageHeight = dst->size[1],
		.imageSubresource = layers,
		.imageOffset = offset,
		.imageExtent = extent,
	};
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		dst);
	vkCmdCopyBufferToImage(cbuf,
		src, dst->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &icopy);
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		dst);
}

// all pixel copy, 1 image 1 layer, do image barrier before and after
// the target is used as sampler
void vwdedit_upload_draw(Vwdedit *ve, VkCommandBuffer cbuf) {
	VkBuffer src = ve->paint_buffer.buffer;
	Vkhelper2Image *dst = &ve->paint;
	Dmgrect *rect = &ve->dmg_paint;
	vwdedit_upload(cbuf, src, dst, rect);
}

void vwdedit_upload_undo(Vwdedit *ve, VkCommandBuffer cbuf, Dmgrect *rect) {
	VkBuffer src = ve->layer_buffer.buffer;
	Vkhelper2Image *dst = &ve->layer;
	vwdedit_upload(cbuf, src, dst, rect);
}

void vwdedit_download_layout_layer(
	Vwdedit *ve, VkCommandBuffer cbuf, Vkhelper2Image *src
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
		src);
	vkhelper2_barrier(cbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		&ve->layer);
	vkCmdCopyImage(cbuf,
		src->image,
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
		src);
}
