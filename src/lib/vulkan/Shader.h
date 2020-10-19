#pragma once

#if HAS_LIB_VULKAN



#include "../base/base.h"
#include "../file/path.h"
#include <vulkan/vulkan.h>
#include "helper.h"

namespace vulkan{

	class Texture;

	class UniformBuffer : public Buffer {
	public:
		UniformBuffer(int size);
		UniformBuffer(int size, int count);
		~UniformBuffer();

		void __init__(int size);
		void __init_multi__(int size, int count);
		void __delete__();

		void update(void *source);
		void update_part(void *source, int offset, int size);
		void update_single(void *source, int index);

		bool is_dynamic();

		int size_single;
		int count, size_single_aligned;
	};

	class Shader {
	public:
		Shader();
		~Shader();

		void __init__();
		void __delete__();

		struct Module {
			VkShaderModule module;
			VkShaderStageFlagBits stage;
		};
		Array<Module> modules;

		Array<VkDescriptorSetLayout> descr_layouts;
		int push_size;
		VkPrimitiveTopology topology;

		static Path directory;
		static Shader* load(const Path &filename);
	};

};

#endif
