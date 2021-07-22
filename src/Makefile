BIN = ../kaba
CPP = g++
GLOBALFLAGS = -fno-omit-frame-pointer -O2 -g -std=c++17 -DHAS_LIB_GTK3=1 -DHAS_LIB_ZLIB=1 -DHAS_LIB_XLIB=1 -DHAS_LIB_UNWIND=1 -DHAS_LIB_DL=1 -DHAS_LIB_VULKAN=0 -DHAS_LIB_GL=1 -DHAS_LIB_GLFW=0
CXXFLAGS =  `pkg-config --cflags gtk+-3.0 zlib x11 gl` $(GLOBALFLAGS)
CXXFLAGSLINKING = -lunwind -ldl `pkg-config --cflags --libs gtk+-3.0 zlib x11 gl` $(GLOBALFLAGS)
prefix = /usr/local

OBJ = Helper/CLIParser.o \
 lib/algebra/crypto.o \
 lib/algebra/vli.o \
 lib/any/any.o \
 lib/base/array.o \
 lib/base/pointer.o \
 lib/base/strings.o \
 lib/file/file.o \
 lib/file/file_op.o \
 lib/file/msg.o \
 lib/file/path.o \
 lib/hui/Controls/Control.o \
 lib/hui/Controls/ControlButtonGtk.o \
 lib/hui/Controls/ControlCheckBoxGtk.o \
 lib/hui/Controls/ControlColorButtonGtk.o \
 lib/hui/Controls/ControlComboBoxGtk.o \
 lib/hui/Controls/ControlDrawingAreaGtk.o \
 lib/hui/Controls/ControlEditGtk.o \
 lib/hui/Controls/ControlExpanderGtk.o \
 lib/hui/Controls/ControlGridGtk.o \
 lib/hui/Controls/ControlGroupGtk.o \
 lib/hui/Controls/ControlLabelGtk.o \
 lib/hui/Controls/ControlListViewGtk.o \
 lib/hui/Controls/ControlMenuButtonGtk.o \
 lib/hui/Controls/ControlMultilineEditGtk.o \
 lib/hui/Controls/ControlPanedGtk.o \
 lib/hui/Controls/ControlProgressBarGtk.o \
 lib/hui/Controls/ControlRadioButtonGtk.o \
 lib/hui/Controls/ControlRevealerGtk.o \
 lib/hui/Controls/ControlScrollerGtk.o \
 lib/hui/Controls/ControlSeparatorGtk.o \
 lib/hui/Controls/ControlSliderGtk.o \
 lib/hui/Controls/ControlSpinButtonGtk.o \
 lib/hui/Controls/ControlTabControlGtk.o \
 lib/hui/Controls/ControlToggleButtonGtk.o \
 lib/hui/Controls/ControlTreeViewGtk.o \
 lib/hui/Controls/MenuItemGtk.o \
 lib/hui/Controls/MenuItemSeparatorGtk.o \
 lib/hui/Controls/MenuItemSeparatorWin.o \
 lib/hui/Controls/MenuItemSubmenuGtk.o \
 lib/hui/Controls/MenuItemSubmenuWin.o \
 lib/hui/Controls/MenuItemToggleGtk.o \
 lib/hui/Controls/MenuItemToggleWin.o \
 lib/hui/Controls/MenuItemWin.o \
 lib/hui/Controls/ToolItemButtonGtk.o \
 lib/hui/Controls/ToolItemButtonWin.o \
 lib/hui/Controls/ToolItemMenuButtonGtk.o \
 lib/hui/Controls/ToolItemMenuButtonWin.o \
 lib/hui/Controls/ToolItemSeparatorGtk.o \
 lib/hui/Controls/ToolItemSeparatorWin.o \
 lib/hui/Controls/ToolItemToggleButtonGtk.o \
 lib/hui/Controls/ToolItemToggleButtonWin.o \
 lib/hui/Application.o \
 lib/hui/Callback.o \
 lib/hui/Clipboard.o \
 lib/hui/common_dlg.o \
 lib/hui/common_dlg_gtk.o \
 lib/hui/common_dlg_win.o \
 lib/hui/Config.o \
 lib/hui/error.o \
 lib/hui/Event.o \
 lib/hui/hui.o \
 lib/hui/language.o \
 lib/hui/main_gtk.o \
 lib/hui/main_win.o \
 lib/hui/Menu.o \
 lib/hui/MenuGtk.o \
 lib/hui/MenuWin.o \
 lib/hui/PainterGtk.o \
 lib/hui/Panel.o \
 lib/hui/Resource.o \
 lib/hui/Timer.o \
 lib/hui/Toolbar.o \
 lib/hui/ToolbarGtk.o \
 lib/hui/ToolbarWin.o \
 lib/hui/utility.o \
 lib/hui/Window.o \
 lib/hui/window_control.o \
 lib/hui/window_control_gtk.o \
 lib/hui/window_control_win.o \
 lib/hui/WindowGtk.o \
 lib/hui/WindowWin.o \
 lib/image/color.o \
 lib/image/image.o \
 lib/image/image_bmp.o \
 lib/image/image_jpg.o \
 lib/image/image_png.o \
 lib/image/image_tga.o \
 lib/image/ImagePainter.o \
 lib/kaba/asm/asm.o \
 lib/kaba/asm/asm_arm.o \
 lib/kaba/asm/asm_x86.o \
 lib/kaba/compiler/Backend.o \
 lib/kaba/compiler/BackendAmd64.o \
 lib/kaba/compiler/BackendARM.o \
 lib/kaba/compiler/BackendX86.o \
 lib/kaba/compiler/CommandList.o \
 lib/kaba/compiler/compiler.o \
 lib/kaba/compiler/mapper.o \
 lib/kaba/compiler/serializer.o \
 lib/kaba/compiler/SerializerX.o \
 lib/kaba/compiler/SerialNode.o \
 lib/kaba/dynamic/call.o \
 lib/kaba/dynamic/dynamic.o \
 lib/kaba/dynamic/exception.o \
 lib/kaba/lib/dict.o \
 lib/kaba/lib/extern.o \
 lib/kaba/lib/lib.o \
 lib/kaba/lib/lib_base.o \
 lib/kaba/lib/lib_doc.o \
 lib/kaba/lib/lib_hui.o \
 lib/kaba/lib/lib_image.o \
 lib/kaba/lib/lib_kaba.o \
 lib/kaba/lib/lib_math.o \
 lib/kaba/lib/lib_net.o \
 lib/kaba/lib/lib_nix.o \
 lib/kaba/lib/lib_os.o \
 lib/kaba/lib/lib_threads.o \
 lib/kaba/lib/lib_time.o \
 lib/kaba/lib/lib_vulkan.o \
 lib/kaba/syntax/Block.o \
 lib/kaba/syntax/Class.o \
 lib/kaba/syntax/Constant.o \
 lib/kaba/syntax/Function.o \
 lib/kaba/syntax/Identifier.o \
 lib/kaba/syntax/implicit.o \
 lib/kaba/syntax/lexical.o \
 lib/kaba/syntax/Node.o \
 lib/kaba/syntax/Operator.o \
 lib/kaba/syntax/Parser.o \
 lib/kaba/syntax/precompiler.o \
 lib/kaba/syntax/preprocessor.o \
 lib/kaba/syntax/Statement.o \
 lib/kaba/syntax/SyntaxTree.o \
 lib/kaba/syntax/Variable.o \
 lib/kaba/CompilerConfiguration.o \
 lib/kaba/Interpreter.o \
 lib/kaba/kaba.o \
 lib/math/complex.o \
 lib/math/interpolation.o \
 lib/math/math.o \
 lib/math/matrix.o \
 lib/math/matrix3.o \
 lib/math/plane.o \
 lib/math/quaternion.o \
 lib/math/random.o \
 lib/math/ray.o \
 lib/math/rect.o \
 lib/math/vec2.o \
 lib/math/vec4.o \
 lib/math/vector.o \
 lib/net/BinaryBuffer.o \
 lib/net/net.o \
 lib/net/Socket.o \
 lib/nix/nix.o \
 lib/nix/nix_buffer.o \
 lib/nix/nix_draw.o \
 lib/nix/nix_framebuffer.o \
 lib/nix/nix_light.o \
 lib/nix/nix_shader.o \
 lib/nix/nix_textures.o \
 lib/nix/nix_vertexbuffer.o \
 lib/nix/nix_view.o \
 lib/threads/Mutex.o \
 lib/threads/Thread.o \
 lib/threads/ThreadedWork.o \
 lib/vulkan/AccelerationStructure.o \
 lib/vulkan/Buffer.o \
 lib/vulkan/CommandBuffer.o \
 lib/vulkan/DescriptorSet.o \
 lib/vulkan/Device.o \
 lib/vulkan/FrameBuffer.o \
 lib/vulkan/helper.o \
 lib/vulkan/Pipeline.o \
 lib/vulkan/RenderPass.o \
 lib/vulkan/Semaphore.o \
 lib/vulkan/Shader.o \
 lib/vulkan/SwapChain.o \
 lib/vulkan/Texture.o \
 lib/vulkan/VertexBuffer.o \
 lib/vulkan/vulkan.o \
 lib/xfile/chunked.o \
 lib/xfile/pdf.o \
 lib/xfile/xml.o

