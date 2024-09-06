#if HAS_LIB_VULKAN


#include "Shader.h"
#include "DescriptorSet.h"
#include "Texture.h"
#include "Device.h"
#include <vulkan/vulkan.h>

#include "common.h"
#include "helper.h"
#include "../os/file.h"
#include "../os/filesystem.h"
#include "../os/formatter.h"
#include "../os/msg.h"
#include "../image/image.h"

#if HAS_LIB_SHADERC
#include "shaderc/shaderc.h"
#endif


namespace vulkan {

#if HAS_LIB_SHADERC
static shaderc_compiler_t shaderc = nullptr;
#endif


	VkShaderModule create_shader_module(const bytes &code) {
		if (code.num == 0)
			return nullptr;
		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = code.num;
		info.pCode = reinterpret_cast<const uint32_t*>(code.data);

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(default_device->device, &info, nullptr, &shaderModule) != VK_SUCCESS)
			throw Exception("failed to create shader module!");

		return shaderModule;
	}

	Path Shader::directory;


	string overwrite_bindings;



	const int TYPE_LAYOUT = -41;
	const int TYPE_MODULE = -42;


	string vertex_module_default = "vertex-default-nix";

	string shader_error;

#if HAS_LIB_SHADERC
	struct ShaderSourcePart {
		VkShaderStageFlagBits type;
		string source;
	};

	struct ShaderMetaData {
		string version, name, bindings;
		Array<string> extensions;
		int push_size = 0;
	};

	struct ShaderModule {
		ShaderMetaData meta;
		string source;
	};
	static Array<ShaderModule> shader_modules;

	Array<ShaderSourcePart> get_shader_parts(const string &source) {
		Array<ShaderSourcePart> parts;
		bool has_vertex = false;
		bool has_fragment = false;
		int pos = 0;
		while (pos < source.num - 5) {
			int pos0 = source.find("<", pos);
			if (pos0 < 0)
				break;
			pos = pos0 + 1;
			int pos1 = source.find(">", pos0);
			if (pos1 < 0)
				break;

			string tag = source.sub(pos0 + 1, pos1);
			if ((tag.num > 64) or (tag.find("<") >= 0))
				continue;

			int pos2 = source.find("</" + tag + ">", pos1 + 1);
			if (pos2 < 0)
				continue;
			ShaderSourcePart p;
			p.source = source.sub(pos1 + 1, pos2);
			pos = pos2 + tag.num + 3;
			if (tag == "VertexShader") {
				p.type = VK_SHADER_STAGE_VERTEX_BIT;
				has_vertex = true;
			} else if (tag == "FragmentShader") {
				p.type = VK_SHADER_STAGE_FRAGMENT_BIT;
				has_fragment = true;
			} else if (tag == "ComputeShader") {
				p.type = VK_SHADER_STAGE_COMPUTE_BIT;
			} else if (tag == "TessControlShader") {
				p.type = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			} else if (tag == "TessEvaluationShader") {
				p.type = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			} else if (tag == "GeometryShader") {
				p.type = VK_SHADER_STAGE_GEOMETRY_BIT;
			} else if (tag == "RayGenShader") {
				p.type = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			} else if (tag == "RayClosestHitShader") {
				p.type = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			} else if (tag == "RayAnyHitShader") {
				p.type = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			} else if (tag == "RayMissShader") {
				p.type = VK_SHADER_STAGE_MISS_BIT_KHR;
			} else if (tag == "Module") {
				p.type = (VkShaderStageFlagBits)TYPE_MODULE;
			} else if (tag == "Layout") {
				p.type = (VkShaderStageFlagBits)TYPE_LAYOUT;
			} else {
				msg_error("unknown shader tag: '" + tag + "'");
				continue;
			}
			parts.add(p);
		}
		if (has_fragment and !has_vertex) {
			if (verbosity >= 1)
				msg_write(" ...auto import " + vertex_module_default);
			parts.add({VK_SHADER_STAGE_VERTEX_BIT, format("#import %s\n", vertex_module_default)});
		}
		return parts;
	}

	string get_inside_of_tag(const string &source, const string &tag) {
		string r;
		int pos0 = source.find("<" + tag + ">");
		if (pos0 < 0)
			return "";
		pos0 += tag.num + 2;
		int pos1 = source.find("</" + tag + ">", pos0);
		if (pos1 < 0)
			return "";
		return source.sub(pos0, pos1);
	}

	string expand_shader_source(const string &source, ShaderMetaData &meta) {
		string r = source;
		while (true) {
			int p = r.find("#import", 0);
			if (p < 0)
				break;
			int p2 = r.find("\n", p);
			string imp = r.sub(p + 7, p2).replace(" ", "");
			//msg_error("import '" + imp + "'");

			bool found = false;
			for (auto &m: shader_modules)
				if (m.meta.name == imp) {
					//msg_error("FOUND " + imp);
					r = r.head(p) + "\n// <<\n" + m.source + "\n// >>\n" + r.sub(p2);
					found = true;
				}
			if (!found)
				throw Exception(format("shader import '%s' not found", imp));
		}

		string intro;
		if (meta.version != "")
			intro += "#version " + meta.version + "\n";
		for (auto &e: meta.extensions)
			intro += "#extension " + e + " : require\n";
		if (r.find("GL_ARB_separate_shader_objects", 0) < 0)
			intro += "#extension GL_ARB_separate_shader_objects : enable\n";
		return intro + r;
	}

	shaderc_shader_kind vk_to_shaderc(VkShaderStageFlagBits s) {
		if (s == VK_SHADER_STAGE_VERTEX_BIT)
			return shaderc_glsl_vertex_shader;
		if (s == VK_SHADER_STAGE_FRAGMENT_BIT)
			return shaderc_glsl_fragment_shader;
		if (s == VK_SHADER_STAGE_COMPUTE_BIT)
			return shaderc_compute_shader;
		if (s == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
			return shaderc_tess_control_shader;
		if (s == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
			return shaderc_tess_evaluation_shader;
		if (s == VK_SHADER_STAGE_GEOMETRY_BIT)
			return shaderc_geometry_shader;
		if (s == VK_SHADER_STAGE_RAYGEN_BIT_KHR)
			return shaderc_raygen_shader;
		if (s == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
			return shaderc_closesthit_shader;
		if (s == VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
			return shaderc_anyhit_shader;
		if (s == VK_SHADER_STAGE_MISS_BIT_KHR)
			return shaderc_miss_shader;
		throw Exception("unhandled shader type...");
		return shaderc_glsl_vertex_shader;
	}

	VkShaderModule create_vk_shader(const string &_source, VkShaderStageFlagBits type, ShaderMetaData &meta) {
		string source = expand_shader_source(_source, meta);
		if (source.num == 0)
			return nullptr;
		static shaderc_compile_options_t options;
		if (!shaderc) {
			shaderc = shaderc_compiler_initialize();
			options = shaderc_compile_options_initialize();
			shaderc_compile_options_add_macro_definition(options, "vulkan", 6, "1", 1);
		}
		//msg_write(">>>----------------------------------------------------------------------------------- xxxx");
		//msg_write(source);

		auto result = shaderc_compile_into_spv(shaderc,
				(const char*)&source[0], source.num,
				vk_to_shaderc(type), "dummy", "main", options);

		if (shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success) {
			bytes code = bytes(shaderc_result_get_bytes(result), shaderc_result_get_length(result));
			shaderc_result_release(result);
			return create_shader_module(code);
		} else {
			shader_error = shaderc_result_get_error_message(result);
			shaderc_result_release(result);
			msg_error(shader_error);
			throw Exception("while compiling shader: " + shader_error);
		}
		return nullptr;
	}

	ShaderMetaData parse_meta(string source) {
		ShaderMetaData m;
		for (auto &x: source.explode("\n")) {
			auto y = x.explode("=");
			if (y.num == 2) {
				string k = y[0].trim();
				string v = y[1].trim();
				if (k == "name") {
					m.name = v;
				} else if (k == "version") {
					m.version = v;
				} else if ((k == "binding") or (k == "bindings")) {
					m.bindings = v;
				} else if (k == "pushsize") {
					m.push_size = v._int();
				} else if (k == "input") {
				} else if (k == "topology") {
				} else if (k == "extensions") {
					m.extensions = v.explode(",");
				} else {
					msg_error("unhandled shader meta: " + x);
				}
			}
		}
		if (overwrite_bindings != "")
			m.bindings = overwrite_bindings;
		return m;
	}

	xfer<Shader> Shader::create(const string &source) {
		auto parts = get_shader_parts(source);

		if (parts.num == 0)
			throw Exception("no shader tags found (<VertexShader>...</VertexShader> or <FragmentShader>...</FragmentShader>)");

		//int prog = create_empty_shader_program();
		auto s = new Shader();

		ShaderMetaData meta;
		for (auto p: parts) {
			if ((int)p.type == TYPE_MODULE) {
				ShaderModule m;
				m.source = p.source;
				m.meta = meta;
				shader_modules.add(m);
				if (verbosity >= 1)
					msg_write("new module '" + m.meta.name + "'");
				return nullptr;
			} else if ((int)p.type == TYPE_LAYOUT) {
				meta = parse_meta(p.source);
			} else {
				auto mm = create_vk_shader(p.source, p.type, meta);
				if (mm)
					s->modules.add({mm, p.type});
			}
		}

		s->push_size = meta.push_size;
		s->descr_layouts = DescriptorSet::parse_bindings(meta.bindings);
		return s;
	}
#else

	xfer<Shader> Shader::create(const string &source) {
		throw Exception("Shader.crete() requires this program to be compiled with shaderc support!");
		return nullptr;
	}
#endif



	Shader::Shader() {
		push_size = 0;
	}

	Shader::~Shader() {
		if (verbosity >= 1)
			msg_write("delete shader");
		for (auto &m: modules)
			vkDestroyShaderModule(default_device->device, m.module, nullptr);
		for (auto &l: descr_layouts)
			DescriptorSet::destroy_layout(l);
	}


	xfer<Shader> Shader::load(const Path &_filename) {
		if (!_filename)
			return nullptr;
		Path filename = directory | _filename;
		if (verbosity >= 1)
			msg_write(format("load shader %s", filename));

#if HAS_LIB_SHADERC
		if (!os::fs::exists(filename.with(".compiled")))
			return Shader::create(os::fs::read_text(filename));
#endif

		Shader *s = new Shader();

		auto f = ownify(os::fs::open(filename.with(".compiled"), "rb"));
		try {
			while(true) {
				string tag = f->read_str();
				string value = f->read_str();
				//std::cout << tag << "\n";
				if (tag == "Topology") {
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
					msg_write("WARNING: " + value);
				}
			}
		} catch(...) {
		}

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
