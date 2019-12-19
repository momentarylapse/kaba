#include "../../file/file.h"
#include "../kaba.h"
#include "../../config.h"
#include "common.h"
#include "exception.h"

#ifdef _X_USE_VULKAN_
	#include "../../vulkan/vulkan.h"
	#include "../../image/image.h"
	#include "../../math/math.h"
#endif

namespace vulkan {
	extern RenderPass *render_pass;
	extern VkDescriptorPool descriptor_pool;
};

namespace Kaba{



#ifdef _X_USE_NIX_
	#define vul_p(p)		(void*)p



#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

vulkan::Texture* __VulkanLoadTexture(const string &filename) {
	KABA_EXCEPTION_WRAPPER(return vulkan::Texture::load(filename));
	return nullptr;
}

vulkan::Shader* __VulkanLoadShader(const string &filename) {
	KABA_EXCEPTION_WRAPPER(return vulkan::Shader::load(filename));
	return nullptr;
}

#pragma GCC pop_options

void __create_default_descriptor_pool() {
	vulkan::descriptor_pool = vulkan::create_descriptor_pool();
}


#else
	namespace vulkan{
		typedef int VertexBuffer;
		typedef int Texture;
		typedef int Shader;
	};
	#define vul_p(p)		nullptr
#endif


extern const Class *TypeIntList;
extern const Class *TypeFloatList;
extern const Class *TypeVectorList;
extern const Class *TypePointerList;
extern const Class *TypeImage;
extern const Class *TypeMatrix;

class VulkanVertexList : public Array<vulkan::Vertex1> {
public:
	void __init__() {
		new(this) VulkanVertexList;
	}
};

class VulkanVertex : public vulkan::Vertex1 {
public:
	void __assign__(const VulkanVertex &o) { *this = o; }
};

static vulkan::VertexBuffer* _vulkan_vertex_buffer_build(const VulkanVertexList &vertices, const Array<int> &indices) {
	Array<uint16_t> indices16;
	for (auto i: indices)
		indices16.add(i);
	return vulkan::VertexBuffer::build1i(vertices, indices16);
}


void SIAddPackageVulkan() {
	add_package("vulkan", false);
	
	auto TypeVertexBuffer	= add_type  ("VertexBuffer", sizeof(vulkan::VertexBuffer));
	auto TypeVertexBufferP	= add_type_p("VertexBuffer*", TypeVertexBuffer);
	auto TypeTexture		= add_type  ("Texture", sizeof(vulkan::Texture));
	auto TypeTextureP		= add_type_p("Texture*", TypeTexture);
	auto TypeTexturePList	= add_type_a("Texture*[]", TypeTextureP, -1);
	auto TypeShader			= add_type  ("Shader", sizeof(vulkan::Shader));
	auto TypeShaderP		= add_type_p("Shader*", TypeShader);
	auto TypeCommandBuffer	= add_type  ("CommandBuffer", sizeof(vulkan::CommandBuffer));
	auto TypeCommandBufferP	= add_type_p("CommandBuffer*", TypeCommandBuffer);
	auto TypeVertex			= add_type  ("Vertex", sizeof(vulkan::Vertex1));
	auto TypeVertexList		= add_type_a("Vertex[]",TypeVertex, -1);
	auto TypePipeline		= add_type  ("Pipeline", sizeof(vulkan::Pipeline));
	auto TypePipelineP		= add_type_p("Pipeline*", TypePipeline);
	auto TypeRenderPass		= add_type  ("RenderPass", sizeof(vulkan::RenderPass));
	auto TypeRenderPassP	= add_type_p("RenderPass*", TypeRenderPass);
	auto TypeUBOWrapper		= add_type  ("UBOWrapper", sizeof(vulkan::UBOWrapper));
	auto TypeUBOWrapperP	= add_type_p("UBOWrapper*", TypeUBOWrapper);
	auto TypeUBOWrapperPList= add_type_a("UBOWrapper*[]", TypeUBOWrapperP, -1);
	auto TypeDescriptorSet	= add_type  ("DescriptorSet", sizeof(vulkan::DescriptorSet));
	auto TypeDescriptorSetP	= add_type_p("DescriptorSet*", TypeDescriptorSet);


	add_class(TypeVertex);
		class_add_elementx("pos", TypeVector, &vulkan::Vertex1::pos);
		class_add_elementx("normal", TypeVector, &vulkan::Vertex1::normal);
		class_add_elementx("u", TypeFloat32, &vulkan::Vertex1::u);
		class_add_elementx("v", TypeFloat32, &vulkan::Vertex1::v);
		class_add_func("__assign__", TypeVoid, vul_p(mf(&VulkanVertex::__assign__)));
			func_add_param("o", TypeVertex);

	add_class(TypeVertexList);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&VulkanVertexList::__init__)));

	add_class(TypeVertexBuffer);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::VertexBuffer::__init__)));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::VertexBuffer::__delete__)));
		class_add_func("build", TypeVertexBufferP, vul_p(mf(&vulkan::VertexBuffer::build1)), FLAG_STATIC);
			func_add_param("vertices", TypeVertexList);
		class_add_func("build", TypeVertexBufferP, vul_p(&_vulkan_vertex_buffer_build), FLAG_STATIC);
			func_add_param("vertices", TypeVertexList);
			func_add_param("indices", TypeIntList);



	add_class(TypeTexture);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::Texture::__init__)));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::Texture::__delete__)));
		class_add_func("override", TypeVoid, vul_p(mf(&vulkan::Texture::override)));
			func_add_param("image", TypeImage);
		class_add_func("load", TypeTextureP, vul_p(&__VulkanLoadTexture), FLAG_STATIC);
			func_add_param("filename", TypeString);

	add_class(TypeShader);
		class_add_elementx("descr_layout", TypePointerList, &vulkan::Shader::descr_layouts);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::Shader::__init__)));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::Shader::__delete__)));
		class_add_func("load", TypeShaderP, vul_p(&__VulkanLoadShader), FLAG_STATIC);
			func_add_param("filename", TypeString);

	add_class(TypeUBOWrapper);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::UBOWrapper::__init__)));
			func_add_param("size", TypeInt);
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::UBOWrapper::__delete__)));
		class_add_func("update", TypeVoid, vul_p(mf(&vulkan::UBOWrapper::update)));
			func_add_param("source", TypePointer);

	add_class(TypeDescriptorSet);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::DescriptorSet::__init__)));
			func_add_param("layout", TypePointer);
			func_add_param("ubos", TypeUBOWrapperPList);
			func_add_param("tex", TypeTexturePList);
		//class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::DescriptorSet::__delete__)));

	add_class(TypePipeline);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::Pipeline::__init__)));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::Pipeline::__delete__)));
		class_add_func("set_wireframe", TypeVoid, vul_p(mf(&vulkan::Pipeline::set_wireframe)));
			func_add_param("w", TypeBool);
		class_add_func("set_line_width", TypeVoid, vul_p(mf(&vulkan::Pipeline::set_line_width)));
			func_add_param("w", TypeFloat32);
		void (vulkan::Pipeline::*mpf)(float) = &vulkan::Pipeline::set_blend;
		class_add_func("set_blend", TypeVoid, vul_p(mf(mpf)));
			func_add_param("alpha", TypeFloat32);
		class_add_func("set_z", TypeVoid, vul_p(mf(&vulkan::Pipeline::set_z)));
			func_add_param("test", TypeBool);
			func_add_param("write", TypeBool);
		class_add_func("set_dynamic", TypeVoid, vul_p(mf(&vulkan::Pipeline::set_dynamic)));
			func_add_param("mode", TypeIntList);
		class_add_func("create", TypeVoid, vul_p(mf(&vulkan::Pipeline::create)));
		class_add_func("build", TypePipelineP, vul_p(mf(&vulkan::Pipeline::build)), FLAG_STATIC);
			func_add_param("shader", TypeShader);
			func_add_param("pass", TypeRenderPass);
			func_add_param("create", TypeBool);

	add_class(TypeRenderPass);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::RenderPass::__init__)));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::RenderPass::__delete__)));

	add_class(TypeCommandBuffer);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(mf(&vulkan::CommandBuffer::__init__)));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(mf(&vulkan::CommandBuffer::__delete__)));
		class_add_func("begin", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::begin)));
		class_add_func("end", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::end)));
		class_add_func("set_pipeline", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::set_pipeline)));
			func_add_param("p", TypePipeline);
		class_add_func("draw", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::draw)));
			func_add_param("vb", TypeVertexBuffer);
		class_add_func("begin_render_pass", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::begin_render_pass)));
			func_add_param("rp", TypeRenderPass);
			func_add_param("clear_color", TypeColor);
		class_add_func("end_render_pass", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::end_render_pass)));
		class_add_func("push_constant", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::push_constant)));
			func_add_param("offset", TypeInt);
			func_add_param("size", TypeInt);
			func_add_param("data", TypePointer);
		class_add_func("bind_descriptor_set", TypeVoid, vul_p(mf(&vulkan::CommandBuffer::bind_descriptor_set)));
			func_add_param("index", TypeInt);
			func_add_param("set", TypePointer);

	add_func("create_window", TypePointer, vul_p(&vulkan::create_window), FLAG_STATIC);
		func_add_param("title", TypeString);
		func_add_param("w", TypeInt);
		func_add_param("h", TypeInt);
	add_func("window_handle", TypeBool, vul_p(&vulkan::window_handle), FLAG_STATIC);
		func_add_param("w", TypePointer);
	add_func("window_close", TypeVoid, vul_p(&vulkan::window_close), FLAG_STATIC);
		func_add_param("w", TypePointer);

	add_func("init_vulkan", TypeVoid, vul_p(&vulkan::init), FLAG_STATIC);
		func_add_param("win", TypePointer);
	add_func("start_frame", TypeBool, vul_p(&vulkan::start_frame), FLAG_STATIC);
	add_func("end_frame", TypeVoid, vul_p(&vulkan::end_frame), FLAG_STATIC);
	add_func("submit_command_buffer", TypeVoid, vul_p(&vulkan::submit_command_buffer), FLAG_STATIC);
		func_add_param("cb", TypeCommandBuffer);
	add_func("create_default_descriptor_pool", TypeVoid, vul_p(&__create_default_descriptor_pool), FLAG_STATIC);
	add_func("wait_device_idle", TypeVoid, vul_p(&vulkan::wait_device_idle), FLAG_STATIC);



	add_ext_var("target_width", TypeInt, vul_p(&vulkan::target_height));
	add_ext_var("target_height", TypeInt, vul_p(&vulkan::target_height));
	/*add_ext_var("target", TypeRect, vul_p(&vulkan::target_rect));
	add_ext_var("fullscreen", TypeBool, vul_p(&vulkan::Fullscreen));
	add_ext_var("Api", TypeString, vul_p(&vulkan::ApiName));*/
	//add_ext_var("TextureLifeTime", TypeInt, vul_p(&vulkan::TextureMaxFramesToLive));
	//add_ext_var("LineWidth", TypeFloat32, vul_p(&vulkan::line_width));
	//add_ext_var("SmoothLines", TypeBool, vul_p(&vulkan::smooth_lines));

	// alpha operations