$(BIN) : $(OBJ) main.o
	$(CPP) $(OBJ) main.o -o $(BIN) $(CXXFLAGSLINKING)

all : $(BIN)

Helper/CLIParser.o : Helper/CLIParser.cpp Helper/CLIParser.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h
	$(CPP) -c Helper/CLIParser.cpp -o $@ $(CXXFLAGS)

lib/algebra/crypto.o : lib/algebra/crypto.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/algebra/vli.h lib/algebra/crypto.h lib/math/math.h
	$(CPP) -c lib/algebra/crypto.cpp -o $@ $(CXXFLAGS)

lib/algebra/vli.o : lib/algebra/vli.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/algebra/algebra.h lib/algebra/vli.h lib/algebra/crypto.h
	$(CPP) -c lib/algebra/vli.cpp -o $@ $(CXXFLAGS)

lib/any/any.o : lib/any/any.cpp lib/any/any.h lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/base/map.h lib/base/set.h lib/config.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h
	$(CPP) -c lib/any/any.cpp -o $@ $(CXXFLAGS)

lib/base/array.o : lib/base/array.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/base/array.cpp -o $@ $(CXXFLAGS)

lib/base/pointer.o : lib/base/pointer.cpp lib/base/pointer.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h
	$(CPP) -c lib/base/pointer.cpp -o $@ $(CXXFLAGS)

lib/base/strings.o : lib/base/strings.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/base/strings.cpp -o $@ $(CXXFLAGS)

lib/file/file.o : lib/file/file.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/file/file.cpp -o $@ $(CXXFLAGS)

lib/file/file_op.o : lib/file/file_op.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/file/file_op.cpp -o $@ $(CXXFLAGS)

lib/file/msg.o : lib/file/msg.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/file/msg.cpp -o $@ $(CXXFLAGS)

