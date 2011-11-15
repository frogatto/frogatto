#! /bin/sh
cat <<EOF
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
msgid ""
msgstr ""
"Project-Id-Version: Frogatto v1.0.3\n"
"Report-Msgid-Bugs-To: http://code.google.com/p/frogatto/issues/list\n"
EOF
echo '"POT-Creation-Date: '$(date +"%Y-%m-%d %H:%M%z")"\\\\n\""
cat <<EOF
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n > 1);\n"

EOF
(
# find metadata files
python utils/parse-metadata.py
# find preload messages
grep -Hn 'message="[^"][^"]*"' data/preload.cfg | \
	sed -ne 's/^\(.*:[0-9]*\):.*message="/#: \1\n"/;s/"\([^"]*\)".*/msgid "\1"\nmsgstr ""\n/gp'
# find level titles
grep -Hn '^title="[^"][^"]*"' data/level/*.cfg | \
	sed -ne 's/^\(.*:[0-9]*\):title="/#: \1\n"/;s/"\([^"]*\)".*/msgid "\1"\nmsgstr ""\n/gp'
# find achievements
grep -Hn '\(name\|description\)="[^"][^"]*"' data/achievements.cfg | \
	sed -ne 's/^\(.*:[0-9]*\):.*\(name\|description\)="/#: \1\n"/;s/"\([^"]*\)".*/msgid "\1"\nmsgstr ""\n/gp'
# find marked strings ~...~ in level files
grep -Hn --exclude-dir=".svn" "~[^~][^~]*~" data/level/*.cfg | \
	sed -ne ":A;s/\([a-z0-9/\._-]*:[0-9]*\):[^~]*~\([^~][^~]*\)~/\n#: \1\nmsgid \"\2\"\nmsgstr \"\"\n\1:/;tA;s/\n\([a-z0-9/\._-]*:[0-9]*\):[^\n]*//;p"
# find marked strings ~...~ in data files; files in data/*/experimental are ignored
grep -Hnr --exclude-dir=".svn" --exclude-dir="experimental" "~[^~][^~]*~" data/objects/ data/object_prototypes/ | \
	sed -ne ":A;s/\([a-z0-9/\._-]*:[0-9]*\):[^~]*~\([^~][^~]*\)~/\n#: \1\nmsgid \"\2\"\nmsgstr \"\"\n\1:/;tA;s/\n\([a-z0-9/\._-]*:[0-9]*\):[^\n]*//;p"
# find marked strings _("...") in source files
grep -Hn '[^a-z]_("[^"]*")' src/*.cpp | \
	sed -ne 's/^\(.*:[0-9][0-9]*\):.*_("/#: \1\n_("/;s/_("\([^"]*\)").*\(_("\|\)/msgid "\1"\nmsgstr ""\n\2/gp'
) | cat
