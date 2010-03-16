objects = IMG_savepng.o background.o blur.o border_widget.o button.o character_editor_dialog.o current_generator.o editor_dialogs.o editor_formula_functions.o editor_layers_dialog.o editor_stats_dialog.o editor_variable_info.o collision_utils.o color_utils.o controls.o custom_object.o custom_object_callable.o custom_object_functions.o custom_object_type.o debug_console.o dialog.o draw_number.o draw_scene.o draw_tile.o editor.o editor_level_properties_dialog.o entity.o filesystem.o font.o formula.o formula_callable_definition.o formula_constants.o formula_function.o formula_tokenizer.o formula_variable_storage.o frame.o framed_gui_element.o geometry.o graphical_font.o grid_widget.o group_property_editor_dialog.o gui_formula_functions.o gui_section.o image_widget.o input.o inventory.o iphone_controls.o joystick.o key.o label.o level.o level_logic.o level_object.o level_runner.o level_solid_map.o load_level.o main.o message_dialog.o movement_script.o multi_tile_pattern.o multiplayer.o object_events.o options_dialog.o particle_system.o playable_custom_object.o player_info.o powerup.o preferences.o preprocessor.o preview_tileset_widget.o property_editor_dialog.o random.o raster.o raster_distortion.o rectangle_rotator.o solid_map.o sound.o speech_dialog.o stats.o string_utils.o surface_cache.o surface_formula.o surface_scaling.o surface.o texture.o text_entry_widget.o thread.o tile_map.o tileset_editor_dialog.o tooltip.o translate.o utils.o variant.o water.o water_particle_system.o weather_particle_system.o widget.o wml_formula_adapter.o wml_formula_callable.o wml_modify.o wml_node.o wml_parser.o wml_schema.o wml_utils.o wml_writer.o unit_test.o formula_test.o wml_parser_test.o loading_screen.o

server_objects = server.o simple_wml.o

formula_test_objects = filesystem.o formula_function.o formula_tokenizer.o string_utils.o variant.o wml_node.o wml_parser.o wml_utils.o wml_writer.o

wml_modify_test_objects = filesystem.o string_utils.o wml_node.o wml_parser.o wml_utils.o
wml_schema_test_objects = filesystem.o string_utils.o wml_node.o wml_parser.o wml_utils.o

OPT=-O2 -fno-inline-functions

%.o : src/%.cpp
	g++ -DIMPLEMENT_SAVE_PNG -fno-inline-functions -g $(OPT) `sdl-config --cflags` -I/usr/X11R6/include -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -fthreadsafe-statics -c $<

game: $(objects)
	g++ -g $(OPT) -L. -L/sw/lib -L/usr/X11R6/lib -L. -lprofiler -lX11 -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lGLEW -lSDL_image -lSDL_ttf -lSDL_mixer -lpng -lboost_regex-mt -lboost_system-mt -fthreadsafe-statics $(objects) -o game

server: $(server_objects)
	g++ -fno-inline-functions -g $(OPT) -L/sw/lib -L/usr/X11R6/lib -lX11 -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt -fthreadsafe-statics $(server_objects) -o server

poolalloc.o: src/poolalloc.c
	gcc -fno-inline-functions -g $(OPT) `sdl-config --cflags` -I/usr/X11R6/include -D_GNU_SOURCE=1 -D_REENTRANT -Wreturn-type -c src/poolalloc.c

malloc.o: src/malloc.c
	gcc -fno-inline-functions -g $(OPT) `sdl-config --cflags` -I/usr/X11R6/include -D_GNU_SOURCE=1 -D_REENTRANT -Wreturn-type -c src/malloc.c

formula_test: $(formula_test_objects)
	g++ -O2 -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_FORMULA -Wnon-virtual-dtor -Wreturn-type -L/usr/lib -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex src/formula.cpp $(formula_test_objects) -o test

wml_modify_test: $(wml_modify_test_objects)
	g++ -O2 -g -framework Cocoa -I/usr/local/include/boost-1_34 -I/sw/include/SDL -I/usr/X11R6/include -Isrc/ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_WML_MODIFY -Wnon-virtual-dtor -Wreturn-type -L/usr/lib -lboost_regex src/wml_modify.cpp $(wml_modify_test_objects) -o test

wml_schema_test: $(wml_schema_test_objects)
	g++ -O2 -g -framework Cocoa -I/usr/local/include/boost-1_34 -I/sw/include/SDL -I/usr/X11R6/include -Isrc/ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_WML_SCHEMA -Wnon-virtual-dtor -Wreturn-type -L/usr/lib -lboost_regex src/wml_schema.cpp $(wml_schema_test_objects) -o test
	

clean:
	rm -f *.o game