lib/file/path.o : lib/file/path.cpp lib/file/path.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file_op.h
	$(CPP) -c lib/file/path.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/Control.o : lib/hui/Controls/Control.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/Controls/ControlMultilineEdit.h
	$(CPP) -c lib/hui/Controls/Control.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlButtonGtk.o : lib/hui/Controls/ControlButtonGtk.cpp lib/hui/Controls/ControlButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Controls/ControlButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlCheckBoxGtk.o : lib/hui/Controls/ControlCheckBoxGtk.cpp lib/hui/Controls/ControlCheckBox.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Config.h lib/base/map.h lib/base/set.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlCheckBoxGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlColorButtonGtk.o : lib/hui/Controls/ControlColorButtonGtk.cpp lib/hui/Controls/ControlColorButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlColorButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlComboBoxGtk.o : lib/hui/Controls/ControlComboBoxGtk.cpp lib/hui/Controls/ControlComboBox.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlComboBoxGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlDrawingAreaGtk.o : lib/hui/Controls/ControlDrawingAreaGtk.cpp lib/hui/Controls/ControlDrawingArea.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h lib/math/rect.h lib/math/math.h
	$(CPP) -c lib/hui/Controls/ControlDrawingAreaGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlEditGtk.o : lib/hui/Controls/ControlEditGtk.cpp lib/hui/Controls/ControlEdit.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlEditGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlExpanderGtk.o : lib/hui/Controls/ControlExpanderGtk.cpp lib/hui/Controls/ControlExpander.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Window.h lib/hui/Event.h lib/hui/Callback.h lib/hui/Panel.h lib/hui/hui.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Menu.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Controls/ControlExpanderGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlGridGtk.o : lib/hui/Controls/ControlGridGtk.cpp lib/hui/Controls/ControlGrid.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Controls/ControlGridGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlGroupGtk.o : lib/hui/Controls/ControlGroupGtk.cpp lib/hui/Controls/ControlGroup.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Window.h lib/hui/Event.h lib/hui/Callback.h lib/hui/Panel.h lib/hui/hui.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Menu.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Controls/ControlGroupGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlLabelGtk.o : lib/hui/Controls/ControlLabelGtk.cpp lib/hui/Controls/ControlLabel.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ControlLabelGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlListViewGtk.o : lib/hui/Controls/ControlListViewGtk.cpp lib/hui/Controls/ControlListView.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Controls/ControlListViewGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlMenuButtonGtk.o : lib/hui/Controls/ControlMenuButtonGtk.cpp lib/hui/Controls/ControlMenuButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Menu.h lib/hui/Resource.h
	$(CPP) -c lib/hui/Controls/ControlMenuButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlMultilineEditGtk.o : lib/hui/Controls/ControlMultilineEditGtk.cpp lib/hui/Controls/ControlMultilineEdit.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlMultilineEditGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlPanedGtk.o : lib/hui/Controls/ControlPanedGtk.cpp lib/hui/Controls/ControlPaned.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ControlPanedGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlProgressBarGtk.o : lib/hui/Controls/ControlProgressBarGtk.cpp lib/hui/Controls/ControlProgressBar.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ControlProgressBarGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlRadioButtonGtk.o : lib/hui/Controls/ControlRadioButtonGtk.cpp lib/hui/Controls/ControlRadioButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Controls/ControlRadioButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlRevealerGtk.o : lib/hui/Controls/ControlRevealerGtk.cpp lib/hui/Controls/ControlRevealer.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ControlRevealerGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlScrollerGtk.o : lib/hui/Controls/ControlScrollerGtk.cpp lib/hui/Controls/ControlScroller.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ControlScrollerGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlSeparatorGtk.o : lib/hui/Controls/ControlSeparatorGtk.cpp lib/hui/Controls/ControlSeparator.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ControlSeparatorGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlSliderGtk.o : lib/hui/Controls/ControlSliderGtk.cpp lib/hui/Controls/ControlSlider.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlSliderGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlSpinButtonGtk.o : lib/hui/Controls/ControlSpinButtonGtk.cpp lib/hui/Controls/ControlSpinButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlSpinButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlTabControlGtk.o : lib/hui/Controls/ControlTabControlGtk.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/Controls/ControlTabControl.h lib/hui/Controls/Control.h
	$(CPP) -c lib/hui/Controls/ControlTabControlGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlToggleButtonGtk.o : lib/hui/Controls/ControlToggleButtonGtk.cpp lib/hui/Controls/ControlToggleButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ControlToggleButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ControlTreeViewGtk.o : lib/hui/Controls/ControlTreeViewGtk.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/Controls/ControlTreeView.h lib/hui/Controls/Control.h
	$(CPP) -c lib/hui/Controls/ControlTreeViewGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemGtk.o : lib/hui/Controls/MenuItemGtk.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h lib/hui/Controls/MenuItem.h lib/hui/Controls/Control.h
	$(CPP) -c lib/hui/Controls/MenuItemGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemSeparatorGtk.o : lib/hui/Controls/MenuItemSeparatorGtk.cpp lib/hui/Controls/MenuItemSeparator.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/MenuItemSeparatorGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemSeparatorWin.o : lib/hui/Controls/MenuItemSeparatorWin.cpp lib/hui/Controls/MenuItemSeparator.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/MenuItemSeparatorWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemSubmenuGtk.o : lib/hui/Controls/MenuItemSubmenuGtk.cpp lib/hui/Menu.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/Controls/MenuItemSubmenu.h lib/hui/Controls/Control.h lib/image/image.h lib/image/color.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/MenuItemSubmenuGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemSubmenuWin.o : lib/hui/Controls/MenuItemSubmenuWin.cpp lib/hui/Menu.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/Controls/MenuItemSubmenu.h lib/hui/Controls/Control.h lib/image/image.h lib/image/color.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/MenuItemSubmenuWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemToggleGtk.o : lib/hui/Controls/MenuItemToggleGtk.cpp lib/hui/Controls/MenuItemToggle.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/MenuItemToggleGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemToggleWin.o : lib/hui/Controls/MenuItemToggleWin.cpp lib/hui/Controls/MenuItemToggle.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/MenuItemToggleWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/MenuItemWin.o : lib/hui/Controls/MenuItemWin.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h lib/hui/Controls/MenuItem.h lib/hui/Controls/Control.h
	$(CPP) -c lib/hui/Controls/MenuItemWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemButtonGtk.o : lib/hui/Controls/ToolItemButtonGtk.cpp lib/hui/Controls/ToolItemButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Event.h lib/hui/Callback.h
	$(CPP) -c lib/hui/Controls/ToolItemButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemButtonWin.o : lib/hui/Controls/ToolItemButtonWin.cpp lib/hui/Controls/ToolItemButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemButtonWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemMenuButtonGtk.o : lib/hui/Controls/ToolItemMenuButtonGtk.cpp lib/hui/Menu.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/Controls/ToolItemMenuButton.h lib/hui/Controls/Control.h lib/image/image.h lib/image/color.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemMenuButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemMenuButtonWin.o : lib/hui/Controls/ToolItemMenuButtonWin.cpp lib/hui/Menu.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/Controls/ToolItemMenuButton.h lib/hui/Controls/Control.h lib/image/image.h lib/image/color.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemMenuButtonWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemSeparatorGtk.o : lib/hui/Controls/ToolItemSeparatorGtk.cpp lib/hui/Controls/ToolItemSeparator.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemSeparatorGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemSeparatorWin.o : lib/hui/Controls/ToolItemSeparatorWin.cpp lib/hui/Controls/ToolItemSeparator.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemSeparatorWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemToggleButtonGtk.o : lib/hui/Controls/ToolItemToggleButtonGtk.cpp lib/hui/Controls/ToolItemToggleButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemToggleButtonGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Controls/ToolItemToggleButtonWin.o : lib/hui/Controls/ToolItemToggleButtonWin.cpp lib/hui/Controls/ToolItemToggleButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h
	$(CPP) -c lib/hui/Controls/ToolItemToggleButtonWin.cpp -o $@ $(CXXFLAGS)

lib/hui/Application.o : lib/hui/Application.cpp lib/hui/Application.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/map.h lib/base/set.h lib/file/path.h lib/hui/common.h lib/config.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/hui/hui.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/Application.cpp -o $@ $(CXXFLAGS)

lib/hui/Callback.o : lib/hui/Callback.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Callback.cpp -o $@ $(CXXFLAGS)

