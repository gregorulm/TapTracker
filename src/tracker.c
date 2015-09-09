#include "tracker.h"

#include "game.h"
#include "font.h"
#include "draw.h"

#include "joystick.h"
#include "colortheme.h"

#include "layout.h"

#include <sys/wait.h>

#include <stdio.h>

#include <GLFW/glfw3.h>

bool runTracker(int* dataPtr)
{
    if (!glfwInit())
    {
        perror("Could not initialize GLFW");
        return false;
    }

    GLFWwindow* window;

    // Hard coded window sizes to fit tracker window + 640x480 MAME window in
    // qHD. UI is not dynamic, so neither is this, for now.
    float ratio = 540 / 480.0;
    const unsigned int width = 960 - 640 * ratio;
    const unsigned int height = 540;

    {
        glfwWindowHint(GLFW_RESIZABLE, false);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

        window = glfwCreateWindow(width, height,
                                  "TapTracker",
                                  NULL,
                                  NULL);
        if (window == NULL)
        {
            perror("Could not create GLFW window");
            return false;
        }

        glfwMakeContextCurrent(window);

        setupOpenGL(width, height);
    }

    struct font_t font;
    loadFont(&font, "/usr/share/fonts/TTF/DroidSansFallback.ttf", 18.0f);

    struct joystick_t joystick;
    createJoystick(&joystick, GLFW_JOYSTICK_1);

    struct game_t game;
    createNewGame(&game);

    /* const int SCALE_COUNT = 2; */
    /* float scales[] = { 45.0f, 60.0f }; */
    /* int scaleIndex = 0; */

    struct layout_container_t layout;
    createLayoutContainer(&layout, width, height, 8.0f, 2.0f);

    addToContainerRatio(&layout, &drawSectionGraph, 0.50f);
    addToContainerFixed(&layout, &drawSectionLineCount, 26.0f);
    addToContainerRatio(&layout, &drawHistory, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        // Read input from child process and add it to the graph
        {
            game.prevState = game.state;
            game.prevLevel = game.level;
            game.prevTime  = game.time;

            game.state = dataPtr[0];
            game.level = dataPtr[1];
            game.time  = dataPtr[2];

            if (isInPlayingState(game.state) &&
                (game.time < game.prevTime || game.level < game.prevLevel))
            {
                perror("Internal State Error");
                printGameState(&game);
            }

            if (isInPlayingState(game.state) && game.level - game.prevLevel > 0)
            {
                // Push a data point based on the newly acquired game state.
                pushCurrentState(&game);
            }

            if (game.prevState != ACTIVE && game.state == ACTIVE)
            {
                pushHistoryElement(&game.inputHistory, game.level);
            }

            // Reset if we were looking at the game over screen and just
            // moved to an idle state.
            if (isInPlayingState(game.prevState) && !isInPlayingState(game.state))
            {
                resetGame(&game);
            }
        }

        glfwPollEvents();

        updateButtons(&joystick);
        /* if (buttonChangedToState(&joystick, BUTTON_D, GLFW_PRESS)) */
        /* { */
        /*     scaleIndex++; */
        /* } */
        pushCharFromJoystick(&game.inputHistory, &joystick);

        /* setColorTheme(&LIGHT_THEME); */
        setGLClearColor();
        glClear(GL_COLOR_BUFFER_BIT);

        drawLayout(&layout, &game, &font);

        glfwSwapBuffers(window);
    }

    destroyGame(&game, false);
    destroyJoystick(&joystick, false);
    destroyFont(&font, false);

    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
}
