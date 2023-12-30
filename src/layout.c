#include "../include/vwdedit.h"

void vwdedit(copy)(VkCommandBuffer cbuf, Dmgrect() *rect,
	Vkhelper2(Image) *src, Vkhelper2(Image) *dst
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
	vkhelper2(barrier_src)(cbuf, src);
	vkhelper2(barrier_dst)(cbuf, dst);
	vkCmdCopyImage(cbuf,
		src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &icopy);
}

void vwdedit(download_layer)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *rect) {
	Vkhelper2(Image) *src = &ve->layer;
	VkBuffer dst = ve->layer_buffer.buffer;

	Dmgrect() window = {0};
	window.size[0] = src->size[0];
	window.size[1] = src->size[1];
	dmgrect(intersection)(&window, rect);
	if (dmgrect(is_empty)(&window)) { return; }

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
	vkhelper2(barrier_src)(cbuf, src);
	vkCmdCopyImageToBuffer(cbuf,
		src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst,
		1, &icopy);
}

static void buffer_image_copy(VkCommandBuffer cbuf,
	VkBuffer src, Vkhelper2(Image) *dst, Dmgrect() *rect
) {
	Dmgrect() window = {0};
	window.size[0] = dst->size[0];
	window.size[1] = dst->size[1];
	dmgrect(intersection)(&window, rect);
	if (dmgrect(is_empty)(&window)) { return; }

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
	vkhelper2(barrier)(cbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		dst);
	vkCmdCopyBufferToImage(cbuf,
		src, dst->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &icopy);
	vkhelper2(barrier)(cbuf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		dst);
}

void vwdedit(upload_paint)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *rect) {
	VkBuffer src = ve->paint_buffer.buffer;
	Vkhelper2(Image) *dst = &ve->paint;
	buffer_image_copy(cbuf, src, dst, rect);
}
void vwdedit(upload_layer)(Vwdedit() *ve, VkCommandBuffer cbuf, Dmgrect() *rect) {
	VkBuffer src = ve->layer_buffer.buffer;
	Vkhelper2(Image) *dst = &ve->layer;
	buffer_image_copy(cbuf, src, dst, rect);
}

void vwdedit(download_layout_layer)(
	Vwdedit() *ve, VkCommandBuffer cbuf, Vkhelper2(Image) *src
) {
	Dmgrect() rect = { .size = {
		src->size[0],
		src->size[1],
	}};
	vwdedit(copy)(cbuf, &rect, src, &ve->layer);
	vkhelper2(barrier_shader)(cbuf, src);
	vkhelper2(barrier_shader)(cbuf, &ve->layer);
}
