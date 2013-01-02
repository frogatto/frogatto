#ifndef PAUSE_GAME_DIALOG_INCLUDED
#define PAUSE_GAME_DIALOG_INCLUDED

enum PAUSE_GAME_RESULT { PAUSE_GAME_CONTINUE, PAUSE_GAME_CONTROLS, PAUSE_GAME_QUIT, PAUSE_GAME_GO_TO_TITLESCREEN };

PAUSE_GAME_RESULT show_pause_game_dialog();

struct interrupt_game_exception {
	PAUSE_GAME_RESULT result;
	interrupt_game_exception(PAUSE_GAME_RESULT res=PAUSE_GAME_QUIT) : result(res)
	{}
};

#endif
