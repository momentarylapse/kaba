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
	class AccelerationStructure;

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
		void set_storage_image(int binding, Texture *t);
		void set_acceleration_structure(int binding, AccelerationStructure *a);

		void update();

		VkDescriptorSetLayout layout;
		VkDescriptorSet descriptor_set;
		struct BufferData {
			VkDescriptorBufferInfo info;
			int binding;
			VkDescriptorType type;
		};
		struct ImageData {
			VkDescriptorImageInfo info;
			int binding;
			VkDescriptorType type;
		};
		struct AccelerationData {
			VkWriteDescriptorSetAccelerationStructureNV info;
			int binding;
			VkDescriptorType type;
		};
		Array<BufferData> buffers;
		Array<ImageData> images;
		Array<AccelerationData> accelerations;
		int num_dynamic_ubos;

		static Array<VkDescriptorSetLayout> parse_bindings(const string &bindings);
		static void digest_bindings(const string &bindings, Array<VkDescriptorType> &types, Array<int> &binding_no);
		static VkDescriptorSetLayout create_layout(const Array<VkDescriptorType> &types, const Array<int> &bindings);
		static void destroy_layout(VkDescriptorSetLayout layout);
	};
};

#endif
