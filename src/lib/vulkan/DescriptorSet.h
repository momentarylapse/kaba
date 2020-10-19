/*
 * DescriptorSet.h
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */
#pragma once

#if HAS_LIB_VULKAN



#include "../base/base.h"
#include <vulkan/vulkan.h>
#include "helper.h"

namespace vulkan{

	class Texture;
	class UniformBuffer;

	extern VkDescriptorPool descriptor_pool;

	VkDescriptorPool create_descriptor_pool();
	void destroy_descriptor_pool(VkDescriptorPool pool);

	class DescriptorSet {
	public:
		DescriptorSet(const string &s);
		~DescriptorSet();

		void __init__(const string &s);
		void __delete__();

		void set_ubo(int binding, UniformBuffer *ubo);
		void set_ubo_with_offset(int binding, UniformBuffer *ubo, int offset);
		void set_texture(int binding, Texture *t);

		void update();

		VkDescriptorSetLayout layout;
		VkDescriptorSet descriptor_set;
		struct UboData {
			UniformBuffer* ubo;
			int binding;
			int offset;
		};
		struct TextureData {
			Texture* texture;
			int binding;
		};
		Array<UboData> ubos;
		Array<TextureData> textures;
		int num_dynamic_ubos;

		static Array<VkDescriptorSetLayout> parse_bindings(const string &bindings);
		static void digest_bindings(const string &bindings, Array<VkDescriptorType> &types, Array<int> &binding_no);
		static VkDescriptorSetLayout create_layout(const Array<VkDescriptorType> &types, const Array<int> &bindings);
		static void destroy_layout(VkDescriptorSetLayout layout);
	};
};

#endif
