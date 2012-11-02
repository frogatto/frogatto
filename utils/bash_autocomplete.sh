#Note: Do not invoke this script directly, rather, use ". bash_autocomplete_setup.sh".
_frogatto() 
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="--config-path= --fullscreen --height --host --no-joystick --joystick --level --no-music --music --native --relay --no-resizable --resizable --scale --no-send-stats --send-stats --server= --user= --pass= --no-sound --sound --widescreen --width --windowed --wvga --no-debug --debug --fps --no-fps --set-fps= --potonly --textures16 --textures32 --benchmarks --benchmarks= --no-compiled --compiled --edit --show-hitboxes --show-controls --simipad --simiphone --no-autopause --tests --no-tests --textures16"
    utils="codeedit compile_levels compile_objects correct_solidity document_ffl_functions generate_scaling_code hole_punch_test install_module list_modules module_server object_definition publish_module publish_module_stats query render_level sign_game_data stats_server tbs_server test_all_objects textedit"
    cd modules/
    module_names=$(ls -d -x --color=never -- */)
    cd ../
    
    module_paths="data/level/"
    for i in $module_names
    do
    	module_paths="${module_paths} modules/${i}data/level/"
    done
    
    level_names="" #Note: Also contains directories. Should fix.
    current_directory="${PWD}"
    for i in $module_paths
    do
    	if [ -d "${i}" ]; then
    		opts="${opts} --level-path=${i}"
    		cd "${i}"
    		levels=$(ls -x --color=never)
    		level_names="${level_names} ${levels}"
    		cd "${current_directory}"
    	fi
    done
    
    for i in $utils
    do
    	opts="${opts} --utility=${i}"
    done
    
    #echo "${COMP_WORDBREAKS}" | sed s/[\=]//
   	
    case "${prev}" in
	"--level")
		COMPREPLY=( $(compgen -W "${level_names}" -- ${cur}) )
        return 0
        ;;
     *)
		COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
        ;;
    esac
}
launches="game frogatto ct rpg"
for i in $launches
do
	complete -F _frogatto ${i}
done