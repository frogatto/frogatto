from os.path import splitext, exists

SConscript("src/SConscript", variant_dir = "build", duplicate = False)

# Warn user of current set of build options.
if exists('.scons-option-cache'):
    optfile = file('.scons-option-cache')
    print "Saved options:", optfile.read().replace("\n", ", ")[:-2]
    optfile.close()

opts = Variables('.scons-option-cache')

opts.AddVariables(
    ('jobs', 'Set the number of parallel compilations', "1", lambda key, value, env: int(value), int),
    )

env = Environment(options = opts)
env["MSGFMT"] = env.WhereIs("msgfmt")
if env["MSGFMT"]:
    for po_file in Glob("po/*.po"):
        lingua, ext = splitext(po_file.name)
	env.Command(Dir("locale").Dir(lingua).Dir("LC_MESSAGES").File("frogatto.mo"), po_file, "$MSGFMT --statistics -o $TARGET $SOURCE")

if env['jobs'] > 1:
    SetOption("num_jobs", env['jobs'])

opts.Save('.scons-option-cache', env)
