#pragma once

#if HAS_LIB_VULKAN



#include "../base/base.h"
#include "../base/pointer.h"
#include "../os/path.h"
#include <vulkan/vulkan.h>
#include "helper.h"

namespace vulkan{

	class Shader : public Sharable<base::Empty> {
	public:
		Shader();
		~Shader();

		struct Module {
			VkShaderModule module;
			VkShaderStageFlagBits stage;
		};
		Array<Module> modules;

		Array<VkDescriptorSetLayout> descr_layouts;
		int push_size;

		VkShaderModule get_module(VkShaderStageFlagBits stage) const;

		static Path directory;
		static Shader* load(const Path &filename);
		static Shader* create(const string &source);
	};

};

#endif
