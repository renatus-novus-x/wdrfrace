#include "gmode.h"

GameMode::~GameMode() {}

int GameMode::initialize() { return 1; }

int GameMode::initialize_step() { return 1; }

void GameMode::advance_time(int) {}

GameModeId GameMode::update() { return GAME_MODE_EXIT; }

int GameMode::consume_select_sound() { return 0; }

GameSoundId GameMode::consume_game_sound() { return GAME_SOUND_NONE; }

void GameMode::render(int) {}

void GameMode::finalize() {}
