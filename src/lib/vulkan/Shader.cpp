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



	UBOWrapper::UBOWrapper(int _size) {
		size = _size;
		VkDeviceSize buffer_size = size;

		buffers.resize(swap_chain_images.num);
		memory.resize(swap_chain_images.num);

		for (size_t i=0; i<swap_chain_images.num; i++) {
			create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffers[i], memory[i]);
		}
	}

	UBOWrapper::~UBOWrapper() {
		for (size_t i=0; i<swap_chain_images.num; i++) {
			vkDestroyBuffer(device, buffers[i], nullptr);
			vkFreeMemory(device, memory[i], nullptr);
		}
	}
	void UBOWrapper::__init__(int size) {
		new(this) UBOWrapper(size);
	}
	void UBOWrapper::__delete__() {
		this->~UBOWrapper();
	}

	extern uint32_t image_index;
	void UBOWrapper::update(void *source) {
		void* data;
		vkMapMemory(device, memory[image_index], 0, size, 0, &data);
			memcpy(data, source, size);
		vkUnmapMemory(device, memory[image_index]);
	}


	VkDescriptorPool create_descriptor_pool() {
		std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = static_cast<uint32_t>(swap_chain_images.num) * 64;
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = static_cast<uint32_t>(swap_chain_images.num) * 64;

		VkDescriptorPoolCreateInfo pci = {};
		pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pci.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		pci.pPoolSizes = pool_sizes.data();
		pci.maxSets = static_cast<uint32_t>(swap_chain_images.num) * 64;

		VkDescriptorPool pool;
		if (vkCreateDescriptorPool(device, &pci, nullptr, &pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
		return pool;
	}

	void destroy_descriptor_pool(VkDescriptorPool pool) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}

	DescriptorSet::DescriptorSet(VkDescriptorSetLayout layout, const Array<UBOWrapper*> &ubos, const Array<Texture*> &tex) {
		std::vector<VkDescriptorSetLayout> layouts(swap_chain_images.num, layout);
		VkDescriptorSetAllocateInfo ai = {};
		ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ai.descriptorPool = descriptor_pool;
		ai.descriptorSetCount = static_cast<uint32_t>(swap_chain_images.num);
		ai.pSetLayouts = layouts.data();

		descriptor_sets.resize(swap_chain_images.num);
		if (vkAllocateDescriptorSets(device, &ai, descriptor_sets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		set(ubos, tex);
	}
	
	

	void DescriptorSet::__init__(VkDescriptorSetLayout layout, const Array<UBOWrapper*> &ubos, const Array<Texture*> &tex) {
		new(this) DescriptorSet(layout, ubos, tex);
	}
	/*void DescriptorSet::__delete__() {
		this->~DescriptorSet();
	}*/

	void DescriptorSet::set(const Array<UBOWrapper*> &ubos, const Array<Texture*> &tex) {
		std::cout << "create dset with " << ubos.num << " ubos, " << tex.num << " samplers\n";
		for (size_t i=0; i<swap_chain_images.num; i++) {
			std::vector<VkDescriptorBufferInfo> buffer_info;
			for (int j=0; j<ubos.num; j++) {
				VkDescriptorBufferInfo bi = {};
				bi.buffer = ubos[j]->buffers[i];
				bi.offset = 0;
				bi.range = ubos[j]->size;
				buffer_info.push_back(bi);
			}


			std::vector<VkDescriptorImageInfo> image_info;
			for (int j=0; j<tex.num; j++) {
				VkDescriptorImageInfo ii = {};
				ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				ii.imageView = tex[j]->view;
				ii.sampler = tex[j]->sampler;
				image_info.push_back(ii);
			}

			std::vector<VkWriteDescriptorSet> wds;
			for (int j=0; j<ubos.num; j++) {
				VkWriteDescriptorSet w;
				w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				w.dstSet = descriptor_sets[i];
				w.dstBinding = j;
				w.dstArrayElement = 0;
				w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				w.descriptorCount = 1;
				w.pBufferInfo = &buffer_info[j];
				wds.push_back(w);
			}

			for (int j=0; j<tex.num; j++) {
				VkWriteDescriptorSet w;
				w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				w.dstSet = descriptor_sets[i];
				w.dstBinding = 1 + j;
				w.dstArrayElement = 0;
				w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				w.descriptorCount = 1;
				w.pImageInfo = &image_info[j];
				wds.push_back(w);
			}

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(wds.size()), wds.data(), 0, nullptr);
		}
	}

	VkDescriptorSetLayout DescriptorSet::create_layout(int num_ubos, int num_samplers) {
		std::cout << "create dset layout, " << num_ubos << " ubos, " << num_samplers << " samplers\n";
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (int i=0; i<num_ubos;i++) {
			VkDescriptorSetLayoutBinding lb = {};
			lb.binding = i;
			lb.descriptorCount = 1;
			lb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lb.pImmutableSamplers = nullptr;
			lb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings.push_back(lb);
		}

		for (int i=0; i<num_samplers;i++) {
			VkDescriptorSetLayoutBinding lb = {};
			lb.binding = num_ubos + i;
			lb.descriptorCount = 1;
			lb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lb.pImmutableSamplers = nullptr;
			lb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings.push_back(lb);
		}


		VkDescriptorSetLayoutCreateInfo lci = {};
		lci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		lci.bindingCount = static_cast<uint32_t>(bindings.size());
		lci.pBindings = bindings.data();

		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(device, &lci, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
		return layout;
	}

	void DescriptorSet::destroy_layout(VkDescriptorSetLayout layout) {
		vkDestroyDescriptorSetLayout(device, layout, nullptr);
	}



	VkShaderModule create_shader_module(const string &code) {
		if (code == "")
			return nullptr;
		VkShaderModuleCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = code.num;
		ci.pCode = reinterpret_cast<const uint32_t*>(code.data);

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &ci, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	Shader::Shader() {
		vert_module = nullptr;
		geom_module = nullptr;
		frag_module = nullptr;
		topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		push_size = 0;
	}

	Shader::~Shader() {
		std::cout << "delete shader" << "\n";
		if (vert_module)
			vkDestroyShaderModule(device, vert_module, nullptr);
		if (geom_module)
			vkDestroyShaderModule(device, geom_module, nullptr);
		if (frag_module)
			vkDestroyShaderModule(device, frag_module, nullptr);
		for (auto &l: descr_layouts) {
			DescriptorSet::destroy_layout(l);
		}
	}


	void Shader::__init__() {
		new(this) Shader();
	}

	void Shader::__delete__() {
		this->~Shader();
	}

	Array<VkDescriptorSetLayout> parse_bindings(const string &bindings) {
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
			auto x = bb.explode(",");
			int num_ubos = 0;
			int num_samplers = 0;
			for (auto &y: x) {
				if (y == "buffer")
					num_ubos ++;
				if (y == "sampler")
					num_samplers ++;
				}
			rr.add(DescriptorSet::create_layout(num_ubos, num_samplers));

			i0 = i2 + 1;
		}
		return rr;
	}

	Shader* Shader::load(const string &filename) {
		Shader *s = new Shader();
		std::cout << "load shader " << filename.c_str() << "\n";

		File *f = FileOpen(filename + ".compiled");
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
					s->descr_layouts = parse_bindings(value);
				} else if (tag == "PushSize") {
					s->push_size = value._int();
				} else if (tag == "Input") {
				} else if (tag == "Info") {
				} else if (tag == "VertexShader") {
					s->vert_module = create_shader_module(value);
				} else if (tag == "GeometryShader") {
					s->geom_module = create_shader_module(value);
				} else if (tag == "FragmentShader") {
					s->frag_module = create_shader_module(value);
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
