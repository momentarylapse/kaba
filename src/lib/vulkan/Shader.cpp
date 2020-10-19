#if HAS_LIB_VULKAN


#include "Shader.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"
#include "../file/file.h"
#include "../image/image.h"


namespace vulkan{

	VkDescriptorPool descriptor_pool;

	Array<DescriptorSet*> descriptor_sets;
	Array<Shader*> shaders;
	Array<UniformBuffer*> ubo_wrappers;

	extern VkPhysicalDeviceProperties device_properties;
	int make_aligned(int size) {
		if (device_properties.limits.minUniformBufferOffsetAlignment == 0)
			return 0;
		return (size + device_properties.limits.minUniformBufferOffsetAlignment - 1) & ~(size - 1);
	}


	UniformBuffer::UniformBuffer(int _size) {
		count = 0;
		size = _size;
		size_single = size;
		size_single_aligned = size;
		VkDeviceSize buffer_size = size;

		create(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	UniformBuffer::UniformBuffer(int _size, int _count) {
		// "dynamic"
		count = _count;
		size_single = _size;
		size_single_aligned = make_aligned(size_single);
		size = size_single_aligned * count;
		VkDeviceSize buffer_size = size;

		create(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	UniformBuffer::~UniformBuffer() {
	}
	void UniformBuffer::__init__(int size) {
		new(this) UniformBuffer(size);
	}
	void UniformBuffer::__delete__() {
		this->~UniformBuffer();
	}

	bool UniformBuffer::is_dynamic() {
		return count > 0;
	}

	void UniformBuffer::update_part(void *source, int offset, int update_size) {
		void* data;
		map(offset, update_size, &data);
		memcpy(data, source, update_size);
		unmap();
	}

	void UniformBuffer::update(void *source) {
		update_part(source, 0, size);
	}

	void UniformBuffer::update_single(void *source, int index) {
		update_part(source, size_single_aligned * index, size_single);
	}


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



	VkShaderModule create_shader_module(const string &code) {
		if (code == "")
			return nullptr;
		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = code.num;
		info.pCode = reinterpret_cast<const uint32_t*>(code.data);

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &info, nullptr, &shaderModule) != VK_SUCCESS) {
			throw Exception("failed to create shader module!");
		}

		return shaderModule;
	}

	Path Shader::directory;

	Shader::Shader() {
		topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		push_size = 0;

		shaders.add(this);
	}

	Shader::~Shader() {
		std::cout << "delete shader" << "\n";
		for (auto &m: modules)
			vkDestroyShaderModule(device, m.module, nullptr);
		for (auto &l: descr_layouts) {
			DescriptorSet::destroy_layout(l);
		}

		for (int i=0; i<shaders.num; i++)
			if (shaders[i] == this)
				shaders.erase(i);
	}


	void Shader::__init__() {
		new(this) Shader();
	}

	void Shader::__delete__() {
		this->~Shader();
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

	Shader* Shader::load(const Path &_filename) {
		if (_filename.is_empty())
			return nullptr;
		Shader *s = new Shader();
		Path filename = directory << _filename;
		std::cout << "load shader " << filename.str().c_str() << "\n";

		File *f = FileOpen(Path(filename.str() + ".compiled"));
		try {
			while(true) {
				string tag = f->read_str();
				string value = f->read_str();
				//std::cout << tag << "\n";
				if (tag == "Topology") {
					if (value == "points")
						s->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
					if (value == "lines")
						s->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				} else if (tag == "Bindings") {
					s->descr_layouts = DescriptorSet::parse_bindings(value);
				} else if (tag == "PushSize") {
					s->push_size = value._int();
				} else if (tag == "Input") {
				} else if (tag == "Info") {
				} else if (tag == "VertexShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_VERTEX_BIT});
				} else if (tag == "GeometryShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_GEOMETRY_BIT});
				} else if (tag == "FragmentShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_FRAGMENT_BIT});
				} else if (tag == "ComputeShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_COMPUTE_BIT});
				} else if (tag == "RayGenShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_RAYGEN_BIT_NV});
				} else if (tag == "RayMissShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_MISS_BIT_NV});
				} else if (tag == "RayClosestHitShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV});
				} else if (tag == "RayAnyHitShader") {
					s->modules.add({create_shader_module(value), VK_SHADER_STAGE_ANY_HIT_BIT_NV});
				} else {
					std::cerr << "WARNING: " << value.c_str() << "\n";
				}
			}
		} catch(...) {
		}
		delete f;

		return s;
	}

};

#endif
