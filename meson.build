project('kaba', 'cpp', default_options : ['cpp_std=c++17', 'warning_level=1', 'optimization=2', 'debug=true'])
add_project_arguments('-Wno-unused', '-Wstrict-aliasing=0', '-fno-omit-frame-pointer', language : ['cpp'])

#install_data
#meson.add_install_script('')
#install_subdir

cc = meson.get_compiler('cpp')

# mandatory dependencies
mdep = cc.find_library('m', required : false)
threaddep = dependency('threads')
gtk3dep = dependency('gtk+-3.0')
add_project_arguments('-DHAS_LIB_GTK3=1', language : ['cpp'])
zlibdep = dependency('zlib')
add_project_arguments('-DHAS_LIB_ZLIB=1', language : ['cpp'])
deps = [mdep, threaddep, gtk3dep, zlibdep]

# optional dependency: xlib
d = cc.find_library('X11', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_XLIB=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_XLIB=0', language : ['cpp'])
endif

# optional dependency: unwind
d = cc.find_library('unwind', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_UNWIND=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_UNWIND=0', language : ['cpp'])
endif

# optional dependency: dl
d = cc.find_library('dl', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_DL=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_DL=0', language : ['cpp'])
endif

# optional dependency: vulkan
d = cc.find_library('vulkan', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_VULKAN=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_VULKAN=0', language : ['cpp'])
endif

# optional dependency: shaderc
if 
	add_project_arguments('-DHAS_LIB_SHADERC=1', language : ['cpp'])
	deps += []
else
	add_project_arguments('-DHAS_LIB_SHADERC=0', language : ['cpp'])
endif

# optional dependency: gl
d = cc.find_library('GL', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_GL=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_GL=0', language : ['cpp'])
endif

# optional dependency: glfw
d = cc.find_library('glfw', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_GLFW=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_GLFW=0', language : ['cpp'])
endif


sources = [
	'src/helper/elf.cpp',
	'src/helper/experiments.cpp',
	'src/helper/symbols.cpp',
	'src/lib/algebra/crypto.cpp',
	'src/lib/algebra/vli.cpp',
	'src/lib/any/any.cpp',
	'src/lib/base/array.cpp',
	'src/lib/base/pointer.cpp',
	'src/lib/base/strings.cpp',
	'src/lib/doc/chunked.cpp',
	'src/lib/doc/config.cpp',
	'src/lib/doc/pdf.cpp',
	'src/lib/doc/xml.cpp',
	'src/lib/hui/Controls/Control.cpp',
	'src/lib/hui/Controls/ControlButtonGtk.cpp',
	'src/lib/hui/Controls/ControlCheckBoxGtk.cpp',
	'src/lib/hui/Controls/ControlColorButtonGtk.cpp',
	'src/lib/hui/Controls/ControlComboBoxGtk.cpp',
	'src/lib/hui/Controls/ControlDrawingAreaGtk.cpp',
	'src/lib/hui/Controls/ControlEditGtk.cpp',
	'src/lib/hui/Controls/ControlExpanderGtk.cpp',
	'src/lib/hui/Controls/ControlGridGtk.cpp',
	'src/lib/hui/Controls/ControlGroupGtk.cpp',
	'src/lib/hui/Controls/ControlHeaderBar.cpp',
	'src/lib/hui/Controls/ControlLabelGtk.cpp',
	'src/lib/hui/Controls/ControlListViewGtk.cpp',
	'src/lib/hui/Controls/ControlMenuButtonGtk.cpp',
	'src/lib/hui/Controls/ControlMultilineEditGtk.cpp',
	'src/lib/hui/Controls/ControlPanedGtk.cpp',
	'src/lib/hui/Controls/ControlProgressBarGtk.cpp',
	'src/lib/hui/Controls/ControlRadioButtonGtk.cpp',
	'src/lib/hui/Controls/ControlScrollerGtk.cpp',
	'src/lib/hui/Controls/ControlSeparatorGtk.cpp',
	'src/lib/hui/Controls/ControlSliderGtk.cpp',
	'src/lib/hui/Controls/ControlSpinButtonGtk.cpp',
	'src/lib/hui/Controls/ControlTabControlGtk.cpp',
	'src/lib/hui/Controls/ControlToggleButtonGtk.cpp',
	'src/lib/hui/Controls/ControlTreeViewGtk.cpp',
	'src/lib/hui/Controls/MenuItemGtk.cpp',
	'src/lib/hui/Controls/MenuItemSeparatorGtk.cpp',
	'src/lib/hui/Controls/MenuItemSeparatorWin.cpp',
	'src/lib/hui/Controls/MenuItemSubmenuGtk.cpp',
	'src/lib/hui/Controls/MenuItemSubmenuWin.cpp',
	'src/lib/hui/Controls/MenuItemToggleGtk.cpp',
	'src/lib/hui/Controls/MenuItemToggleWin.cpp',
	'src/lib/hui/Controls/MenuItemWin.cpp',
	'src/lib/hui/Controls/ToolItemButtonGtk.cpp',
	'src/lib/hui/Controls/ToolItemButtonWin.cpp',
	'src/lib/hui/Controls/ToolItemMenuButtonGtk.cpp',
	'src/lib/hui/Controls/ToolItemMenuButtonWin.cpp',
	'src/lib/hui/Controls/ToolItemSeparatorGtk.cpp',
	'src/lib/hui/Controls/ToolItemSeparatorWin.cpp',
	'src/lib/hui/Controls/ToolItemToggleButtonGtk.cpp',
	'src/lib/hui/Controls/ToolItemToggleButtonWin.cpp',
	'src/lib/hui/Application.cpp',
	'src/lib/hui/Callback.cpp',
	'src/lib/hui/Clipboard.cpp',
	'src/lib/hui/common_dlg.cpp',
	'src/lib/hui/common_dlg_gtk.cpp',
	'src/lib/hui/common_dlg_win.cpp',
	'src/lib/hui/error.cpp',
	'src/lib/hui/Event.cpp',
	'src/lib/hui/hui.cpp',
	'src/lib/hui/language.cpp',
	'src/lib/hui/main_gtk.cpp',
	'src/lib/hui/main_win.cpp',
	'src/lib/hui/Menu.cpp',
	'src/lib/hui/MenuGtk.cpp',
	'src/lib/hui/MenuWin.cpp',
	'src/lib/hui/PainterGtk.cpp',
	'src/lib/hui/Panel.cpp',
	'src/lib/hui/Resource.cpp',
	'src/lib/hui/Toolbar.cpp',
	'src/lib/hui/ToolbarGtk.cpp',
	'src/lib/hui/ToolbarWin.cpp',
	'src/lib/hui/utility.cpp',
	'src/lib/hui/Window.cpp',
	'src/lib/hui/window_control.cpp',
	'src/lib/hui/window_control_gtk.cpp',
	'src/lib/hui/window_control_win.cpp',
	'src/lib/hui/WindowGtk.cpp',
	'src/lib/hui/WindowWin.cpp',
	'src/lib/image/color.cpp',
	'src/lib/image/image.cpp',
	'src/lib/image/image_bmp.cpp',
	'src/lib/image/image_jpg.cpp',
	'src/lib/image/image_png.cpp',
	'src/lib/image/image_tga.cpp',
	'src/lib/image/ImagePainter.cpp',
	'src/lib/kaba/asm/asm.cpp',
	'src/lib/kaba/asm/asm_arm.cpp',
	'src/lib/kaba/asm/asm_x86.cpp',
	'src/lib/kaba/compiler/Backend.cpp',
	'src/lib/kaba/compiler/BackendAmd64.cpp',
	'src/lib/kaba/compiler/BackendARM.cpp',
	'src/lib/kaba/compiler/BackendX86.cpp',
	'src/lib/kaba/compiler/CommandList.cpp',
	'src/lib/kaba/compiler/compiler.cpp',
	'src/lib/kaba/compiler/mapper.cpp',
	'src/lib/kaba/compiler/serializer.cpp',
	'src/lib/kaba/compiler/SerializerX.cpp',
	'src/lib/kaba/compiler/SerialNode.cpp',
	'src/lib/kaba/dynamic/call.cpp',
	'src/lib/kaba/dynamic/dynamic.cpp',
	'src/lib/kaba/dynamic/exception.cpp',
	'src/lib/kaba/lib/dict.cpp',
	'src/lib/kaba/lib/extern.cpp',
	'src/lib/kaba/lib/lib.cpp',
	'src/lib/kaba/lib/lib_base.cpp',
	'src/lib/kaba/lib/lib_doc.cpp',
	'src/lib/kaba/lib/lib_gl.cpp',
	'src/lib/kaba/lib/lib_hui.cpp',
	'src/lib/kaba/lib/lib_image.cpp',
	'src/lib/kaba/lib/lib_kaba.cpp',
	'src/lib/kaba/lib/lib_math.cpp',
	'src/lib/kaba/lib/lib_net.cpp',
	'src/lib/kaba/lib/lib_os.cpp',
	'src/lib/kaba/lib/lib_threads.cpp',
	'src/lib/kaba/lib/lib_time.cpp',
	'src/lib/kaba/lib/lib_vulkan.cpp',
	'src/lib/kaba/lib/list.cpp',
	'src/lib/kaba/parser/abstract.cpp',
	'src/lib/kaba/parser/Concretifier.cpp',
	'src/lib/kaba/parser/implicit.cpp',
	'src/lib/kaba/parser/import.cpp',
	'src/lib/kaba/parser/lexical.cpp',
	'src/lib/kaba/parser/macros.cpp',
	'src/lib/kaba/parser/Parser.cpp',
	'src/lib/kaba/parser/template.cpp',
	'src/lib/kaba/syntax/Block.cpp',
	'src/lib/kaba/syntax/Class.cpp',
	'src/lib/kaba/syntax/Constant.cpp',
	'src/lib/kaba/syntax/Function.cpp',
	'src/lib/kaba/syntax/Identifier.cpp',
	'src/lib/kaba/syntax/Node.cpp',
	'src/lib/kaba/syntax/Operator.cpp',
	'src/lib/kaba/syntax/preprocessor.cpp',
	'src/lib/kaba/syntax/Statement.cpp',
	'src/lib/kaba/syntax/SyntaxTree.cpp',
	'src/lib/kaba/syntax/Variable.cpp',
	'src/lib/kaba/CompilerConfiguration.cpp',
	'src/lib/kaba/Interpreter.cpp',
	'src/lib/kaba/kaba.cpp',
	'src/lib/math/complex.cpp',
	'src/lib/math/interpolation.cpp',
	'src/lib/math/math.cpp',
	'src/lib/math/matrix.cpp',
	'src/lib/math/matrix3.cpp',
	'src/lib/math/plane.cpp',
	'src/lib/math/quaternion.cpp',
	'src/lib/math/random.cpp',
	'src/lib/math/ray.cpp',
	'src/lib/math/rect.cpp',
	'src/lib/math/vec2.cpp',
	'src/lib/math/vec4.cpp',
	'src/lib/math/vector.cpp',
	'src/lib/net/BinaryBuffer.cpp',
	'src/lib/net/net.cpp',
	'src/lib/net/Socket.cpp',
	'src/lib/nix/nix.cpp',
	'src/lib/nix/nix_buffer.cpp',
	'src/lib/nix/nix_draw.cpp',
	'src/lib/nix/nix_framebuffer.cpp',
	'src/lib/nix/nix_light.cpp',
	'src/lib/nix/nix_shader.cpp',
	'src/lib/nix/nix_textures.cpp',
	'src/lib/nix/nix_vertexbuffer.cpp',
	'src/lib/nix/nix_view.cpp',
	'src/lib/os/CommandLineParser.cpp',
	'src/lib/os/date.cpp',
	'src/lib/os/file.cpp',
	'src/lib/os/filesystem.cpp',
	'src/lib/os/formatter.cpp',
	'src/lib/os/msg.cpp',
	'src/lib/os/path.cpp',
	'src/lib/os/stream.cpp',
	'src/lib/os/time.cpp',
	'src/lib/threads/Mutex.cpp',
	'src/lib/threads/Thread.cpp',
	'src/lib/threads/ThreadedWork.cpp',
	'src/lib/vulkan/AccelerationStructure.cpp',
	'src/lib/vulkan/Buffer.cpp',
	'src/lib/vulkan/CommandBuffer.cpp',
	'src/lib/vulkan/DescriptorSet.cpp',
	'src/lib/vulkan/Device.cpp',
	'src/lib/vulkan/FrameBuffer.cpp',
	'src/lib/vulkan/helper.cpp',
	'src/lib/vulkan/Instance.cpp',
	'src/lib/vulkan/Pipeline.cpp',
	'src/lib/vulkan/Queue.cpp',
	'src/lib/vulkan/RenderPass.cpp',
	'src/lib/vulkan/Semaphore.cpp',
	'src/lib/vulkan/Shader.cpp',
	'src/lib/vulkan/SwapChain.cpp',
	'src/lib/vulkan/Texture.cpp',
	'src/lib/vulkan/VertexBuffer.cpp',
	'src/lib/vulkan/vulkan.cpp',
	'src/main.cpp'
]


executable('kaba', sources, dependencies : deps, install : true)

install_data('static/icon.ico', install_dir : join_paths(get_option('datadir'), 'kaba'))
install_data('static/icon.png', install_dir : join_paths(get_option('datadir'), 'kaba'))
install_data('static/michisoft-kaba.desktop', install_dir : join_paths(get_option('datadir'), 'applications'))
