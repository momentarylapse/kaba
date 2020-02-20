#ifndef _NIX_SHADER_H
#define _NIX_SHADER_H

#if HAS_LIB_VULKAN



#include "../base/base.h"
#include <vulkan/vulkan.h>
#include "../math/matrix.h"
#include <vector>

namespace vulkan{

	class Texture;

	class UniformBuffer {
	public:
		UniformBuffer(int size);
		UniformBuffer(int size, int count);
		~UniformBuffer();

		void __init__(int size);
		void __init_multi__(int size, int count);
		void __delete__();

		void update(void *source);
		void update_part(void *source, int offset, int size);
		void update_single(void *source, int index);

		bool is_dynamic();

		int size, size_single;
		int count, size_single_aligned;
		VkBuffer buffer;
		VkDeviceMemory memory;
	};

	class Shader {
	public:
		Shader();
		~Shader();

		void __init__();
		void __delete__();

		VkShaderModule vert_module;
		VkShaderModule geom_module;
		VkShaderModule frag_module;

		Array<VkDescriptorSetLayout> descr_layouts;
		int push_size;
		VkPrimitiveTopology topology;

		static string directory;
		static Shader* load(const string &filename);
	};



	extern VkDescriptorPool descriptor_pool;

	VkDescriptorPool create_descriptor_pool();
	void destroy_descriptor_pool(VkDescriptorPool pool);

	class DescriptorSet {
	public:
		DescriptorSet(const Array<UniformBuffer*> &ubos, const Array<Texture*> &tex);
		~DescriptorSet();

		void __init__(const Array<UniformBuffer*> &ubos, const Array<Texture*> &tex);
		void __delete__();

		void set(const Array<UniformBuffer*> &ubos, const Array<Texture*> &tex);
		void set_with_offset(const Array<UniformBuffer*> &ubos, const Array<int> &offsets, const Array<Texture*> &tex);

		VkDescriptorSetLayout layout;
		VkDescriptorSet descriptor_set;
		Array<UniformBuffer*> ubos;
		int num_dynamic_ubos;

		static VkDescriptorSetLayout create_layout(const Array<VkDescriptorType> &types);
		static void destroy_layout(VkDescriptorSetLayout layout);
	};
};

#endif

#endif
