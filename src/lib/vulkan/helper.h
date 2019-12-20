#ifndef _NIX_HELPER_H
#define _NIX_HELPER_H

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace vulkan{

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	bool hasStencilComponent(VkFormat format);


	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();

	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;

	extern VkSurfaceKHR surface;

	extern std::vector<VkImage> swapChainImages;
	extern VkExtent2D swapChainExtent;


	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() ;
	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

};

#endif

#endif
