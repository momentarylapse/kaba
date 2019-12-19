#ifndef _NIX_SHADER_H
#define _NIX_SHADER_H

#include "../base/base.h"
#include <vulkan/vulkan.h>
#include "../math/matrix.h"
#include <vector>

namespace vulkan{

	class Texture;

	class UBOWrapper {
	public:
		UBOWrapper(int size);
		~UBOWrapper();
		
		void __init__(int size);
		void __delete__();
		
		void update(void *source);

		int size;
		std::vector<VkBuffer> buffers;
		std::vector<VkDeviceMemory> memory;
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

		static Shader* load(const string &filename);
	};



	extern VkDescriptorPool descriptor_pool;

	VkDescriptorPool create_descriptor_pool();
	void destroy_descriptor_pool(VkDescriptorPool pool);

	class DescriptorSet {
	public:
		DescriptorSet(VkDescriptorSetLayout layout, const Array<UBOWrapper*> &ubos, const Array<Texture*> &tex);
		//~DescriptorSet();

		void __init__(VkDescriptorSetLayout layout, const Array<UBOWrapper*> &ubos, const Array<Texture*> &tex);
		//void __delete__();

		void set(const Array<UBOWrapper*> &ubos, const Array<Texture*> &tex);

		std::vector<VkDescriptorSet> descriptor_sets;


		static VkDescriptorSetLayout create_layout(int num_ubos, int num_samplers);
		static void destroy_layout(VkDescriptorSetLayout layout);
	};
};

#endif
