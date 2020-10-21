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
	class DescriptorSet;

	class DescriptorPool {
	public:
		DescriptorPool(const string &s, int max_sets);
		~DescriptorPool();

		void __init__(const string &s, int max_sets);
		void __delete__();

		DescriptorSet *create_set(const string &s);

		VkDescriptorPool pool;
	};

	class DescriptorSet {
		friend class DescriptorPool;
		DescriptorSet(DescriptorPool *pool, const string &s);
	public:
		~DescriptorSet();
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
