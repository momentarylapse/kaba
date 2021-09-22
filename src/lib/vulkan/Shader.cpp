#if HAS_LIB_VULKAN


#include "Shader.h"
#include "DescriptorSet.h"
#include "Texture.h"
#include "Device.h"
#include <vulkan/vulkan.h>

#include <array>
#include <iostream>

#include "helper.h"
#include "../file/file.h"
#include "../image/image.h"


namespace vulkan {

extern bool verbose;

	VkShaderModule create_shader_module(const string &code) {
		if (code == "")
			return nullptr;
		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = code.num;
		info.pCode = reinterpret_cast<const uint32_t*>(code.data);

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(default_device->device, &info, nullptr, &shaderModule) != VK_SUCCESS) {
			throw Exception("failed to create shader module!");
		}

		return shaderModule;
	}

	Path Shader::directory;

	Shader::Shader() {
		topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		push_size = 0;
	}

	Shader::~Shader() {
		if (verbose)
			std::cout << "delete shader" << "\n";
		for (auto &m: modules)
			vkDestroyShaderModule(default_device->device, m.module, nullptr);
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

	Shader* Shader::load(const Path &_filename) {
		if (_filename.is_empty())
			return nullptr;
		Shader *s = new Shader();
		Path filename = directory << _filename;
		if (verbose)
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

	VkShaderModule Shader::get_module(VkShaderStageFlagBits stage) const {
		for (auto &m: modules)
			if (m.stage == stage)
				return m.module;
		return VK_NULL_HANDLE;
	}

};

#endif
