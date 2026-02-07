# Bullet Shift

Welcome to my game, Bullet Shift.

This a game i made to showcase my opengl and c++ skills.

It is a first person shooter where you get bullet time with each enemy elimination.

It leverages **ImGui** for the user interface, **Assimp** for model loading, **GLAD** for OpenGL function loading, **GLFW** for window management, **GLM** for mathematics, **stb_image** for texture handling, and **miniaudio** for the audio.

I am releasing the source code as of 2026/02/07 to help other developers learn opengl or use this code for their own projects under the 'GNU GENERAL PUBLIC LICENSE Version 3' as specified in the "LICENSE" file.

There is a TODO.md file with a list of features i would like to continue working on, feel free to colaborate on those.

Enjoy!

## Dependencies

This project is configured to build using Docker and Docker Compose to ensure a consistent environment across different operating systems.

For further information about how to build this project check the commands/README.md file.

- **CMake 3.14+** (Required for dependency fetching)
- **C++17 compatible compiler** (GCC 9+, Clang 10+, or MSVC 2019+)
- **OpenGL 4.6+** compatible drivers and hardware

The following libraries are used (automatically managed):
- **GLFW 3.3+**: Window and input management.
- **GLAD**: OpenGL function loader.
- **GLM**: OpenGL Mathematics.
- **Assimp**: Open Asset Import Library for 3D models.
- **Dear ImGui**: User interface.
- **miniaudio**: Audio engine.
- **stb_image**: Image loading.

### Game Controls
- **Mouse**: Look around
- **Left Click**: Shoot
- **W/A/S/D**: Move forward/left/backward/right
- **Space**: Jump
- **ESC**: Open menu
- **SHIFT**: Dash

## Credits

Agustín Fabio Motta, Programming, Music and almost everything.

Low poly pistol made by TastyTony 
https://sketchfab.com/3d-models/low-poly-g17-60efc54ac59e47d18bdff94b2f5144ff

I used it Under Attribution 4.0 International CC BY 4.0 Deed
https://creativecommons.org/licenses/by/4.0/

https://pixabay.com/es/sound-effects/single-gunshot-62-hp-37188/

Universal UI/Menu Soundpack by Nathan Gibson
https://cyrex-studios.itch.io/universal-ui-soundpack