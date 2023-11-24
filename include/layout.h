#pragma once

#include "../../vkhelper/include/image.h"
#include "../include/vwdedit.h"

void vwdedit_download_layout_layer(
	Vwdedit *ve, VkCommandBuffer cbuf, VkhelperImage src
);