/*	add_const("ALPHA_NONE",             TypeInt, vul_p(ALPHA_NONE));
	add_const("ALPHA_ZERO",             TypeInt, vul_p(ALPHA_ZERO));
	add_const("ALPHA_ONE",              TypeInt, vul_p(ALPHA_ONE));
	add_const("ALPHA_COLOR_KEY",        TypeInt, vul_p(ALPHA_COLOR_KEY_SMOOTH));
	add_const("ALPHA_COLOR_KEY_HARD",   TypeInt, vul_p(ALPHA_COLOR_KEY_HARD));
	add_const("ALPHA_ADD",              TypeInt, vul_p(ALPHA_ADD));
	add_const("ALPHA_MATERIAL",         TypeInt, vul_p(ALPHA_MATERIAL));
	add_const("ALPHA_SOURCE_COLOR",     TypeInt, vul_p(ALPHA_SOURCE_COLOR));
	add_const("ALPHA_SOURCE_INV_COLOR", TypeInt, vul_p(ALPHA_SOURCE_INV_COLOR));
	add_const("ALPHA_SOURCE_ALPHA",     TypeInt, vul_p(ALPHA_SOURCE_ALPHA));
	add_const("ALPHA_SOURCE_INV_ALPHA", TypeInt, vul_p(ALPHA_SOURCE_INV_ALPHA));
	add_const("ALPHA_DEST_COLOR",       TypeInt, vul_p(ALPHA_DEST_COLOR));
	add_const("ALPHA_DEST_INV_COLOR",   TypeInt, vul_p(ALPHA_DEST_INV_COLOR));
	add_const("ALPHA_DEST_ALPHA",       TypeInt, vul_p(ALPHA_DEST_ALPHA));
	add_const("ALPHA_DEST_INV_ALPHA",   TypeInt, vul_p(ALPHA_DEST_INV_ALPHA));
	// stencil operations
	add_const("STENCIL_NONE",               TypeInt, vul_p(STENCIL_NONE));
	add_const("STENCIL_INCREASE",           TypeInt, vul_p(STENCIL_INCREASE));
	add_const("STENCIL_DECREASE",           TypeInt, vul_p(STENCIL_DECREASE));
	add_const("STENCIL_SET",                TypeInt, vul_p(STENCIL_SET));
	add_const("STENCIL_MASK_EQUAL",         TypeInt, vul_p(STENCIL_MASK_EQUAL));
	add_const("STENCIL_MASK_NOT_EQUAL",     TypeInt, vul_p(STENCIL_MASK_NOT_EQUAL));
	add_const("STENCIL_MASK_LESS",          TypeInt, vul_p(STENCIL_MASK_LESS));
	add_const("STENCIL_MASK_LESS_EQUAL",    TypeInt, vul_p(STENCIL_MASK_LESS_EQUAL));
	add_const("STENCIL_MASK_GREATER",       TypeInt, vul_p(STENCIL_MASK_GREATER));
	add_const("STENCIL_MASK_GREATER_EQUAL", TypeInt, vul_p(STENCIL_MASK_GREATER_EQUAL));
	add_const("STENCIL_RESET",              TypeInt, vul_p(STENCIL_RESET));
	// fog
	add_const("FOG_LINEAR", TypeInt, vul_p(FOG_LINEAR));
	add_const("FOG_EXP",    TypeInt, vul_p(FOG_EXP));
	add_const("FOG_EXP2",   TypeInt, vul_p(FOG_EXP2));


	add_ext_var("vb_temp", TypeVertexBufferP, vul_p(&vulkan::vb_temp));*/

	add_const("VK_DYNAMIC_STATE_SCISSOR", TypeInt, vul_p(VK_DYNAMIC_STATE_SCISSOR));


	add_ext_var("render_pass", TypeRenderPassP, vul_p(&vulkan::render_pass));
}

};
