#ifndef _NIX_TEXTURE_H
#define _NIX_TEXTURE_H

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include <vulkan/vulkan.h>

class Image;

namespace vulkan {

	class Texture {
	public:
		Texture();
		~Texture();

		void __init__();
		void __delete__();

		void _load(const string &filename);
		void override(const Image *image);
		void overridex(const void *image, int nx, int ny, int nz);

		void _generate_mipmaps(VkFormat image_format);
		void _create_image(const void *data, int nx, int ny, int nz);
		void _create_view();
		void _create_sampler();


		int width, height, depth;
		int mip_levels;
		VkImage image;
		VkDeviceMemory memory;

		VkImageView view;
		VkSampler sampler;

		static Texture* load(const string &filename);
	};
};

#endif

#endif
