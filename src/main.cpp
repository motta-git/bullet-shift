#include "Game.h"

int main() {
    Game game;
    
    if (!game.initialize()) {
        return -1;
    }

    game.run();
    return 0;
}
