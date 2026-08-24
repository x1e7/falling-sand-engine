#define SDL_MAIN_HANDLED

#include "Game/GameLoop.h"

int main(int argc, char* argv[]) {
    GameLoop game;
    game.run();
    return 0;
}