lib/hui/Clipboard.o : lib/hui/Clipboard.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Clipboard.cpp -o $@ $(CXXFLAGS)

lib/hui/common_dlg.o : lib/hui/common_dlg.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/common_dlg.cpp -o $@ $(CXXFLAGS)

lib/hui/common_dlg_gtk.o : lib/hui/common_dlg_gtk.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/common_dlg_gtk.cpp -o $@ $(CXXFLAGS)

lib/hui/common_dlg_win.o : lib/hui/common_dlg_win.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/common_dlg_win.cpp -o $@ $(CXXFLAGS)

lib/hui/Config.o : lib/hui/Config.cpp lib/hui/Config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/map.h lib/base/set.h lib/file/path.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/hui/Config.cpp -o $@ $(CXXFLAGS)

lib/hui/error.o : lib/hui/error.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h lib/net/net.h lib/net/Socket.h lib/base/pointer.h lib/net/BinaryBuffer.h
	$(CPP) -c lib/hui/error.cpp -o $@ $(CXXFLAGS)

lib/hui/Event.o : lib/hui/Event.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Event.cpp -o $@ $(CXXFLAGS)

lib/hui/hui.o : lib/hui/hui.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/hui.cpp -o $@ $(CXXFLAGS)

lib/hui/language.o : lib/hui/language.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/language.cpp -o $@ $(CXXFLAGS)

lib/hui/main_gtk.o : lib/hui/main_gtk.cpp
	$(CPP) -c lib/hui/main_gtk.cpp -o $@ $(CXXFLAGS)

lib/hui/main_win.o : lib/hui/main_win.cpp
	$(CPP) -c lib/hui/main_win.cpp -o $@ $(CXXFLAGS)

lib/hui/Menu.o : lib/hui/Menu.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/Controls/MenuItem.h lib/hui/Controls/Control.h lib/hui/Controls/MenuItemSeparator.h lib/hui/Controls/MenuItemSubmenu.h lib/hui/Controls/MenuItemToggle.h
	$(CPP) -c lib/hui/Menu.cpp -o $@ $(CXXFLAGS)

lib/hui/MenuGtk.o : lib/hui/MenuGtk.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/MenuGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/MenuWin.o : lib/hui/MenuWin.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/MenuWin.cpp -o $@ $(CXXFLAGS)

lib/hui/PainterGtk.o : lib/hui/PainterGtk.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/math/vec2.h lib/math/math.h lib/math/rect.h lib/hui/Controls/Control.h lib/hui/Controls/ControlDrawingArea.h
	$(CPP) -c lib/hui/PainterGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/Panel.o : lib/hui/Panel.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/Panel.cpp -o $@ $(CXXFLAGS)

lib/hui/Resource.o : lib/hui/Resource.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/math/math.h lib/hui/Controls/Control.h lib/hui/internal.h
	$(CPP) -c lib/hui/Resource.cpp -o $@ $(CXXFLAGS)

lib/hui/Timer.o : lib/hui/Timer.cpp lib/hui/Timer.h lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/Timer.cpp -o $@ $(CXXFLAGS)

lib/hui/Toolbar.o : lib/hui/Toolbar.cpp lib/hui/Toolbar.h lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/Controls/ToolItemButton.h lib/hui/Controls/Control.h lib/hui/Controls/ToolItemMenuButton.h lib/hui/Controls/ToolItemSeparator.h lib/hui/Controls/ToolItemToggleButton.h
	$(CPP) -c lib/hui/Toolbar.cpp -o $@ $(CXXFLAGS)

lib/hui/ToolbarGtk.o : lib/hui/ToolbarGtk.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Toolbar.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/ToolbarGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/ToolbarWin.o : lib/hui/ToolbarWin.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/ToolbarWin.cpp -o $@ $(CXXFLAGS)

lib/hui/utility.o : lib/hui/utility.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/utility.cpp -o $@ $(CXXFLAGS)

lib/hui/Window.o : lib/hui/Window.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/Window.cpp -o $@ $(CXXFLAGS)

lib/hui/window_control.o : lib/hui/window_control.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/window_control.cpp -o $@ $(CXXFLAGS)

lib/hui/window_control_gtk.o : lib/hui/window_control_gtk.cpp lib/hui/Controls/ControlButton.h lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/Controls/ControlCheckBox.h lib/hui/Controls/ControlColorButton.h lib/hui/Controls/ControlComboBox.h lib/hui/Controls/ControlDrawingArea.h lib/hui/Controls/ControlEdit.h lib/hui/Controls/ControlExpander.h lib/hui/Controls/ControlGrid.h lib/hui/Controls/ControlGroup.h lib/hui/Controls/ControlLabel.h lib/hui/Controls/ControlListView.h lib/hui/Controls/ControlMultilineEdit.h lib/hui/Controls/ControlMenuButton.h lib/hui/Controls/ControlPaned.h lib/hui/Controls/ControlProgressBar.h lib/hui/Controls/ControlRadioButton.h lib/hui/Controls/ControlRevealer.h lib/hui/Controls/ControlScroller.h lib/hui/Controls/ControlSeparator.h lib/hui/Controls/ControlSlider.h lib/hui/Controls/ControlSpinButton.h lib/hui/Controls/ControlTabControl.h lib/hui/Controls/ControlToggleButton.h lib/hui/Controls/ControlTreeView.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/window_control_gtk.cpp -o $@ $(CXXFLAGS)

lib/hui/window_control_win.o : lib/hui/window_control_win.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/window_control_win.cpp -o $@ $(CXXFLAGS)

lib/hui/WindowGtk.o : lib/hui/WindowGtk.cpp lib/hui/Controls/Control.h lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/language.h lib/hui/hui.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/hui/internal.h
	$(CPP) -c lib/hui/WindowGtk.cpp -o $@ $(CXXFLAGS)

lib/hui/WindowWin.o : lib/hui/WindowWin.cpp lib/hui/hui.h lib/hui/common.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/hui/WindowWin.cpp -o $@ $(CXXFLAGS)

lib/image/color.o : lib/image/color.cpp lib/image/color.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/math.h
	$(CPP) -c lib/image/color.cpp -o $@ $(CXXFLAGS)

lib/image/image.o : lib/image/image.cpp lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/math/math.h lib/image/image_bmp.h lib/image/image_tga.h lib/image/image_jpg.h lib/image/image_png.h lib/image/ImagePainter.h lib/image/Painter.h lib/math/rect.h
	$(CPP) -c lib/image/image.cpp -o $@ $(CXXFLAGS)

