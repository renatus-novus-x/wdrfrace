#include "gmode.h"

GameMode::~GameMode() {}

int GameMode::initialize() { return 1; }

int GameMode::initialize_step() { return 1; }

GameModeId GameMode::update() { return GAME_MODE_EXIT; }

int GameMode::consume_select_sound() { return 0; }

void GameMode::render(int) {}

void GameMode::finalize() {}
