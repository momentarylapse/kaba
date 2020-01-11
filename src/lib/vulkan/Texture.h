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
		Texture(int w, int h);
		~Texture();

		void __init__();
		void __delete__();

		void _load(const string &filename);
		void override(const Image *image);
		void overridex(const void *image, int nx, int ny, int nz, const string &format);

		void _destroy();
		void _generate_mipmaps(VkFormat image_format);
		void _create_image(const void *data, int nx, int ny, int nz, VkFormat image_format, bool allow_mip);
		void _create_view();
		void _create_sampler();


		int width, height, depth;
		int mip_levels;
		VkFormat format;
		VkImage image;
		VkDeviceMemory memory;

		VkImageView view;
		VkSampler sampler;
		VkCompareOp compare_op;

		static string directory;
		static Texture* load(const string &filename);
	};

	class DynamicTexture : public Texture {
	public:
		DynamicTexture(int nx, int ny, int nz, const string &format);
		void __init__(int nx, int ny, int nz, const string &format);
	};

	extern Array<Texture*> textures;
};

#endif

#endif
