#if HAS_LIB_VULKAN

#include "Texture.h"
#include <vulkan/vulkan.h>

#include <cmath>
#include <iostream>

#include "helper.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "Device.h"
#include "../image/image.h"

namespace vulkan {

extern bool verbose;

Array<Texture*> textures;

VkCompareOp next_compare_op = VK_COMPARE_OP_ALWAYS;

VkFormat parse_format(const string &s) {
	if (s == "rgba:i8")
		return VK_FORMAT_R8G8B8A8_UNORM;
	if (s == "bgra:i8")
		return VK_FORMAT_B8G8R8A8_UNORM;
	if (s == "rgb:i8")
		return VK_FORMAT_R8G8B8_UNORM;
	if (s == "r:i8")
		return VK_FORMAT_R8_UNORM;
	if (s == "argb:i10")
		return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
	if (s == "rgba:f32")
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	if (s == "rgb:f32")
		return VK_FORMAT_R32G32B32_SFLOAT;
	if (s == "rgba:f16")
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	if (s == "rgb:f16")
		return VK_FORMAT_R16G16B16_SFLOAT;
	if (s == "r:f32")
		return VK_FORMAT_R32_SFLOAT;
	if (s == "d:f32")
		return VK_FORMAT_D32_SFLOAT;
	if (s == "d:i16")
		return VK_FORMAT_D16_UNORM;
	if (s == "ds:u24i8")
		return VK_FORMAT_D24_UNORM_S8_UINT;
	if (s == "ds:f32i8")
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	throw Exception("unknown image format: " + s);
	return VK_FORMAT_R8G8B8A8_UNORM;
}

int format_size(VkFormat f) {
	// i8
	if (f == VK_FORMAT_R8G8B8A8_UNORM)
		return 4;
	if (f == VK_FORMAT_R8G8B8_UNORM)
		return 3;
	if (f == VK_FORMAT_R8_UNORM)
		return 1;
	// weird
	if (f == VK_FORMAT_A2R10G10B10_SNORM_PACK32)
		return 4;
	// f32
	if (f == VK_FORMAT_R32G32B32A32_SFLOAT)
		return 16;
	if (f == VK_FORMAT_R32G32B32_SFLOAT)
		return 12;
	if (f == VK_FORMAT_R32G32_SFLOAT)
		return 8;
	if (f == VK_FORMAT_R32_SFLOAT)
		return 4;
	// i32
	if (f == VK_FORMAT_R32G32B32A32_SINT)
		return 16;
	if (f == VK_FORMAT_R32G32B32_SINT)
		return 12;
	if (f == VK_FORMAT_R32G32_SINT)
		return 8;
	if (f == VK_FORMAT_R32_SINT)
		return 4;
	// f16
	if (f == VK_FORMAT_R16G16B16A16_SFLOAT)
		return 8;
	if (f == VK_FORMAT_R16G16B16_SFLOAT)
		return 6;
	if (f == VK_FORMAT_R16G16_SFLOAT)
		return 4;
	if (f == VK_FORMAT_R16_SFLOAT)
		return 4;
	// depth
	if (f == VK_FORMAT_D32_SFLOAT)
		return 4;
	if (f == VK_FORMAT_D16_UNORM)
		return 2;
	if (f == VK_FORMAT_D32_SFLOAT_S8_UINT)
		return 5; // ?
	return 4;
}

Texture::Texture() {
	image = nullptr;
	memory = nullptr;
	sampler = nullptr;
	view = nullptr;
	width = height = depth = 0;
	mip_levels = 0;
	format = VK_FORMAT_UNDEFINED;
	compare_op = next_compare_op;
	magfilter = VK_FILTER_LINEAR;
	minfilter = VK_FILTER_LINEAR;
	address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	textures.add(this);
}

Texture::Texture(int w, int h) : Texture() {
	// sometimes a newly created texture is already used....
	Image im;
	im.create(w, h, White);
	override(im);
}

Texture::~Texture() {
	_destroy();

	for (int i=0; i<textures.num; i++)
		if (textures[i] == this)
			textures.erase(i);
}

void Texture::__init__() {
	new(this) Texture();
}

void Texture::__delete__() {
	this->~Texture();
}

DynamicTexture::DynamicTexture(int nx, int ny, int nz, const string &_format) {
	width = nx;
	height = ny;
	depth = nz;
	format = parse_format(_format);
	_create_image(nullptr, nx, ny, nz, format, false, true);
	_create_view();
	_create_sampler();
}

void DynamicTexture::__init__(int nx, int ny, int nz, const string &format) {
	new(this) DynamicTexture(nx, ny, nz, format);
}

StorageTexture::StorageTexture(int nx, int ny, int nz, const string &_format) {
	width = nx;
	height = ny;
	depth = nz;
	format = parse_format(_format);
	int ps = format_size(format);
	VkDeviceSize image_size = width * height * depth * ps;
	mip_levels = 1;

	VkExtent3D extent = {(unsigned)nx, (unsigned)ny, (unsigned)nz};



	VkImageCreateInfo imageCreateInfo;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = extent;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	auto result = vkCreateImage(default_device->device, &imageCreateInfo, nullptr, &image);
	if (VK_SUCCESS != result)
		throw Exception("vkCreateImage failed");
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(default_device->device, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = default_device->find_memory_type(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkAllocateMemory(default_device->device, &memoryAllocateInfo, nullptr, &memory);
	if (VK_SUCCESS != result)
		throw Exception("vkAllocateMemory failed");
	result = vkBindImageMemory(default_device->device, image, memory, 0);
	if (VK_SUCCESS != result)
		throw Exception("vkBindImageMemory failed");
	if (verbose)
		std::cout << "  storage image ok\n";

	_create_view();
	//_create_sampler();
}

void StorageTexture::__init__(int nx, int ny, int nz, const string &format) {
	new(this) StorageTexture(nx, ny, nz, format);
}

void Texture::_destroy() {
	if (sampler)
		vkDestroySampler(default_device->device, sampler, nullptr);
	if (view)
		vkDestroyImageView(default_device->device, view, nullptr);
	if (image)
		vkDestroyImage(default_device->device, image, nullptr);
	if (memory)
		vkFreeMemory(default_device->device, memory, nullptr);
	sampler = nullptr;
	view = nullptr;
	image = nullptr;
	memory = nullptr;
	width = height = depth = 0;
	mip_levels = 0;
}

Texture* Texture::load(const Path &filename) {
	if (verbose)
		std::cout << " load texture " << filename.str().c_str() << "\n";
	if (filename.is_empty())
		return new Texture(16, 16);
	Texture *t = new Texture();
	t->_load(filename);
	return t;
}


void Texture::_load(const Path &filename) {
	Image *im = Image::load(filename);
	if (!im) {
		throw Exception("failed to load texture image!");
	}
	override(*im);
	delete im;
}

void Texture::override(const Image &im) {
	overridex(im.data.data, im.width, im.height, 1, "rgba:i8");
}

void Texture::overridex(const void *data, int nx, int ny, int nz, const string &format) {
	_destroy();
	_create_image(data, nx, ny, nz, parse_format(format), depth == 1, false);
	_create_view();
	_create_sampler();
}

void Texture::_create_image(const void *image_data, int nx, int ny, int nz, VkFormat image_format, bool allow_mip, bool allow_storage) {
	width = nx;
	height = ny;
	depth = nz;
	format = image_format;
	int ps = format_size(image_format);
	VkDeviceSize image_size = width * height * depth * ps;
	mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	if (!allow_mip)
		mip_levels = 1;


	Buffer staging;
	if (image_data) {
		staging.create(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		staging.update_part(image_data, 0, image_size);
	}

	auto usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (allow_storage)
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	auto tiling = VK_IMAGE_TILING_OPTIMAL;
	create_image(width, height, depth, mip_levels, format, tiling, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

	transition_image_layout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);

	if (image_data) {
		copy_buffer_to_image(staging.buffer, image, width, height, depth);
	}

	auto layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//if (allow_storage)
	//	layout = VK_IMAGE_LAYOUT_GENERAL;

	if (allow_mip)
		_generate_mipmaps(format);
	else
		transition_image_layout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, mip_levels);
}

void Texture::_generate_mipmaps(VkFormat image_format) {
	// Check if image format supports linear blitting
	VkFormatProperties fp;
	vkGetPhysicalDeviceFormatProperties(default_device->physical_device, image_format, &fp);

	if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw Exception("texture image format does not support linear blitting!");
	}

	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mip_width = width;
	int32_t mip_height = height;

	for (int i=1; i<mip_levels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mip_width, mip_height, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(command_buffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mip_width > 1) mip_width /= 2;
		if (mip_height > 1) mip_height /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	end_single_time_commands(command_buffer);
}



void Texture::_create_view() const {
	VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
	if (depth > 1)
		type = VK_IMAGE_VIEW_TYPE_3D;
	view = create_image_view(image, format, VK_IMAGE_ASPECT_COLOR_BIT, type, mip_levels);
}

void Texture::_create_sampler() const {
	VkSamplerCreateInfo si = {};
	si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	si.magFilter = magfilter;
	si.minFilter = minfilter;
	si.addressModeU = address_mode;
	si.addressModeV = address_mode;
	si.addressModeW = address_mode;
	si.anisotropyEnable = VK_TRUE;
	si.maxAnisotropy = 16;
	si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	si.unnormalizedCoordinates = VK_FALSE;
	si.compareEnable = (compare_op == VK_COMPARE_OP_ALWAYS) ? VK_FALSE : VK_TRUE;
	si.compareOp = compare_op;
	si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	si.minLod = 0;
	si.maxLod = static_cast<float>(mip_levels);
	si.mipLodBias = 0;

	if (vkCreateSampler(default_device->device, &si, nullptr, &sampler) != VK_SUCCESS) {
		throw Exception("failed to create texture sampler!");
	}
}

// hmmm, mag/minfilter doesn't seem to do much...
void Texture::set_options(const string &options) const {
	for (auto &x: options.explode(",")) {
		auto y = x.explode("=");
		if (y.num != 2)
			throw Exception("key=value expected: " + x);
		string key = y[0];
		string value = y[1];
		if (key == "wrap") {
			if (value == "repeat") {
				address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			} else if (value == "clamp") {
				address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			} else {
				throw Exception("unknown value for key: " + x);
			}
		} else if (key == "magfilter") {
			if (value == "linear") {
				magfilter = VK_FILTER_LINEAR;
			} else if (value == "nearest") {
				magfilter = VK_FILTER_NEAREST;
			} else if (value == "cubic") {
				magfilter = VK_FILTER_CUBIC_IMG;
			} else {
				throw Exception("unknown value for key: " + x);
			}
		} else if (key == "minfilter") {
			if (value == "linear") {
				minfilter = VK_FILTER_LINEAR;
			} else if (value == "nearest") {
				minfilter = VK_FILTER_NEAREST;
			} else if (value == "cubic") {
				minfilter = VK_FILTER_CUBIC_IMG;
			} else {
				throw Exception("unknown value for key: " + x);
			}
		} else {
			throw Exception("unknown key: " + key);
		}
	}
	if (sampler)
		vkDestroySampler(default_device->device, sampler, nullptr);
	sampler = nullptr;
	_create_sampler();
}


};

#endif
