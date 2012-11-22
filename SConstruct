# vi: syntax=python:et:ts=4

import os.path

# Warn user of current set of build options.
if os.path.exists('.scons-option-cache'):
    optfile = file('.scons-option-cache')
    print "Saved options:", optfile.read().replace("\n", ", ")[:-2]
    optfile.close()

opts = Variables('.scons-option-cache')

opts.AddVariables(
    EnumVariable('build', 'Build variant: debug, release profile', "release", ["release", "debug", "profile"]),
    PathVariable('build_dir', 'Build all intermediate files(objects, test programs, etc) under this dir', "build", PathVariable.PathAccept),
    BoolVariable('ccache', "Use ccache", False),
    BoolVariable('cxx0x', 'Use C++0x features.', False),
    BoolVariable('extrawarn', "Use wesnoth-level warnings", False),
    BoolVariable('gles2', "Use GLES2", False),
    BoolVariable('strict', 'Set to strict compilation', False),
    ('cxxtool', 'Set c++ compiler command if not using standard compiler.'),
    ('jobs', 'Set the number of parallel compilations', "1", lambda key, value, env: int(value), int),
    )

env = Environment(options = opts)

env.ParseConfig("sdl-config --libs --cflags")
env.Append(LIBS = ["GL", "GLU", "GLEW", "SDL_mixer", "SDL_image", "SDL_ttf", "boost_regex", "boost_system", "boost_iostreams", "png"])
env.Append(CXXFLAGS= ["-pthread", "-DIMPLEMENT_SAVE_PNG"], LINKFLAGS = ["-pthread"])

opts.Save('.scons-option-cache', env)

builds = {
    "debug"         : dict(CCFLAGS   = Split("$DEBUG_FLAGS")),
    "release"       : dict(CCFLAGS   = "$OPT_FLAGS"),
    "profile"       : dict(CCFLAGS   = "-pg", LINKFLAGS = "-pg")
    }
build = env["build"]

env.AppendUnique(**builds[build])

build_dir = os.path.join("$build_dir", build)

if build == "release" : build_suffix = ""
else                  : build_suffix = "-" + build
Export("build_suffix")

if "gcc" in env["TOOLS"]:
    env.AppendUnique(CCFLAGS = Split("-Wignored-qualifiers -Wformat -Wswitch"))
    if env['extrawarn']:
        env.AppendUnique(CCFLAGS = Split("-W -Wall -Wno-sign-compare -Wno-parentheses"))

    if env['cxx0x']:
        env.AppendUnique(CXXFLAGS = "-std=c++0x")
        env.Append(CPPDEFINES = "HAVE_CXX0X")
    else:
        env.AppendUnique(CXXFLAGS = "-std=c++98")

    if env['strict']:
        env.AppendUnique(CCFLAGS = Split("-Werror"))

    env["OPT_FLAGS"] = "-O2"
    env["DEBUG_FLAGS"] = Split("-O0 -DDEBUG -ggdb3")

env['exclude'] = []

if env['gles2']:
    env.Append(CXXFLAGS= ["-DUSE_GLES2"])
else:
    pass
    #env['exclude'] += [ "gles2.cpp", "shaders.cpp" ]
if env.get('cxxtool',""):
    env['CXX'] = env['cxxtool']
    if 'HOME' in os.environ:
        env['ENV']['HOME'] = os.environ['HOME']
if env['ccache']:
    env['CCACHE'] = env.WhereIs("ccache")
    env['CC'] = '%s %s' % (env['CCACHE'], env['CC'])
    env['CXX'] = '%s %s' % (env['CCACHE'], env['CXX'])
    for i in ['HOME',
          'CCACHE_DIR',
          'CCACHE_TEMPDIR',
          'CCACHE_LOGFILE',
          'CCACHE_PATH',
          'CCACHE_CC',
          'CCACHE_PREFIX',
          'CCACHE_DISABLE',
          'CCACHE_READONLY',
          'CCACHE_CPP2',
          'CCACHE_NOSTATS',
          'CCACHE_NLEVELS',
          'CCACHE_HARDLINK',
          'CCACHE_RECACHE',
          'CCACHE_UMASK',
          'CCACHE_HASHDIR',
          'CCACHE_UNIFY',
          'CCACHE_EXTENSION']:
        if os.environ.has_key(i) and not env.has_key(i):
            env['ENV'][i] = os.environ[i]
if env['jobs'] > 1:
    SetOption("num_jobs", env['jobs'])

Export(["env"])
env.SConscript(dirs=["src"], variant_dir = build_dir, duplicate = False)
