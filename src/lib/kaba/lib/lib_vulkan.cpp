#include "../../file/file.h"
#include "../kaba.h"
#include "../../config.h"
#include "common.h"
#include "exception.h"

#ifdef _X_USE_VULKAN_
	#include "../../vulkan/vulkan.h"

#if HAS_LIB_VULKAN
namespace vulkan {
	extern RenderPass *render_pass;
	extern VkDescriptorPool descriptor_pool;
};
#endif
#endif

namespace Kaba{



#if defined(_X_USE_VULKAN_) && HAS_LIB_VULKAN
	#define vul_p(p)		p



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


#else
	namespace vulkan{
		typedef int VertexBuffer;
		typedef int Texture;
		typedef int Shader;
		typedef int Pipeline;
		typedef int Vertex1;
		typedef int RenderPass;
		typedef int UBOWrapper;
		typedef int DescriptorSet;
		typedef int CommandBuffer;
	};
	#define vul_p(p)		nullptr
#endif


extern const Class *TypeIntList;
extern const Class *TypeFloatList;
extern const Class *TypeVectorList;
extern const Class *TypePointerList;
extern const Class *TypeImage;
extern const Class *TypeMatrix;



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
		class_add_elementx("pos", TypeVector, vul_p(&vulkan::Vertex1::pos));
		class_add_elementx("normal", TypeVector, vul_p(&vulkan::Vertex1::normal));
		class_add_elementx("u", TypeFloat32, vul_p(&vulkan::Vertex1::u));
		class_add_elementx("v", TypeFloat32, vul_p(&vulkan::Vertex1::v));
		class_add_funcx("__assign__", TypeVoid, vul_p(&VulkanVertex::__assign__));
			func_add_param("o", TypeVertex);

	add_class(TypeVertexList);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&VulkanVertexList::__init__));

	add_class(TypeVertexBuffer);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::VertexBuffer::__init__));
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::VertexBuffer::__delete__));
		class_add_funcx("build", TypeVoid, vul_p(&vulkan::VertexBuffer::build1));
			func_add_param("vertices", TypeVertexList);
		class_add_funcx("build", TypeVoid, vul_p(&vulkan::VertexBuffer::build1i));
			func_add_param("vertices", TypeVertexList);
			func_add_param("indices", TypeIntList);
		class_add_funcx("build", TypeVoid, vul_p(&vulkan::VertexBuffer::build));
			func_add_param("vertices", TypePointer);
			func_add_param("size", TypeInt);
			func_add_param("count", TypeInt);



	add_class(TypeTexture);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::Texture::__init__));
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::Texture::__delete__));
		class_add_funcx("override", TypeVoid, vul_p(&vulkan::Texture::override));
			func_add_param("image", TypeImage);
		class_add_funcx("override", TypeVoid, vul_p(&vulkan::Texture::overridex));
			func_add_param("data", TypePointer);
			func_add_param("nx", TypeInt);
			func_add_param("ny", TypeInt);
			func_add_param("nz", TypeInt);
		class_add_funcx("load", TypeTextureP, vul_p(&__VulkanLoadTexture), FLAG_STATIC);
			func_add_param("filename", TypeString);

	add_class(TypeShader);
		class_add_elementx("descr_layout", TypePointerList, vul_p(&vulkan::Shader::descr_layouts));
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::Shader::__init__));
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::Shader::__delete__));
		class_add_funcx("load", TypeShaderP, vul_p(&__VulkanLoadShader), FLAG_STATIC);
			func_add_param("filename", TypeString);

	add_class(TypeUBOWrapper);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::UBOWrapper::__init__));
			func_add_param("size", TypeInt);
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::UBOWrapper::__delete__));
		class_add_funcx("update", TypeVoid, vul_p(&vulkan::UBOWrapper::update));
			func_add_param("source", TypePointer);

	add_class(TypeDescriptorSet);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::DescriptorSet::__init__));
			func_add_param("layout", TypePointer);
			func_add_param("ubos", TypeUBOWrapperPList);
			func_add_param("tex", TypeTexturePList);
		//class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::DescriptorSet::__delete__));

	add_class(TypePipeline);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::Pipeline::__init__));
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::Pipeline::__delete__));
		class_add_funcx("set_wireframe", TypeVoid, vul_p(&vulkan::Pipeline::set_wireframe));
			func_add_param("w", TypeBool);
		class_add_funcx("set_line_width", TypeVoid, vul_p(&vulkan::Pipeline::set_line_width));
			func_add_param("w", TypeFloat32);