lib/image/image_bmp.o : lib/image/image_bmp.cpp lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/hui.h lib/hui/common.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/image/image_bmp.cpp -o $@ $(CXXFLAGS)

lib/image/image_jpg.o : lib/image/image_jpg.cpp lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/math/math.h
	$(CPP) -c lib/image/image_jpg.cpp -o $@ $(CXXFLAGS)

lib/image/image_png.o : lib/image/image_png.cpp lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/image/image_png.cpp -o $@ $(CXXFLAGS)

lib/image/image_tga.o : lib/image/image_tga.cpp lib/image/image.h lib/config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/image/image_tga.cpp -o $@ $(CXXFLAGS)

lib/image/ImagePainter.o : lib/image/ImagePainter.cpp lib/image/ImagePainter.h lib/image/Painter.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/math/rect.h lib/math/math.h lib/image/image.h lib/config.h lib/math/vec2.h
	$(CPP) -c lib/image/ImagePainter.cpp -o $@ $(CXXFLAGS)

lib/kaba/asm/asm.o : lib/kaba/asm/asm.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/asm/internal.h
	$(CPP) -c lib/kaba/asm/asm.cpp -o $@ $(CXXFLAGS)

lib/kaba/asm/asm_arm.o : lib/kaba/asm/asm_arm.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/kaba/asm/asm.h lib/kaba/asm/internal.h
	$(CPP) -c lib/kaba/asm/asm_arm.cpp -o $@ $(CXXFLAGS)

lib/kaba/asm/asm_x86.o : lib/kaba/asm/asm_x86.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/kaba/asm/asm.h lib/kaba/asm/internal.h
	$(CPP) -c lib/kaba/asm/asm_x86.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/Backend.o : lib/kaba/compiler/Backend.cpp lib/kaba/compiler/Backend.h lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/file/msg.h
	$(CPP) -c lib/kaba/compiler/Backend.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/BackendAmd64.o : lib/kaba/compiler/BackendAmd64.cpp lib/kaba/compiler/BackendAmd64.h lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/kaba/compiler/BackendX86.h lib/kaba/compiler/Backend.h lib/file/msg.h
	$(CPP) -c lib/kaba/compiler/BackendAmd64.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/BackendARM.o : lib/kaba/compiler/BackendARM.cpp lib/kaba/compiler/BackendARM.h lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/Backend.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/file/msg.h
	$(CPP) -c lib/kaba/compiler/BackendARM.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/BackendX86.o : lib/kaba/compiler/BackendX86.cpp lib/kaba/compiler/BackendX86.h lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/Backend.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/file/msg.h
	$(CPP) -c lib/kaba/compiler/BackendX86.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/CommandList.o : lib/kaba/compiler/CommandList.cpp lib/kaba/compiler/CommandList.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/compiler/SerialNode.h lib/kaba/compiler/serializer.h lib/kaba/kaba.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/msg.h
	$(CPP) -c lib/kaba/compiler/CommandList.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/compiler.o : lib/kaba/compiler/compiler.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/base/set.h
	$(CPP) -c lib/kaba/compiler/compiler.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/mapper.o : lib/kaba/compiler/mapper.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/compiler/mapper.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/serializer.o : lib/kaba/compiler/serializer.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/kaba/compiler/BackendAmd64.h lib/kaba/compiler/BackendX86.h lib/kaba/compiler/Backend.h lib/kaba/compiler/BackendARM.h lib/kaba/Interpreter.h
	$(CPP) -c lib/kaba/compiler/serializer.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/SerializerX.o : lib/kaba/compiler/SerializerX.cpp lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/compiler/SerialNode.h lib/kaba/kaba.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/msg.h
	$(CPP) -c lib/kaba/compiler/SerializerX.cpp -o $@ $(CXXFLAGS)

lib/kaba/compiler/SerialNode.o : lib/kaba/compiler/SerialNode.cpp lib/kaba/compiler/SerialNode.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/kaba.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h
	$(CPP) -c lib/kaba/compiler/SerialNode.cpp -o $@ $(CXXFLAGS)

lib/kaba/dynamic/call.o : lib/kaba/dynamic/call.cpp lib/kaba/syntax/Function.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/kaba/syntax/Class.h lib/kaba/CompilerConfiguration.h lib/file/path.h lib/file/msg.h
	$(CPP) -c lib/kaba/dynamic/call.cpp -o $@ $(CXXFLAGS)

lib/kaba/dynamic/dynamic.o : lib/kaba/dynamic/dynamic.cpp lib/kaba/dynamic/dynamic.h lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/dynamic/exception.h lib/kaba/dynamic/call.h lib/any/any.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/dynamic/dynamic.cpp -o $@ $(CXXFLAGS)

lib/kaba/dynamic/exception.o : lib/kaba/dynamic/exception.cpp lib/kaba/dynamic/exception.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/kaba.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/msg.h
	$(CPP) -c lib/kaba/dynamic/exception.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/dict.o : lib/kaba/lib/dict.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/dynamic/exception.h lib/kaba/dynamic/dynamic.h lib/base/map.h lib/base/set.h
	$(CPP) -c lib/kaba/lib/dict.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/extern.o : lib/kaba/lib/extern.cpp lib/kaba/lib/extern.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/file/path.h lib/kaba/syntax/Flags.h lib/base/pointer.h lib/kaba/kaba.h lib/kaba/asm/asm.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/msg.h
	$(CPP) -c lib/kaba/lib/extern.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib.o : lib/kaba/lib/lib.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/lib/dict.h lib/kaba/dynamic/exception.h lib/config.h lib/math/complex.h lib/math/math.h lib/any/any.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/hui/hui.h lib/hui/common.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/kaba/lib/lib.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_base.o : lib/kaba/lib/lib_base.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/dynamic/exception.h lib/kaba/dynamic/dynamic.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/lib/lib_base.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_doc.o : lib/kaba/lib/lib_doc.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/xfile/pdf.h lib/image/Painter.h
	$(CPP) -c lib/kaba/lib/lib_doc.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_hui.o : lib/kaba/lib/lib_hui.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/hui/hui.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/kaba/lib/lib_hui.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_image.o : lib/kaba/lib/lib_image.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/image/image.h lib/image/color.h lib/image/ImagePainter.h lib/image/Painter.h lib/math/rect.h lib/math/math.h
	$(CPP) -c lib/kaba/lib/lib_image.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_kaba.o : lib/kaba/lib/lib_kaba.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/dynamic/exception.h lib/kaba/dynamic/dynamic.h
	$(CPP) -c lib/kaba/lib/lib_kaba.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_math.o : lib/kaba/lib/lib_math.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/math/math.h lib/math/vec2.h lib/math/vector.h lib/math/complex.h lib/math/quaternion.h lib/math/matrix.h lib/math/matrix3.h lib/math/plane.h lib/math/rect.h lib/math/interpolation.h lib/math/random.h lib/image/color.h lib/base/map.h lib/base/set.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/kaba/dynamic/exception.h lib/algebra/algebra.h lib/algebra/vli.h lib/algebra/crypto.h lib/any/any.h
	$(CPP) -c lib/kaba/lib/lib_math.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_net.o : lib/kaba/lib/lib_net.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/kaba/dynamic/exception.h lib/net/net.h lib/net/Socket.h lib/net/BinaryBuffer.h
	$(CPP) -c lib/kaba/lib/lib_net.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_nix.o : lib/kaba/lib/lib_nix.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/kaba/dynamic/exception.h lib/nix/nix.h lib/nix/nix_config.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h
	$(CPP) -c lib/kaba/lib/lib_nix.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_os.o : lib/kaba/lib/lib_os.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/kaba/dynamic/exception.h
	$(CPP) -c lib/kaba/lib/lib_os.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_threads.o : lib/kaba/lib/lib_threads.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/threads/Thread.h lib/threads/Mutex.h lib/threads/ThreadedWork.h
	$(CPP) -c lib/kaba/lib/lib_threads.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_time.o : lib/kaba/lib/lib_time.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/hui/hui.h lib/hui/common.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/kaba/lib/lib_time.cpp -o $@ $(CXXFLAGS)

