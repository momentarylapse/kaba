#if HAS_LIB_VULKAN


#include "Shader.h"
#include "DescriptorSet.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"
#include "../file/file.h"
#include "../image/image.h"


namespace vulkan{
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

	Shader* Shader::load(const Path &_filename) {
		if (_filename.is_empty())
			return nullptr;
		Shader *s = new Shader();
		Path filename = directory << _filename;
		std::cout << "load shader " << filename.str().c_str() << "\n";

		File *f = FileOpen(filename.with(".compiled"));
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
