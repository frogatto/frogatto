local _ = MojoSetup.translate
local TOTAL_INSTALL_SIZE = 267949154;

Setup.Package
{
    id = "frogatto",
    description = "Frogatto & Friends",
    vendor = "frogatto.com",
    version = "1.3.2",
    splash = "logo.png",
    superuser = false,
    recommended_destinations =
    {
        MojoSetup.info.homedir,
        "/opt/games",
        "/usr/local/games"
    },

    Setup.Eula
    {
        description = _("Frogatto License"),
        source = "LICENSE"
    },

    Setup.Readme
    {
        description = _("Frogatto Readme"),
        source = "README"
    },

    Setup.Option
    {
        value = true,
        required = false,
        disabled = false,
        bytes = TOTAL_INSTALL_SIZE,
        description = "Frogatto 1.3.2",

        -- Linux-specific files, executables, etc.
        Setup.File
        {
            wildcards = "*";
            filter = function(dest)
                if dest == "game" then
                    return dest, "0755"   -- make sure it's executable.
                end
		if string.match(dest, ".so.") then
                    return dest, "0755"   -- make sure it's executable.
                end
                return dest   -- everything else just goes through as-is.
            end

        },
	
        Setup.DesktopMenuItem
        {
            disabled = false,
            name = "Frogatto & Friends",
            genericname = "frogatto",
            tooltip = _("Frogatto & Friends"),
            builtin_icon = false,
            icon = "icon.png",
            commandline = "%0/game",
            workingdir = "%0",
            category = "Game"
        }
    },

}

-- end of config.lua ...
