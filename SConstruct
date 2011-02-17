SConscript("src/SConscript", variant_dir = "build", duplicate = False)

from os.path import splitext
env = Environment()
env["MSGFMT"] = env.WhereIs("msgfmt")
if env["MSGFMT"]:
    for po_file in Glob("po/*.po"):
        lingua, ext = splitext(po_file.name)
	env.Command(Dir("locale").Dir(lingua).Dir("LC_MESSAGES").File("frogatto.mo"), po_file, "$MSGFMT --statistics -o $TARGET $SOURCE")
