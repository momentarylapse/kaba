/*
 * DescriptorSet.cpp
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */


#if HAS_LIB_VULKAN


#include "DescriptorSet.h"
#include "AccelerationStructure.h"
#include "Buffer.h"
#include "Device.h"
#include "Shader.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

#include "helper.h"

namespace vulkan {

string result2str(VkResult r);

const string DESCRIPTOR_NAME_ACCELERATION_STRUCTURE = "acceleration-structure";

VkDescriptorType descriptor_type(const string &s) {
	if (s == "uniform" or s == "ubo" or s == "buffer")
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	if (s == "dynamice" or s == "dbuffer")
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	if (s == "storage" or s == "ssbo" or s == "storage-buffer")
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	if (s == "sampler")
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	if (s == "image")
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	if (s == DESCRIPTOR_NAME_ACCELERATION_STRUCTURE)
		return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	throw Exception("unknown type: " + s);
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}


	DescriptorPool::DescriptorPool(const string &s, int max_sets) {
		Array<VkDescriptorPoolSize> pool_sizes;
		for (auto &xx: s.explode(",")) {
			auto y = xx.explode(":");
			pool_sizes.add({descriptor_type(y[0]), (unsigned)y[1]._int()});
		}


		VkDescriptorPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = static_cast<uint32_t>(pool_sizes.num);
		info.pPoolSizes = &pool_sizes[0];
		info.maxSets = max_sets;

		auto r = vkCreateDescriptorPool(default_device->device, &info, nullptr, &pool);
		if (r != VK_SUCCESS)
			throw Exception("failed to create descriptor pool!  " + result2str(r));
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(default_device->device, pool, nullptr);
	}

	DescriptorSet *DescriptorPool::create_set(const string &s) {
		Array<VkDescriptorType> types;
		Array<int> binding_no;
		DescriptorSet::digest_bindings(s, types, binding_no);

		auto layout = DescriptorSet::create_layout(types, binding_no);
		return create_set_from_layout(layout);
	}

	DescriptorSet *DescriptorPool::create_set(Shader *s) {
		return create_set_from_layout(s->descr_layouts[0]);
	}

	DescriptorSet *DescriptorPool::create_set_from_layout(VkDescriptorSetLayout layout) {
		return new DescriptorSet(this, layout);
	}

	Array<DescriptorSet*> descriptor_sets;

	DescriptorSet::DescriptorSet(DescriptorPool *pool, VkDescriptorSetLayout _layout) {
		layout = _layout;

		num_dynamic_ubos = 0;
		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = pool->pool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layout;

		auto r = vkAllocateDescriptorSets(default_device->device, &info, &descriptor_set);
		if (r != VK_SUCCESS)
			throw Exception("failed to allocate descriptor sets!  " + result2str(r));
	}
	DescriptorSet::~DescriptorSet() {
		// no VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT...
		//vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
		destroy_layout(layout);
	}

	template<class T>
	T &get_for_binding(Array<T> &array, int binding, VkDescriptorType type) {
		for (auto &x: array)
			if (x.binding == binding)
				return x;
		array.add({{}, binding, type});
		return array.back();
	}