#if defined(_X_USE_VULKAN_) && HAS_LIB_VULKAN
		void (vulkan::Pipeline::*mpf)(float) = &vulkan::Pipeline::set_blend;
		class_add_funcx("set_blend", TypeVoid, vul_p(mpf));
#else
		class_add_funcx("set_blend", TypeVoid, nullptr);
#endif
			func_add_param("alpha", TypeFloat32);
		class_add_funcx("set_z", TypeVoid, vul_p(&vulkan::Pipeline::set_z));
			func_add_param("test", TypeBool);
			func_add_param("write", TypeBool);
		class_add_funcx("set_dynamic", TypeVoid, vul_p(&vulkan::Pipeline::set_dynamic));
			func_add_param("mode", TypeIntList);
		class_add_funcx("create", TypeVoid, vul_p(&vulkan::Pipeline::create));
		class_add_funcx("build", TypePipelineP, vul_p(&vulkan::Pipeline::build), FLAG_STATIC);
			func_add_param("shader", TypeShader);
			func_add_param("pass", TypeRenderPass);
			func_add_param("num_textures", TypeInt);
			func_add_param("create", TypeBool);

	add_class(TypeRenderPass);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::RenderPass::__init__));
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::RenderPass::__delete__));

	add_class(TypeCommandBuffer);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, vul_p(&vulkan::CommandBuffer::__init__));
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, vul_p(&vulkan::CommandBuffer::__delete__));
		class_add_funcx("begin", TypeVoid, vul_p(&vulkan::CommandBuffer::begin));
		class_add_funcx("end", TypeVoid, vul_p(&vulkan::CommandBuffer::end));
		class_add_funcx("set_pipeline", TypeVoid, vul_p(&vulkan::CommandBuffer::set_pipeline));
			func_add_param("p", TypePipeline);
		class_add_funcx("draw", TypeVoid, vul_p(&vulkan::CommandBuffer::draw));
			func_add_param("vb", TypeVertexBuffer);
		class_add_funcx("begin_render_pass", TypeVoid, vul_p(&vulkan::CommandBuffer::begin_render_pass));
			func_add_param("rp", TypeRenderPass);
			func_add_param("clear_color", TypeColor);
		class_add_funcx("end_render_pass", TypeVoid, vul_p(&vulkan::CommandBuffer::end_render_pass));
		class_add_funcx("push_constant", TypeVoid, vul_p(&vulkan::CommandBuffer::push_constant));
			func_add_param("offset", TypeInt);
			func_add_param("size", TypeInt);
			func_add_param("data", TypePointer);
		class_add_funcx("bind_descriptor_set", TypeVoid, vul_p(&vulkan::CommandBuffer::bind_descriptor_set));
			func_add_param("index", TypeInt);
			func_add_param("set", TypePointer);

	add_funcx("create_window", TypePointer, vul_p(&vulkan::create_window), FLAG_STATIC);
		func_add_param("title", TypeString);
		func_add_param("w", TypeInt);
		func_add_param("h", TypeInt);
	add_funcx("window_handle", TypeBool, vul_p(&vulkan::window_handle), FLAG_STATIC);
		func_add_param("w", TypePointer);
	add_funcx("window_close", TypeVoid, vul_p(&vulkan::window_close), FLAG_STATIC);
		func_add_param("w", TypePointer);

	add_funcx("init_vulkan", TypeVoid, vul_p(&vulkan::init), FLAG_STATIC);
		func_add_param("win", TypePointer);
	add_funcx("start_frame", TypeBool, vul_p(&vulkan::start_frame), FLAG_STATIC);
	add_funcx("end_frame", TypeVoid, vul_p(&vulkan::end_frame), FLAG_STATIC);
	add_funcx("submit_command_buffer", TypeVoid, vul_p(&vulkan::submit_command_buffer), FLAG_STATIC);
		func_add_param("cb", TypeCommandBuffer);
	add_funcx("create_default_descriptor_pool", TypeVoid, vul_p(&__create_default_descriptor_pool), FLAG_STATIC);
	add_funcx("wait_device_idle", TypeVoid, vul_p(&vulkan::wait_device_idle), FLAG_STATIC);



	add_ext_var("render_pass", TypeRenderPassP, (void*)vul_p(&vulkan::render_pass));
	add_ext_var("target_width", TypeInt, (void*)vul_p(&vulkan::target_height));
	add_ext_var("target_height", TypeInt, (void*)vul_p(&vulkan::target_height));
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

	add_const("VK_DYNAMIC_STATE_SCISSOR", TypeInt, (void*)vul_p(VK_DYNAMIC_STATE_SCISSOR));

}

};
