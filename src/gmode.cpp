#include "gmode.h"

GameMode::~GameMode() {}

int GameMode::initialize() { return 1; }

GameModeId GameMode::update() { return GAME_MODE_EXIT; }

void GameMode::render() {}

void GameMode::finalize() {}
