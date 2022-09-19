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
#include "../os/msg.h"

namespace vulkan {

extern bool verbose;

Array<Texture*> textures;

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
	type = Type::DEFAULT;
	sampler = nullptr;
	view = nullptr;
	width = height = 0;
	depth = 1;
	mip_levels = 1;
	compare_op = VK_COMPARE_OP_ALWAYS;
	magfilter = VK_FILTER_LINEAR;
	minfilter = VK_FILTER_LINEAR;
	address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	textures.add(this);
}

Texture::Texture(int w, int h, const string &format) : Texture() {
	// sometimes a newly created texture is already used....
	/*Image im;
	im.create(w, h, White);
	override(im);*/
	width = w;
	height = h;
	depth = 1;
	_create_image(nullptr, VK_IMAGE_TYPE_2D, parse_format(format), false, false, false);
	view = image.create_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, mip_levels, 0, 1);
	_create_sampler();
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

void Texture::__init_ext__(int w, int h, const string &format) {
	new(this) Texture(w, h, format);
}

void Texture::__delete__() {
	this->~Texture();
}

VolumeTexture::VolumeTexture(int nx, int ny, int nz, const string &format) {
	type = Type::VOLUME;
	width = nx;
	height = ny;
	depth = nz;
	_create_image(nullptr, VK_IMAGE_TYPE_3D, parse_format(format), false, false, false);
	view = image.create_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_3D, mip_levels, 0, 1);
	_create_sampler();
}

void VolumeTexture::__init__(int nx, int ny, int nz, const string &format) {
	new(this) VolumeTexture(nx, ny, nz, format);
}

StorageTexture::StorageTexture(int nx, int ny, int nz, const string &_format) {
	width = nx;
	height = ny;
	depth = nz;
	image.format = parse_format(_format);
	int ps = format_size(image.format);
	VkDeviceSize image_size = width * height * depth * ps;
	mip_levels = 1;

	VkExtent3D extent = {(unsigned)nx, (unsigned)ny, (unsigned)nz};



	VkImageCreateInfo imageCreateInfo;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
	imageCreateInfo.format = image.format;
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

	auto result = vkCreateImage(default_device->device, &imageCreateInfo, nullptr, &image.image);
	if (VK_SUCCESS != result)
		throw Exception("vkCreateImage failed");
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(default_device->device, image.image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = default_device->find_memory_type(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkAllocateMemory(default_device->device, &memoryAllocateInfo, nullptr, &image.memory);
	if (VK_SUCCESS != result)
		throw Exception("vkAllocateMemory failed");
	result = vkBindImageMemory(default_device->device, image.image, image.memory, 0);
	if (VK_SUCCESS != result)
		throw Exception("vkBindImageMemory failed");
	if (verbose)
		std::cout << "  storage image ok\n";

	view = image.create_view(VK_IMAGE_ASPECT_COLOR_BIT, depth == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D, mip_levels, 0, 1);
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
	image._destroy();
	sampler = nullptr;
	view = nullptr;
	width = height = depth = 0;
	mip_levels = 1;
}

Texture* Texture::load(const Path &filename) {
	if (verbose)
		std::cout << " load texture " << filename.str().c_str() << "\n";
	if (filename.is_empty())
		return new Texture(16, 16, "rgba:i8");
	Texture *t = new Texture();
	t->_load(filename);
	return t;
}


void Texture::_load(const Path &filename) {
	Image *im = Image::load(filename);
	if (!im) {
		throw Exception("failed to load texture image!");
	}
	write(*im);
	delete im;
}

void Texture::write(const Image &im) {
	writex(im.data.data, im.width, im.height, 1, "rgba:i8");
}

void Texture::writex(const void *data, int nx, int ny, int nz, const string &format) {
	_destroy();
	width = nx;
	height = ny;
	depth = nz;

	_create_image(data, depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D, parse_format(format), depth == 1, false, false);
	view = image.create_view(VK_IMAGE_ASPECT_COLOR_BIT, depth == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D, mip_levels, 0, 1);
	_create_sampler();
}

void Texture::_create_image(const void *image_data, VkImageType type, VkFormat format, bool allow_mip, bool allow_storage, bool cube) {
	int num_layers = cube ? 6 : 1;
	int layer_size = width * height * depth * format_size(format);
	VkDeviceSize image_size = layer_size * num_layers;
	mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	if (!allow_mip)
		mip_levels = 1;

	auto layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//if (allow_storage)
	//	layout = VK_IMAGE_LAYOUT_GENERAL;


	auto usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (allow_storage)
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	image.create(type, width, height, depth, mip_levels, num_layers, format, usage, cube);

	image.transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, 0, num_layers);


	if (image_data) {
		Buffer staging;
		staging.create(layer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		for (int k=0; k<num_layers; k++) {
			staging.update_part((char*)image_data + k * layer_size, 0, layer_size);
			copy_buffer_to_image(staging.buffer, image.image, width, height, depth, 0, k);
		}
	}


	if (allow_mip)
		image.generate_mipmaps(width, height, mip_levels, 0, num_layers, layout);
	else
		image.transition_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, mip_levels, 0, num_layers);

}

void Texture::_create_sampler() const {
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter = magfilter;
	info.minFilter = minfilter;
	info.addressModeU = address_mode;
	info.addressModeV = address_mode;
	info.addressModeW = address_mode;
	info.anisotropyEnable = VK_TRUE;
	info.maxAnisotropy = 16;
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable = (compare_op == VK_COMPARE_OP_ALWAYS) ? VK_FALSE : VK_TRUE;
	info.compareOp = compare_op;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.minLod = 0;
	info.maxLod = static_cast<float>(mip_levels);
	info.mipLodBias = 0;

	if (vkCreateSampler(default_device->device, &info, nullptr, &sampler) != VK_SUCCESS) {
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

CubeMap::CubeMap(int size, const string &format) {
	type = Type::CUBE;
	width = size;
	height = size;
	const unsigned int COL[6] = {0xff0000ff, 0xffff0000, 0xff000080, 0xff00ff00, 0xff00ffff, 0xffff00ff};
	Array<unsigned int> data;
	for (int k=0; k<6; k++)
		for (int i=0; i<size*size; i++)
			data.add(COL[k]);
	_create_image(nullptr, VK_IMAGE_TYPE_2D, parse_format(format), false, false, true);
	view = image.create_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, mip_levels, 0, 6);
	_create_sampler();
}

void CubeMap::__init__(int size, const string &format) {
	new(this) CubeMap(size, format);
}

void CubeMap::write_side(int side, const Image &_image) {
	if (image.format != VK_FORMAT_R8G8B8A8_UNORM) {
		msg_error("CubeMap.write_side(): format is not rgba:i8");
		return;
	}
	if (_image.width != width or _image.height != height) {
		msg_error("CubeMap.write_side(): size mismatch");
		return;
	}
	//overridex();

	auto layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	image.transition_layout(layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, side, 1);

	int layer_size = width * height * 4;


	Buffer staging;
	staging.create(layer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	staging.update_part(&_image.data[0], 0, layer_size);
	copy_buffer_to_image(staging.buffer, image.image, width, height, depth, 0, side);


	/*if (allow_mip)
		image.generate_mipmaps(width, height, mip_levels, 0, num_layers, layout);
	else*/
		image.transition_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, mip_levels, side, 1);
}


};

#endif