lib/kaba/lib/lib_vulkan.o : lib/kaba/lib/lib_vulkan.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/config.h lib/math/matrix.h lib/math/math.h lib/kaba/dynamic/exception.h lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/vulkan/helper.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/vulkan/Texture.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/vulkan/AccelerationStructure.h
	$(CPP) -c lib/kaba/lib/lib_vulkan.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Block.o : lib/kaba/syntax/Block.cpp lib/kaba/syntax/Block.h lib/kaba/syntax/Node.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/kaba/syntax/Flags.h lib/kaba/syntax/Function.h lib/kaba/syntax/Variable.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h
	$(CPP) -c lib/kaba/syntax/Block.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Class.o : lib/kaba/syntax/Class.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/kaba.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/Class.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Constant.o : lib/kaba/syntax/Constant.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/dynamic/dynamic.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/Constant.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Function.o : lib/kaba/syntax/Function.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/base/set.h
	$(CPP) -c lib/kaba/syntax/Function.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Identifier.o : lib/kaba/syntax/Identifier.cpp lib/kaba/syntax/Identifier.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/kaba/syntax/Identifier.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/implicit.o : lib/kaba/syntax/implicit.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/syntax/Parser.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/implicit.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/lexical.o : lib/kaba/syntax/lexical.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/syntax/lexical.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/Class.h lib/base/pointer.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Flags.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h
	$(CPP) -c lib/kaba/syntax/lexical.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Node.o : lib/kaba/syntax/Node.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/Node.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Operator.o : lib/kaba/syntax/Operator.cpp lib/kaba/syntax/Operator.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/syntax/Identifier.h
	$(CPP) -c lib/kaba/syntax/Operator.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Parser.o : lib/kaba/syntax/Parser.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/kaba/syntax/Parser.h lib/config.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/common.h
	$(CPP) -c lib/kaba/syntax/Parser.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/precompiler.o : lib/kaba/syntax/precompiler.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/syntax/Parser.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/precompiler.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/preprocessor.o : lib/kaba/syntax/preprocessor.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/dynamic/call.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/preprocessor.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Statement.o : lib/kaba/syntax/Statement.cpp lib/kaba/syntax/Statement.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/syntax/Identifier.h
	$(CPP) -c lib/kaba/syntax/Statement.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/SyntaxTree.o : lib/kaba/syntax/SyntaxTree.cpp lib/kaba/kaba.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/syntax/Parser.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/kaba/syntax/SyntaxTree.cpp -o $@ $(CXXFLAGS)

lib/kaba/syntax/Variable.o : lib/kaba/syntax/Variable.cpp lib/kaba/syntax/Variable.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/kaba/syntax/Flags.h
	$(CPP) -c lib/kaba/syntax/Variable.cpp -o $@ $(CXXFLAGS)

lib/kaba/CompilerConfiguration.o : lib/kaba/CompilerConfiguration.cpp lib/kaba/CompilerConfiguration.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/syntax/Function.h lib/base/pointer.h
	$(CPP) -c lib/kaba/CompilerConfiguration.cpp -o $@ $(CXXFLAGS)

lib/kaba/Interpreter.o : lib/kaba/Interpreter.cpp lib/kaba/Interpreter.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/kaba/kaba.h lib/base/pointer.h lib/file/path.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/compiler/serializer.h lib/kaba/compiler/CommandList.h lib/kaba/compiler/SerialNode.h lib/file/msg.h
	$(CPP) -c lib/kaba/Interpreter.cpp -o $@ $(CXXFLAGS)

lib/kaba/kaba.o : lib/kaba/kaba.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/kaba/kaba.h lib/base/pointer.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/syntax/Parser.h lib/kaba/Interpreter.h lib/config.h
	$(CPP) -c lib/kaba/kaba.cpp -o $@ $(CXXFLAGS)

lib/math/complex.o : lib/math/complex.cpp lib/math/complex.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/math/complex.cpp -o $@ $(CXXFLAGS)

lib/math/interpolation.o : lib/math/interpolation.cpp lib/math/interpolation.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/complex.h lib/math/math.h lib/math/vector.h lib/math/quaternion.h lib/file/msg.h
	$(CPP) -c lib/math/interpolation.cpp -o $@ $(CXXFLAGS)

lib/math/math.o : lib/math/math.cpp lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/math/math.cpp -o $@ $(CXXFLAGS)

lib/math/matrix.o : lib/math/matrix.cpp lib/math/matrix.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/plane.h lib/math/quaternion.h lib/file/msg.h
	$(CPP) -c lib/math/matrix.cpp -o $@ $(CXXFLAGS)

lib/math/matrix3.o : lib/math/matrix3.cpp lib/math/matrix3.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/quaternion.h lib/file/msg.h
	$(CPP) -c lib/math/matrix3.cpp -o $@ $(CXXFLAGS)

lib/math/plane.o : lib/math/plane.cpp lib/math/plane.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/matrix.h
	$(CPP) -c lib/math/plane.cpp -o $@ $(CXXFLAGS)

