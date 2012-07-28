# vi: syntax=python:et:ts=4

import os.path

# Warn user of current set of build options.
if os.path.exists('.scons-option-cache'):
    optfile = file('.scons-option-cache')
    print "Saved options:", optfile.read().replace("\n", ", ")[:-2]
    optfile.close()

opts = Variables('.scons-option-cache')

opts.AddVariables(
    BoolVariable('ccache', "Use ccache", False),
    ('cxxtool', 'Set c++ compiler command if not using standard compiler.'),
    ('jobs', 'Set the number of parallel compilations', "1", lambda key, value, env: int(value), int),
    )

env = Environment(options = opts)

env.ParseConfig("sdl-config --libs --cflags")
env.Append(LIBS = ["GL", "GLU", "GLEW", "SDL_mixer", "SDL_image", "SDL_ttf", "boost_regex", "boost_system", "boost_iostreams", "png"])
env.Append(CXXFLAGS= ["-pthread", "-DIMPLEMENT_SAVE_PNG"], LINKFLAGS = ["-pthread"])

opts.Save('.scons-option-cache', env)

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
env.SConscript(dirs=["src"], variant_dir = "build", duplicate = False)
