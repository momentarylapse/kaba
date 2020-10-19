/*
 * DescriptorSet.cpp
 *
 *  Created on: 19.10.2020
 *      Author: michi
 */


#if HAS_LIB_VULKAN


#include "DescriptorSet.h"
#include "Shader.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"
#include "../file/file.h"


namespace vulkan{

	VkDescriptorPool descriptor_pool;

	Array<DescriptorSet*> descriptor_sets;

	VkDescriptorPool create_descriptor_pool() {
		std::array<VkDescriptorPoolSize, 5> pool_sizes = {};
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1024*64;
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_sizes[1].descriptorCount = 128*64;
		pool_sizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[2].descriptorCount = 1024*32;
		pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pool_sizes[3].descriptorCount = 64;
		pool_sizes[4].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		pool_sizes[4].descriptorCount = 64;


		VkDescriptorPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		info.pPoolSizes = pool_sizes.data();
		info.maxSets = 1024*64;

		VkDescriptorPool pool;
		if (vkCreateDescriptorPool(device, &info, nullptr, &pool) != VK_SUCCESS) {
			throw Exception("failed to create descriptor pool!");
		}
		return pool;
	}

	void destroy_descriptor_pool(VkDescriptorPool pool) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}

	DescriptorSet::DescriptorSet(const string &bindings) {
		Array<VkDescriptorType> types;
		Array<int> binding_no;
		digest_bindings(bindings, types, binding_no);

		num_dynamic_ubos = 0;

		layout = create_layout(types, binding_no);

		VkDescriptorSetAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = descriptor_pool;
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

	void DescriptorSet::__init__(const string &bindings) {
		new(this) DescriptorSet(bindings);
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
			} else if (y == "buffer") {
				types.add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
				binding_no.add(cur_binding ++);
			} else if (y == "dbuffer") {
				types.add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
				binding_no.add(cur_binding ++);
			} else if (y == "sampler") {
				types.add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				binding_no.add(cur_binding ++);
			} else if (y == "image") {
				types.add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				binding_no.add(cur_binding ++);
			} else if (y == "acceleration-structure") {
				types.add(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);
				binding_no.add(cur_binding ++);
			} else {
				std::cerr << "UNKNOWN BINDING TYPE: " << y.c_str() << "\n";
			}
		}
	}


};

#endif