lib/math/quaternion.o : lib/math/quaternion.cpp lib/math/quaternion.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/matrix.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/math/quaternion.cpp -o $@ $(CXXFLAGS)

lib/math/random.o : lib/math/random.cpp lib/math/random.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/math/random.cpp -o $@ $(CXXFLAGS)

lib/math/ray.o : lib/math/ray.cpp lib/math/ray.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/plane.h
	$(CPP) -c lib/math/ray.cpp -o $@ $(CXXFLAGS)

lib/math/rect.o : lib/math/rect.cpp lib/math/rect.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vec2.h
	$(CPP) -c lib/math/rect.cpp -o $@ $(CXXFLAGS)

lib/math/vec2.o : lib/math/vec2.cpp lib/math/vec2.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/math/vec2.cpp -o $@ $(CXXFLAGS)

lib/math/vec4.o : lib/math/vec4.cpp lib/math/vec4.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h
	$(CPP) -c lib/math/vec4.cpp -o $@ $(CXXFLAGS)

lib/math/vector.o : lib/math/vector.cpp lib/math/vector.h lib/math/math.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/matrix.h lib/math/plane.h
	$(CPP) -c lib/math/vector.cpp -o $@ $(CXXFLAGS)

lib/net/BinaryBuffer.o : lib/net/BinaryBuffer.cpp lib/net/BinaryBuffer.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/math.h
	$(CPP) -c lib/net/BinaryBuffer.cpp -o $@ $(CXXFLAGS)

lib/net/net.o : lib/net/net.cpp lib/net/net.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/net/Socket.h lib/base/pointer.h lib/net/BinaryBuffer.h lib/file/msg.h
	$(CPP) -c lib/net/net.cpp -o $@ $(CXXFLAGS)

lib/net/Socket.o : lib/net/Socket.cpp lib/net/net.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/net/Socket.h lib/base/pointer.h lib/net/BinaryBuffer.h lib/file/msg.h
	$(CPP) -c lib/net/Socket.cpp -o $@ $(CXXFLAGS)

lib/nix/nix.o : lib/nix/nix.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h lib/file/msg.h
	$(CPP) -c lib/nix/nix.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_buffer.o : lib/nix/nix_buffer.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h
	$(CPP) -c lib/nix/nix_buffer.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_draw.o : lib/nix/nix_draw.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h
	$(CPP) -c lib/nix/nix_draw.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_framebuffer.o : lib/nix/nix_framebuffer.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h lib/file/msg.h
	$(CPP) -c lib/nix/nix_framebuffer.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_light.o : lib/nix/nix_light.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h
	$(CPP) -c lib/nix/nix_light.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_shader.o : lib/nix/nix_shader.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/nix/nix_shader.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_textures.o : lib/nix/nix_textures.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h lib/image/image.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/nix/nix_textures.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_vertexbuffer.o : lib/nix/nix_vertexbuffer.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h
	$(CPP) -c lib/nix/nix_vertexbuffer.cpp -o $@ $(CXXFLAGS)

lib/nix/nix_view.o : lib/nix/nix_view.cpp lib/nix/nix.h lib/config.h lib/nix/nix_config.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/image/color.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/file/path.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/nix/nix_common.h lib/hui/hui.h lib/hui/common.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/image/image.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h
	$(CPP) -c lib/nix/nix_view.cpp -o $@ $(CXXFLAGS)

lib/threads/Mutex.o : lib/threads/Mutex.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/threads/Mutex.h lib/threads/Thread.h
	$(CPP) -c lib/threads/Mutex.cpp -o $@ $(CXXFLAGS)

lib/threads/Thread.o : lib/threads/Thread.cpp lib/threads/Thread.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/threads/Thread.cpp -o $@ $(CXXFLAGS)

lib/threads/ThreadedWork.o : lib/threads/ThreadedWork.cpp lib/file/file.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/threads/Mutex.h lib/threads/Thread.h lib/threads/ThreadedWork.h
	$(CPP) -c lib/threads/ThreadedWork.cpp -o $@ $(CXXFLAGS)

lib/vulkan/AccelerationStructure.o : lib/vulkan/AccelerationStructure.cpp lib/vulkan/AccelerationStructure.h lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/helper.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/math/matrix.h
	$(CPP) -c lib/vulkan/AccelerationStructure.cpp -o $@ $(CXXFLAGS)

lib/vulkan/Buffer.o : lib/vulkan/Buffer.cpp lib/vulkan/Buffer.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/helper.h
	$(CPP) -c lib/vulkan/Buffer.cpp -o $@ $(CXXFLAGS)

lib/vulkan/CommandBuffer.o : lib/vulkan/CommandBuffer.cpp lib/vulkan/CommandBuffer.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/helper.h lib/vulkan/DescriptorSet.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Buffer.h lib/vulkan/Pipeline.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Shader.h lib/base/pointer.h lib/file/path.h lib/vulkan/FrameBuffer.h lib/vulkan/Texture.h lib/vulkan/vulkan.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/vulkan/AccelerationStructure.h lib/math/rect.h
	$(CPP) -c lib/vulkan/CommandBuffer.cpp -o $@ $(CXXFLAGS)

lib/vulkan/DescriptorSet.o : lib/vulkan/DescriptorSet.cpp lib/vulkan/DescriptorSet.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/helper.h lib/vulkan/AccelerationStructure.h lib/vulkan/vulkan.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/file/file.h lib/file/msg.h lib/file/file_op.h
	$(CPP) -c lib/vulkan/DescriptorSet.cpp -o $@ $(CXXFLAGS)

lib/vulkan/Device.o : lib/vulkan/Device.cpp lib/vulkan/Device.h
	$(CPP) -c lib/vulkan/Device.cpp -o $@ $(CXXFLAGS)

lib/vulkan/FrameBuffer.o : lib/vulkan/FrameBuffer.cpp lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/helper.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/vulkan/AccelerationStructure.h
	$(CPP) -c lib/vulkan/FrameBuffer.cpp -o $@ $(CXXFLAGS)

lib/vulkan/helper.o : lib/vulkan/helper.cpp lib/vulkan/helper.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/CommandBuffer.h
	$(CPP) -c lib/vulkan/helper.cpp -o $@ $(CXXFLAGS)

lib/vulkan/Pipeline.o : lib/vulkan/Pipeline.cpp lib/vulkan/Pipeline.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/Buffer.h lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/vulkan/helper.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/vulkan/AccelerationStructure.h lib/file/msg.h lib/math/rect.h
	$(CPP) -c lib/vulkan/Pipeline.cpp -o $@ $(CXXFLAGS)