	void DescriptorSet::set_texture(int binding, Texture *t) {
		auto &i = get_for_binding(images, binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		i.info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		i.info.imageView = t->view;
		i.info.sampler = t->sampler;
	}

	void DescriptorSet::set_storage_image(int binding, Texture *t) {
		auto &i = get_for_binding(images, binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		i.info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		i.info.imageView = t->view;
		i.info.sampler = VK_NULL_HANDLE; //t->sampler;
	}

	void DescriptorSet::set_uniform_buffer_with_offset(int binding, Buffer *u, int offset) {
		auto type = /*u->is_dynamic() ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :*/ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		auto &i = get_for_binding(buffers, binding, type);
		i.info.buffer = u->buffer;
		i.info.offset = offset;
		i.info.range = u->size;
	}

	void DescriptorSet::set_storage_buffer(int binding, Buffer *u) {
		auto type = /*u->is_dynamic() ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :*/ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		auto &i = get_for_binding(buffers, binding, type);
		i.info.buffer = u->buffer;
		i.info.offset = 0;
		i.info.range = u->size;
	}

	void DescriptorSet::set_acceleration_structure(int binding, AccelerationStructure *a) {
		if (!a)
			return;
		auto &i = get_for_binding(accelerations, binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	    i.info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	    i.info.accelerationStructureCount = 1;
	    i.info.pAccelerationStructures = &a->structure;
	}

	void DescriptorSet::set_uniform_buffer(int binding, Buffer *u) {
		set_uniform_buffer_with_offset(binding, u, 0);
	}

	void DescriptorSet::update() {
		//msg_write(format("update dset with %d buffers, %d images", buffers.num, images.num));

		Array<VkWriteDescriptorSet> wds;
		for (auto &b: buffers) {
			VkWriteDescriptorSet w = {};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = b.binding;
			w.dstArrayElement = 0;
			w.descriptorType = b.type;
			w.descriptorCount = 1;
			w.pBufferInfo = &b.info;
			wds.add(w);
		}

		for (auto &i: images) {
			VkWriteDescriptorSet w = {};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = i.binding;
			w.dstArrayElement = 0;
			w.descriptorType = i.type;
			w.descriptorCount = 1;
			w.pImageInfo = &i.info;
			wds.add(w);
		}
		for (auto &a: accelerations) {
			VkWriteDescriptorSet w = {};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = a.binding;
			w.dstArrayElement = 0;
			w.descriptorType = a.type;
			w.descriptorCount = 1;
			w.pNext = &a.info;
			wds.add(w);
		}

		//msg_write("dset update  " + p2s(descriptor_set));
		vkUpdateDescriptorSets(default_device->device, static_cast<uint32_t>(wds.num), &wds[0], 0, nullptr);
	}

	VkDescriptorSetLayout DescriptorSet::create_layout(const Array<VkDescriptorType> &types, const Array<int> &binding_no) {
		//std::cout << "create dset layout, " << num_ubos << " ubos, " << num_samplers << " samplers\n";
		Array<VkDescriptorSetLayoutBinding> bindings;
		for (int i=0; i<types.num;i++) {
			VkDescriptorSetLayoutBinding lb = {};
			lb.descriptorType = types[i];
			lb.descriptorCount = 1;
			lb.binding = binding_no[i];
			lb.pImmutableSamplers = nullptr;
			if (types[i] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				lb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			else
				lb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
			lb.stageFlags = VK_SHADER_STAGE_ALL;
			bindings.add(lb);
		}


		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = bindings.num;
		info.pBindings = &bindings[0];

		VkDescriptorSetLayout layout;
		auto r = vkCreateDescriptorSetLayout(default_device->device, &info, nullptr, &layout);
		if (r != VK_SUCCESS)
			throw Exception("failed to create descriptor set layout!  " + result2str(r));
		return layout;
	}

	void DescriptorSet::destroy_layout(VkDescriptorSetLayout layout) {
		vkDestroyDescriptorSetLayout(default_device->device, layout, nullptr);
	}



	Array<VkDescriptorSetLayout> DescriptorSet::parse_bindings(const string &bindings) {
		Array<VkDescriptorSetLayout> rr;
		int i0 = 1;
		while (i0 < bindings.num) {
			int i1 = bindings.find("[", i0);
			if (i1 < 0)
				break;
			int i2 = bindings.find("]", i1);
			if (i2 < 0)
				break;
			string bb = bindings.sub(i1+1, i2);
			Array<VkDescriptorType> types;
			Array<int> binding_no;
			digest_bindings(bb, types, binding_no);
			rr.add(DescriptorSet::create_layout(types, binding_no));

			i0 = i2 + 1;
		}
		return rr;
	}

	void DescriptorSet::digest_bindings(const string &bindings, Array<VkDescriptorType> &types, Array<int> &binding_no) {
		auto x = bindings.explode(",");
		//int num_samplers = 0;
		int cur_binding = 0;
		for (auto &y: x) {
			if (y == "" or y == ".") {
				cur_binding ++;
			} else {
				types.add(descriptor_type(y));
				binding_no.add(cur_binding ++);
			}
		}
	}


};

#endif


