/*
 * DescriptorSet.cpp
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */


#if HAS_LIB_VULKAN


#include "DescriptorSet.h"
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"
#include "../file/file.h"

namespace vulkan {

const string DESCRIPTOR_NAME_UNIFORM_BUFFER = "buffer";
const string DESCRIPTOR_NAME_UNIFORM_BUFFER_DYNAMIC = "dbuffer";
const string DESCRIPTOR_NAME_SAMPLER = "sampler";
const string DESCRIPTOR_NAME_STORAGE_IMAGE = "image";
const string DESCRIPTOR_NAME_ACCELERATION_STRUCTURE = "acceleration-structure";

VkDescriptorType descriptor_type(const string &s) {
	if (s == DESCRIPTOR_NAME_UNIFORM_BUFFER)
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	if (s == DESCRIPTOR_NAME_UNIFORM_BUFFER_DYNAMIC)
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	if (s == DESCRIPTOR_NAME_SAMPLER)
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	if (s == DESCRIPTOR_NAME_STORAGE_IMAGE)
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	if (s == DESCRIPTOR_NAME_ACCELERATION_STRUCTURE)
		return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
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

		if (vkCreateDescriptorPool(device, &info, nullptr, &pool) != VK_SUCCESS) {
			throw Exception("failed to create descriptor pool!");
		}
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}

	void DescriptorPool::__init__(const string &s, int max_sets) {
		new(this) DescriptorPool(s, max_sets);
	}
	void DescriptorPool::__delete__() {
		this->~DescriptorPool();
	}

	DescriptorSet *DescriptorPool::create_set(const string &s) {
		return new DescriptorSet(this, s);
	}

	Array<DescriptorSet*> descriptor_sets;

	DescriptorSet::DescriptorSet(DescriptorPool *pool, const string &bindings) {
		Array<VkDescriptorType> types;
		Array<int> binding_no;
		digest_bindings(bindings, types, binding_no);

		num_dynamic_ubos = 0;

		layout = create_layout(types, binding_no);

		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = pool->pool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layout;

		if (vkAllocateDescriptorSets(device, &info, &descriptor_set) != VK_SUCCESS) {
			throw Exception("failed to allocate descriptor sets!");
		}
	}
	DescriptorSet::~DescriptorSet() {
		// no VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT...
		//vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
		destroy_layout(layout);
	}

	void DescriptorSet::__delete__() {
		this->~DescriptorSet();
	}

	/*void DescriptorSet::set(const Array<UniformBuffer*> &_ubos, const Array<Texture*> &tex) {
		Array<int> offsets;
		for (auto *u: _ubos)
			offsets.add(0);
		set_with_offset(_ubos, offsets, tex);
	}*/

	void DescriptorSet::set_texture(int binding, Texture *t) {
		textures.add({t, binding});
	}

	void DescriptorSet::set_ubo_with_offset(int binding, UniformBuffer *u, int offset) {
		ubos.add({u, binding, offset});
	}

	void DescriptorSet::set_ubo(int binding, UniformBuffer *u) {
		set_ubo_with_offset(binding, u, 0);
	}

	void DescriptorSet::update() {

		//std::cout << "update dset with " << ubos.num << " ubos, " << tex.num << " samplers\n";
		Array<VkDescriptorBufferInfo> buffer_info;
		for (auto &u: ubos) {
			VkDescriptorBufferInfo bi = {};
			bi.buffer = u.ubo->buffer;
			bi.offset = u.offset;
			bi.range = u.ubo->size_single;
			buffer_info.add(bi);
		}


		Array<VkDescriptorImageInfo> image_info;
		for (auto &t: textures) {
			VkDescriptorImageInfo ii = {};
			ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ii.imageView = t.texture->view;
			ii.sampler = t.texture->sampler;
			image_info.add(ii);
		}

		Array<VkWriteDescriptorSet> wds;
		foreachi (auto &u, ubos, i) {
			VkWriteDescriptorSet w;
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = u.binding;
			w.dstArrayElement = 0;
			w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			if (u.ubo->is_dynamic())
				w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			w.descriptorCount = 1;
			w.pBufferInfo = &buffer_info[i];
			wds.add(w);
		}

		foreachi (auto &t, textures, i) {
			VkWriteDescriptorSet w;
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = descriptor_set;
			w.dstBinding = t.binding;
			w.dstArrayElement = 0;
			w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			w.descriptorCount = 1;
			w.pImageInfo = &image_info[i];
			wds.add(w);
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(wds.num), &wds[0], 0, nullptr);
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
			if (types[i] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				lb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			} else {
				lb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV;
			}
			bindings.add(lb);
		}


		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = bindings.num;
		info.pBindings = &bindings[0];

		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS) {
			throw Exception("failed to create descriptor set layout!");
		}
		return layout;
	}

	void DescriptorSet::destroy_layout(VkDescriptorSetLayout layout) {
		vkDestroyDescriptorSetLayout(device, layout, nullptr);
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
			string bb = bindings.substr(i1+1, i2-i1-1);
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