lib/vulkan/RenderPass.o : lib/vulkan/RenderPass.cpp lib/vulkan/RenderPass.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/vulkan/helper.h
	$(CPP) -c lib/vulkan/RenderPass.cpp -o $@ $(CXXFLAGS)

lib/vulkan/Semaphore.o : lib/vulkan/Semaphore.cpp lib/vulkan/Semaphore.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/vulkan/helper.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/AccelerationStructure.h
	$(CPP) -c lib/vulkan/Semaphore.cpp -o $@ $(CXXFLAGS)

lib/vulkan/Shader.o : lib/vulkan/Shader.cpp lib/vulkan/Shader.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/vulkan/helper.h lib/vulkan/DescriptorSet.h lib/vulkan/Texture.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/image/image.h lib/config.h lib/image/color.h
	$(CPP) -c lib/vulkan/Shader.cpp -o $@ $(CXXFLAGS)

lib/vulkan/SwapChain.o : lib/vulkan/SwapChain.cpp lib/vulkan/SwapChain.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/image/color.h lib/vulkan/FrameBuffer.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/vulkan/helper.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/Semaphore.h lib/vulkan/AccelerationStructure.h
	$(CPP) -c lib/vulkan/SwapChain.cpp -o $@ $(CXXFLAGS)

lib/vulkan/Texture.o : lib/vulkan/Texture.cpp lib/vulkan/Texture.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/file/path.h lib/vulkan/helper.h lib/vulkan/CommandBuffer.h lib/vulkan/Buffer.h lib/image/image.h lib/config.h lib/image/color.h
	$(CPP) -c lib/vulkan/Texture.cpp -o $@ $(CXXFLAGS)

lib/vulkan/VertexBuffer.o : lib/vulkan/VertexBuffer.cpp lib/vulkan/VertexBuffer.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/math/vector.h lib/math/math.h lib/vulkan/Buffer.h lib/vulkan/helper.h
	$(CPP) -c lib/vulkan/VertexBuffer.cpp -o $@ $(CXXFLAGS)

lib/vulkan/vulkan.o : lib/vulkan/vulkan.cpp lib/vulkan/vulkan.h lib/vulkan/DescriptorSet.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/vulkan/helper.h lib/vulkan/Buffer.h lib/vulkan/VertexBuffer.h lib/math/vector.h lib/math/math.h lib/vulkan/Texture.h lib/base/pointer.h lib/file/path.h lib/vulkan/Shader.h lib/vulkan/RenderPass.h lib/image/color.h lib/vulkan/Pipeline.h lib/vulkan/CommandBuffer.h lib/vulkan/FrameBuffer.h lib/vulkan/SwapChain.h lib/vulkan/Semaphore.h lib/vulkan/AccelerationStructure.h lib/file/msg.h
	$(CPP) -c lib/vulkan/vulkan.cpp -o $@ $(CXXFLAGS)

lib/xfile/chunked.o : lib/xfile/chunked.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/xfile/chunked.h
	$(CPP) -c lib/xfile/chunked.cpp -o $@ $(CXXFLAGS)

lib/xfile/pdf.o : lib/xfile/pdf.cpp lib/xfile/pdf.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/base/pointer.h lib/image/Painter.h lib/image/image.h lib/config.h lib/image/color.h lib/math/rect.h lib/math/math.h lib/math/vec2.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/xfile/pdf.cpp -o $@ $(CXXFLAGS)

lib/xfile/xml.o : lib/xfile/xml.cpp lib/xfile/xml.h lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h
	$(CPP) -c lib/xfile/xml.cpp -o $@ $(CXXFLAGS)

main.o : main.cpp lib/base/base.h lib/base/macros.h lib/base/array.h lib/base/strings.h lib/file/file.h lib/file/msg.h lib/file/file_op.h lib/file/path.h lib/hui/hui.h lib/hui/common.h lib/config.h lib/image/image.h lib/image/color.h lib/hui/Callback.h lib/hui/Application.h lib/base/map.h lib/base/set.h lib/hui/main.h lib/hui/Event.h lib/hui/Menu.h lib/hui/Panel.h lib/hui/Window.h lib/hui/common_dlg.h lib/hui/language.h lib/hui/Config.h lib/hui/Resource.h lib/hui/utility.h lib/hui/Painter.h lib/image/Painter.h lib/hui/Timer.h lib/hui/Toolbar.h lib/hui/error.h lib/hui/Clipboard.h lib/nix/nix.h lib/nix/nix_config.h lib/nix/nix_draw.h lib/nix/nix_light.h lib/math/matrix.h lib/math/math.h lib/math/vector.h lib/nix/nix_vertexbuffer.h lib/math/rect.h lib/nix/nix_shader.h lib/base/pointer.h lib/nix/nix_view.h lib/nix/nix_textures.h lib/nix/nix_framebuffer.h lib/nix/nix_buffer.h lib/net/net.h lib/net/Socket.h lib/net/BinaryBuffer.h lib/kaba/kaba.h lib/kaba/asm/asm.h lib/kaba/lib/lib.h lib/kaba/CompilerConfiguration.h lib/kaba/syntax/Flags.h lib/kaba/lib/extern.h lib/kaba/syntax/SyntaxTree.h lib/kaba/syntax/lexical.h lib/kaba/syntax/Class.h lib/kaba/syntax/Constant.h lib/kaba/syntax/Function.h lib/kaba/syntax/Node.h lib/kaba/syntax/Block.h lib/kaba/syntax/Operator.h lib/kaba/syntax/Variable.h lib/kaba/syntax/Identifier.h lib/kaba/syntax/Inline.h lib/kaba/syntax/Statement.h lib/kaba/Interpreter.h Helper/CLIParser.h lib/math/complex.h
	$(CPP) -c main.cpp -o $@ $(CXXFLAGS)

install :
	mkdir -p $(prefix)/share/kaba
	xdg-desktop-menu install ../static/michisoft-kaba.desktop
	xdg-mime install ../static/michisoft-kaba.xml
	xdg-icon-resource install --context mimetypes --size 64 ../static/icon.png text-michisoft-kaba
	cp -uf '../static/icon.ico' $(prefix)/share/kaba
	cp -uf '../static/icon.png' $(prefix)/share/kaba
	cp $(BIN) $(prefix)/bin

clean :
	rm -f $(BIN)
	find . -name "*.o" -exec rm -f {} \;
	find . -name "*.a" -exec rm -f {} \;

