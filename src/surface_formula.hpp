#ifndef SURFACE_FORMULA_HPP_INCLUDED
#define SURFACE_FORMULA_HPP_INCLUDED

#include <string>
#include <vector>

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA)
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

#include "surface.hpp"

graphics::surface get_surface_formula(graphics::surface input, const std::string& algo);

GLuint get_gl_shader(const std::vector<std::string>& vertex_shader,
                     const std::vector<std::string>& fragment_shader);

#endif
