#
# This file contains lists of object files for our build targets.
# Please keep every file in separate lines to make diff and patching easier.
#

set( FROGATTO_SRC_PLATFORM "" )
set( FROGATTO_LIB_PLATFORM "" )
if( WIN32 )
    set( FROGATTO_SRC_PLATFORM frogatto-resouces.rc )
    set( FROGATTO_LIB_PLATFORM shlwapi.lib pthreadVC2.lib glew32.lib )
endif( WIN32 )

# Game engine
add_executable( frogatto 
	IMG_savepng.cpp
	achievements.cpp
	animation_preview_widget.cpp
    asserts.cpp
	background.cpp
	blur.cpp
	border_widget.cpp
	button.cpp
	character_editor_dialog.cpp
	checkbox.cpp
	checksum.cpp
	code_editor_widget.cpp
	code_editor_dialog.cpp
	current_generator.cpp
	decimal.cpp
	editor_dialogs.cpp
	editor_formula_functions.cpp
	editor_layers_dialog.cpp
	editor_stats_dialog.cpp
	editor_variable_info.cpp
	external_text_editor.cpp
    clipboard.cpp
	collision_utils.cpp
	color_utils.cpp
	controls.cpp
	controls_dialog.cpp
	custom_object.cpp
	custom_object_callable.cpp
	custom_object_functions.cpp
	custom_object_type.cpp
	debug_console.cpp
	dialog.cpp
	draw_number.cpp
	draw_scene.cpp
	draw_tile.cpp
	editor.cpp
	editor_level_properties_dialog.cpp
	entity.cpp
	filesystem.cpp
	font.cpp
	formula.cpp
	formula_callable_definition.cpp
	formula_constants.cpp
	formula_function.cpp
    formula_function_registry.cpp
	formula_profiler.cpp
	formula_tokenizer.cpp
	formula_variable_storage.cpp
	frame.cpp
	framed_gui_element.cpp
	game_registry.cpp
	geometry.cpp
	globals.cpp
	graphical_font.cpp
	graphical_font_label.cpp
	grid_widget.cpp
	group_property_editor_dialog.cpp
	gui_formula_functions.cpp
	gui_section.cpp
	i18n.cpp
	image_widget.cpp
	input.cpp
	iphone_controls.cpp
	joystick.cpp
	json_parser.cpp
	json_tokenizer.cpp
	key.cpp
	key_button.cpp
	label.cpp
	level.cpp
	level_logic.cpp
	level_object.cpp
	level_runner.cpp
	level_solid_map.cpp
	light.cpp
	load_level.cpp
	main.cpp
	md5.cpp
	message_dialog.cpp
    module.cpp
	movement_script.cpp
	multi_tile_pattern.cpp
	multiplayer.cpp
	object_events.cpp
	options_dialog.cpp
	package.cpp
	particle_system.cpp
	pathfinding.cpp
	pause_game_dialog.cpp
	playable_custom_object.cpp
	player_info.cpp
	preferences.cpp
	preprocessor.cpp
	preview_tileset_widget.cpp
	property_editor_dialog.cpp
	random.cpp
	raster.cpp
	raster_distortion.cpp
	rectangle_rotator.cpp
	scrollbar_widget.cpp
	scrollable_widget.cpp
	segment_editor_dialog.cpp
	settings_dialog.cpp
	slider.cpp
	solid_map.cpp
	sound.cpp
	speech_dialog.cpp
	stats.cpp
	string_utils.cpp
	surface_cache.cpp
	surface_formula.cpp
	surface_palette.cpp
	surface_scaling.cpp
	surface.cpp
    text_editor_widget.cpp
    text_entry_widget.cpp
	texture.cpp
	texture_frame_buffer.cpp
	text_entry_widget.cpp
	thread.cpp
	tile_map.cpp
	tileset_editor_dialog.cpp
	tooltip.cpp
	translate.cpp
	utils.cpp
	variant.cpp
	variant_callable.cpp
	variant_utils.cpp
	water.cpp
	water_particle_system.cpp
	weather_particle_system.cpp
	widget.cpp
	unit_test.cpp
	formula_test.cpp
	loading_screen.cpp
	utility_object_compiler.cpp
	utility_query.cpp
	utility_render_level.cpp
    wml_formula_callable.cpp
    ${FROGATTO_SRC_PLATFORM} )

target_link_libraries( frogatto ${OPENGL_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLTTF_LIBRARY} ${SDLMIXER_LIBRARY} ${FROGATTO_LIB_PLATFORM} )

# Server
add_executable( server 
    server.cpp 
    simple_wml.cpp )

#
## Unit test
#add_executable( formula_test_objects 
#    filesystem.cpp
#    formula_function.cpp
#    formula_tokenizer.cpp
#    string_utils.cpp
#    variant.cpp
#    wml_node.cpp
#    wml_parser.cpp 
#    wml_utils.cpp
#    wml_writer.cpp )
#    
## Unit test
#add_executable( wml_modify_test_objects 
#	filesystem.cpp
#	string_utils.cpp
#	wml_node.cpp
#	wml_parser.cpp
#	wml_utils.cpp )
#
## Unit test
#add_executable( wml_schema_test_objects
#	filesystem.cpp
#	string_utils.cpp
#	wml_node.cpp
#	wml_parser.cpp
#	wml_utils.cpp )
#