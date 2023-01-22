#include <QuickGame.h>
#include <gu2gl.h>
#include <pspctrl.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
POST TODOS
3. Mission Interstitals
4. Add checks to move obstalce to ensure they don't overlap node and eachother
6. Animate paddle up and down
12. Add powerups (speed boost, paddle embiggener, multiball)
13. Add pause quit instruction screen
15. Add scroll speed
15a. Synch music with scroll speed
17. Clean Up
17a. Optimize redundant code (checking flip every update)
21. Add Settings option to pause state
*/

QGSprite_t bg, bgFore, attempts, pinkPaddle, bluePaddle, endBall, wall, flipPad, titlePink;
QGSprite_t startScreen, startWhiteScreen, settingsScreen
, packetlost, gameover, runCompleteScreen, missionCompleteScreen, gameCompleteScreen, finalScreen;
QGSprite_t blueSplashTop[6], blueSplashBottom[6];
QGSprite_t pinkSplashTop[6], pinkSplashBottom[6];
QGSprite_t credits[5];
QGSprite_t score[10];
QGSprite_t animBall[6][7];
QGSprite_t run[5];
QGSprite_t mission[3];
QGSprite_t nums[10];
QGSprite_t endNode[29];
QGSprite_t scrolls[6];

enum QGDirection direction;

QGAudioClip_t ping, pong, fail, clear, runMusic1, runMusic2;

QGTimer timer;

typedef enum {
    PAUSED,
    ENDLESS_PAUSED,
    LOADED_NOT_STARTED,
    ENDLESS_LOADED_NOT_STARTED,
    STARTED,
    ENDLESS_STARTED,
    VIEWING_CREDITS,
    VIEWING_SETTINGS,
    VIEWING_START,
    RUN_COMPLETE,
    MISSION_COMPLETE,
    GAME_COMPLETE,
    ENDLESS_COMPLETE,
    VIEWING_PRE_SCROLL,
    VIEWING_POST_SCROLL,
    MOSTLY_DEAD,
    ALL_DEAD,
} GAMESTATE;

GAMESTATE currentState;

const float PADDLE_HEIGHT = 50.0f;
const float SCREEN_HEIGHT = 272.0f;
const float SCREEN_WIDTH = 480.0f;
const float VEL_PADDLE = 5.0f;
const float LEFT_PADDLE_LANE_X = 13.0f;
const float RIGHT_PADDLE_LANE_X = 470.0f;
const float BALL_RADIUS = 7.0f;

float ballYX[6][2];
float wallYX[1][2];
float pinkPaddleY, bluePaddleY;
float flipPadX, flipPadY;
float endNodeY, endNodeX;
float ballVel;
float ballVelX, ballVelY;
float ballPaddleCollisionY;
float textScrollY;
float titlePinkPaddleY;
float bgScrollOffset;
float bgBackScrollOffset;
float runLength;
float flipPadTimer;
float wallTimer;
bool isControlFlipped;
bool isBallMovingUpwards;
bool isBallMovingRight;
bool isBackgroundScrolling;
bool isGameComplete;
bool isFinalScreenShowing;
int currentRun;
int currentMission;
int currentScore;
int currentScroll;
int remainingAttempts;
int selectedStartOption;
int selectedSettingsOption;

int startMenuOptionCoords[3][2] = {
    {172, 130},
    {146, 115},
    {158, 100}
};

int settingsMenuOptionCoords[5][2] = {
    {14, 174},
    {14, 159},
    {14, 144},
    {14, 129},
    {14, 114}
};

// ~35s for max starting at 100.0f and +0.1f
//  ~11s for max starting at 100.0f and +0.3f
float velMax = 300.0f;
int collisionDelay = 0;
int currentCredit = 0;
int difficultyLevel = 1;

bool faceControls = false;
bool allowGlitch = false;
bool complexPhysics = false;
bool endlessMode = false;

enum QGFlip splashFlipsBpTb[4];
bool showBlueSplash = false;
bool showPinkSplash = false;

bool startAnimShown = false;

int currBallAnim = 0;
float animTime = 0.0f;
int currSplashAnim = 0;
float splashAnimTime = 0.0f;
int currEndNodeAnim = 0;
float endNodeAnimTime = 0.0f;

void animateBall(double dt) {
    animTime += dt;

    if(animTime > 0.15f) {
        currBallAnim++;
        animTime = 0.0f;

        if(currBallAnim == 7)
            currBallAnim = 0;
    }
}

void animateSplash(double dt) {
    splashAnimTime += dt;

    if(splashAnimTime > 0.15f) {
        currSplashAnim++;
        splashAnimTime = 0.0f;

        if(currSplashAnim == 5) {
            currSplashAnim = 0;
            showBlueSplash = false;
            showPinkSplash = false;
        }
    }
}

void animateEndNode(double dt) {
    if(endNodeY < SCREEN_HEIGHT + 32.0f) {
        endNodeAnimTime += dt;

        if(endNodeAnimTime > 0.12f) {
            currEndNodeAnim++;
            endNodeAnimTime = 0.0f;

            if(currEndNodeAnim == 29)
                currEndNodeAnim = 0;
        }
    }
    if(endNodeY > 120.0f) {
        endNodeY -= 1.0f;
    } else {
        isBackgroundScrolling = false;
    }
}

void animateStart() {
    if(titlePinkPaddleY > 167.0f) {
        titlePinkPaddleY -= 1.0f;
    } else {
        startAnimShown = true;
    }
}

void handleIsControlFlipped() {
    if(isControlFlipped == true) {

    memcpy(splashFlipsBpTb, (enum QGFlip[]) {
            QG_FLIP_BOTH, QG_FLIP_HORIZONTAL,
            QG_FLIP_VERTICAL, QG_FLIP_NONE
            }, sizeof splashFlipsBpTb);

        bluePaddle->transform.position.x = LEFT_PADDLE_LANE_X;
        pinkPaddle->transform.position.x = RIGHT_PADDLE_LANE_X;

        for(int i = 0; i < 5; i++) {
            pinkSplashBottom[i]->transform.position.x = RIGHT_PADDLE_LANE_X - 12.0f;
            pinkSplashTop[i]->transform.position.x = RIGHT_PADDLE_LANE_X - 12.0f;
            blueSplashBottom[i]->transform.position.x = LEFT_PADDLE_LANE_X + 12.0f;
            blueSplashTop[i]->transform.position.x = LEFT_PADDLE_LANE_X + 12.0f;
        }
    } else {

        memcpy(splashFlipsBpTb, (enum QGFlip[]) {
        QG_FLIP_VERTICAL, QG_FLIP_NONE,
        QG_FLIP_BOTH, QG_FLIP_HORIZONTAL
            }, sizeof splashFlipsBpTb);

        bluePaddle->transform.position.x = RIGHT_PADDLE_LANE_X;
        pinkPaddle->transform.position.x = LEFT_PADDLE_LANE_X;

        for(int i = 0; i < 5; i++) {
            blueSplashBottom[i]->transform.position.x = RIGHT_PADDLE_LANE_X - 12.0f;
            blueSplashTop[i]->transform.position.x = RIGHT_PADDLE_LANE_X - 12.0f;
            pinkSplashBottom[i]->transform.position.x = LEFT_PADDLE_LANE_X + 12.0f;
            pinkSplashTop[i]->transform.position.x = LEFT_PADDLE_LANE_X + 12.0f;
        }
    }
}

void drawBgScroll() {
    if(isBackgroundScrolling) {
        bgBackScrollOffset = timer.total * 0.15f;
        bgScrollOffset = timer.total * 0.08f;
    }
    glTexOffset(0.0f, bgBackScrollOffset);
    QuickGame_Sprite_Draw(bg);
    glTexOffset(0.0f, bgScrollOffset);
    QuickGame_Sprite_Draw(bgFore);
    glTexOffset(0.0f, 0.0f);
}

void drawRemainingAttempts() {
    for(int i = 0; i < remainingAttempts; i++) {
        QuickGame_Sprite_Draw(animBall[i+1][currBallAnim]);
    }
    QuickGame_Sprite_Draw(attempts);
}

void drawScore() {
    int digits = snprintf( NULL, 0, "%d", currentScore);

    float xoff = -((float)digits-1) / 2.0f;
    xoff *= 32.0f;

    float xn = 0.0f;

    int s = currentScore;
    for(int i = 0; i < digits; i++) {
        int c = s % 10;
        s /= 10;

        nums[c]->transform.position.x = -xoff + 240 - xn;
        nums[c]->transform.position.y = 192;

        xn += 32.0f;

        QuickGame_Sprite_Draw(nums[c]);
    }
}

void randomizeFlip() {
    flipPadTimer = fmod(rand(), runLength);
    flipPadY = 300.0f + fmod(rand(),100.0f);
    flipPadX = 180 + rand() % 120;
}

void randomizeWall() {
    wallTimer = fmod(rand(), runLength);
    wallYX[0][0] = 300.0f + fmod(rand(),100.0f);
    wallYX[0][1] = 120 + rand() % 240;
}

void randomizeObstacles() {
    randomizeFlip();
    randomizeWall();
}

void randomizeRunVariables() {
    int startSeed = rand() % 4;

    switch(startSeed) {
        case 0 :
            isBallMovingUpwards = false; 
            isBallMovingRight = false;
            ballVelX = ballVel;
            ballVelY = -ballVel;
            break;
        case 1 :
            isBallMovingUpwards = true; 
            isBallMovingRight = true;
            ballVelX = ballVel;
            ballVelY = ballVel;
            break;
        case 2 :
            isBallMovingUpwards = true; 
            isBallMovingRight = false;
            ballVelX = -ballVel;
            ballVelY = ballVel;
            break;
        case 3 :
            isBallMovingUpwards = false; 
            isBallMovingRight = true;
            ballVelX = -ballVel;
            ballVelY = -ballVel;
            break;
        default :
            isBallMovingUpwards = false; 
            isBallMovingRight = false;
            ballVelX = ballVel;
            ballVelY = -ballVel;
    }

    endNodeX = 100 + rand() % 280;
}

void resetRun() {
    ballYX[0][0] = 136.0f;
    ballYX[0][1] = 240.0f;
    for(int i=1; i<6; i++)
        ballYX[i][0] = -30.0f;
    pinkPaddleY = 136.0f;
    bluePaddleY = 136.0f;
    wallYX[0][0] = 300.0f;
    flipPadY = 300.0f;
    isControlFlipped = false;
    handleIsControlFlipped();
    ballVel = 0.0f;
    ballVelX = 0.0f;
    ballVelY = 0.0f;
    endNodeY = 400.0f;

    isBackgroundScrolling = false;
    currentCredit = 0;
    if(endlessMode) {
        currentState = ENDLESS_LOADED_NOT_STARTED;
    } else {
        currentState = LOADED_NOT_STARTED;
    }

    QuickGame_Timer_Reset(&timer);
    QuickGame_Audio_Play(runMusic1, 1);
}

void resetGame() {
    resetRun();
    currentMission = 1;
    remainingAttempts = 3;
    currentRun = 1;
    currentScore = 0;
    currentScroll = 0;
    selectedStartOption = 1;
    selectedSettingsOption = 1;
    currentState = VIEWING_START;
    isGameComplete = false;
    isFinalScreenShowing = false;
    textScrollY = -248.0f;
    startAnimShown = false;
    titlePinkPaddleY = 300.0f;

    QuickGame_Audio_Play(runMusic2, 1);
}


void moveToNextRun() {
    ballYX[0][0] = 136.0f;
    ballYX[0][1] = 240.0f;
    pinkPaddleY = 136.0f;
    bluePaddleY = 136.0f;
    currentState = LOADED_NOT_STARTED;
    isControlFlipped = false;
    handleIsControlFlipped();
    ballVel = 0.0f;
    ballVelX = 0.0f;
    ballVelY = 0.0f;
    wallYX[0][0] = 320.0f;
    flipPadY = 320.0f;
    endNodeY = 400.0f;
    // First run - ~3s, Second run - ~ 7s?
    runLength = 5.0f + (difficultyLevel * currentRun * 1.0f);
    velMax = 300.0f + ((difficultyLevel - 1) * 25.0f);
}

void moveToNextMission() {
    moveToNextRun();
    textScrollY = -248.0f;
    currentScroll++;
    remainingAttempts = 3;
}

void updateScroll() {
    if(QuickGame_Button_Held(PSP_CTRL_DOWN)) {
        textScrollY += 0.8f;
    } else {
        textScrollY += 0.2f;
    }
    if(textScrollY > 600.0f) {
        if(currentState == VIEWING_PRE_SCROLL) {
            moveToNextMission();
        } else if (currentState == VIEWING_POST_SCROLL) {
            currentScroll++;
            currentState = VIEWING_PRE_SCROLL;
            textScrollY = -248.0f;
        } else {
            isFinalScreenShowing = true;
        }
    } else {
        scrolls[currentScroll]->transform.position.y = textScrollY;   
    }
}

void checkDeath() {
    QuickGame_Audio_Play(fail, 0);
    // isBallMovingRight=!isBallMovingRight;
    if(currentState == ENDLESS_STARTED) {
        currentState = ENDLESS_COMPLETE;
    } else {
        currentState = MOSTLY_DEAD;
        remainingAttempts--;
        if(remainingAttempts < 0) {
            currentState = ALL_DEAD;
        }
    }
}

void checkVictory() {
    currentState = RUN_COMPLETE;
    currentRun++;
    if(currentRun > 5) {
        currentMission++;
        currentRun = 1;
        currentState = MISSION_COMPLETE;
    }
    if(currentMission > 3) {
        currentState = isGameComplete;
    }
}

void checkBallCollision(double dt) {
    if(showPinkSplash || showBlueSplash) {
        animateSplash(dt);
    }

    if(collisionDelay >= 0) {
        collisionDelay--;
    } else {
        if(QuickGame_Sprite_Intersects(animBall[0][currBallAnim], pinkPaddle)) {
            showPinkSplash = true;
            currentScore++;
            ballPaddleCollisionY = pinkPaddleY - ballYX[0][0];
            QuickGame_Audio_Play(ping, 0);
            if(complexPhysics) {
                ballVelX = -ballVelX;
                ballVelY = ballVelY - ballPaddleCollisionY * 5;
            } else {
                isBallMovingRight = !isBallMovingRight;
            }
            collisionDelay = 7;
        }
        
        if(QuickGame_Sprite_Intersects(animBall[0][currBallAnim], wall)) {
            QuickGame_Audio_Play(pong, 0);
            direction = QuickGame_Sprite_Intersect_Direction(animBall[0][currBallAnim], wall);
            if(complexPhysics) {
                if(direction == QG_DIR_LEFT || direction == QG_DIR_RIGHT) {
                    ballVelX = -ballVelX;
                } 
                ballVelY = ballVelY - (wallYX[0][0] - ballYX[0][0]) * 5;
            } else {
                if(direction == QG_DIR_LEFT || direction == QG_DIR_RIGHT) {
                    isBallMovingRight = !isBallMovingRight;
                } else {
                    isBallMovingUpwards = !isBallMovingUpwards;
                }

            }
            collisionDelay = 7;
        }

        if(QuickGame_Sprite_Intersects(animBall[0][currBallAnim], flipPad)) {
            QuickGame_Audio_Play(pong, 0);
            isControlFlipped = !isControlFlipped;
            handleIsControlFlipped();
            collisionDelay = 12;
        }

        if(QuickGame_Sprite_Intersects(animBall[0][currBallAnim], bluePaddle)) {
            showBlueSplash = true;
            currentScore++;
            QuickGame_Audio_Play(ping, 0);
            ballPaddleCollisionY = bluePaddleY - ballYX[0][0];
            if(complexPhysics) {
                ballVelX = -ballVelX;
                ballVelY = ballVelY - ballPaddleCollisionY * 5;
            } else {
                isBallMovingRight = !isBallMovingRight;
            }
            collisionDelay = 5;
        }
    }

    if(ballYX[0][0] < BALL_RADIUS) {
        isBallMovingUpwards=true;
        ballVelY = -ballVelY;
    }

    if(ballYX[0][0] > SCREEN_HEIGHT) {
        isBallMovingUpwards=false;
        ballVelY = -ballVelY;
    }

    if(ballYX[0][1] < 0 || ballYX[0][1] > SCREEN_WIDTH) {
        checkDeath();
    }
}

void animateWall() {
    wallYX[0][0] -= 0.2f;
}

void animateFlipPad() {
    flipPadY -= 0.2f;
}

void updateAnimations() {

    for(int i = 0; i < 6; i++) {
        animBall[i][currBallAnim]->transform.position.y = ballYX[i][0];
        animBall[i][currBallAnim]->transform.position.x = ballYX[i][1];
    }

    for(int i = 0; i < 5; i++) {
        blueSplashBottom[i]->transform.position.y = bluePaddleY - ballPaddleCollisionY + BALL_RADIUS;
        blueSplashTop[i]->transform.position.y = bluePaddleY - ballPaddleCollisionY - BALL_RADIUS;
    }

    for(int i = 0; i < 5; i++) {
        pinkSplashBottom[i]->transform.position.y = pinkPaddleY - ballPaddleCollisionY + BALL_RADIUS;
        pinkSplashTop[i]->transform.position.y = pinkPaddleY - ballPaddleCollisionY - BALL_RADIUS;
    }

    pinkPaddle->transform.position.y = pinkPaddleY;
    bluePaddle->transform.position.y = bluePaddleY;

    wall->transform.position.y = wallYX[0][0];
    wall->transform.position.x = wallYX[0][1];
    flipPad->transform.position.y = flipPadY;
    flipPad->transform.position.x = flipPadX;

    for(int i = 0; i < 29; i++) {
        endNode[i]->transform.position.y = endNodeY;
        endNode[i]->transform.position.x = endNodeX;
    }
    endBall->transform.position.y = endNodeY;
    endBall->transform.position.x = endNodeX;

    titlePink->transform.position.y = titlePinkPaddleY;
}

void animateRunComplete() {

    float endNodeCenterY = endNodeY;
    float endNodeCenterX = endNodeX;

    if ((endNodeCenterY - 0.2f) > ballYX[0][0] || (endNodeCenterY + 0.2f) < ballYX[0][0]) {

        if(endNodeCenterY != ballYX[0][0]) {
            ballYX[0][0] += (endNodeCenterY - ballYX[0][0])/20.0f;
        }
        if(endNodeCenterX != ballYX[0][1]) {
            ballYX[0][1] += (endNodeCenterX - ballYX[0][1])/20.0f;
        }
    } else {
        QuickGame_Audio_Play(clear, 0);
        checkVictory();
    }
}

void updateBall(double dt) {
    if(complexPhysics) {
        ballYX[0][0] += ballVelY * dt;
        ballYX[0][1] += ballVelX * dt;
    } else {
        if (isBallMovingUpwards == true) {
            ballYX[0][0] += ballVel * dt;
        } else {
            ballYX[0][0] -= ballVel * dt;
        }

        if(isBallMovingRight == true) {
            ballYX[0][1] += ballVel * dt;
        } else {
            ballYX[0][1] -= ballVel *dt;
        }
    }

    if(ballVel < velMax) {
        ballVel += (difficultyLevel * 0.1f);
    }
    if(ballVelY < velMax) {
        ballVelY += (difficultyLevel * 0.1f);
    }
    if(ballVelX < velMax) {
        ballVelX += (difficultyLevel * 0.1f);
    }
}

void updateControls() {
    if(!faceControls) {
        if(QuickGame_Button_Held(PSP_CTRL_LTRIGGER)) {
            if(QuickGame_Button_Held(PSP_CTRL_UP) && (pinkPaddleY < SCREEN_HEIGHT - (PADDLE_HEIGHT/2.0f))) {
                pinkPaddleY += VEL_PADDLE;
            }
            if(QuickGame_Button_Held(PSP_CTRL_DOWN) && (pinkPaddleY > PADDLE_HEIGHT - (PADDLE_HEIGHT/2.0f))) {
                pinkPaddleY -= VEL_PADDLE;
            }
        }

        if(QuickGame_Button_Held(PSP_CTRL_RTRIGGER)) {
            if(QuickGame_Button_Held(PSP_CTRL_UP) && (bluePaddleY < SCREEN_HEIGHT - (PADDLE_HEIGHT/2.0f) + 10.0f)) {
                bluePaddleY += VEL_PADDLE;
            }
            if(QuickGame_Button_Held(PSP_CTRL_DOWN) && (bluePaddleY > PADDLE_HEIGHT - (PADDLE_HEIGHT/2.0f))) {
                bluePaddleY -= VEL_PADDLE;
            }
        }
    } else {
        if(QuickGame_Button_Held(PSP_CTRL_UP) && (pinkPaddleY < SCREEN_HEIGHT - (PADDLE_HEIGHT/2.0f) + 10.0f)) {
            pinkPaddleY += VEL_PADDLE;
        }
        if(QuickGame_Button_Held(PSP_CTRL_DOWN) && (pinkPaddleY > PADDLE_HEIGHT - (PADDLE_HEIGHT/2.0f))) {
            pinkPaddleY -= VEL_PADDLE;
        }

        if(QuickGame_Button_Held(PSP_CTRL_TRIANGLE) && (bluePaddleY < SCREEN_HEIGHT - (PADDLE_HEIGHT/2.0f) + 10.0f)) {
            bluePaddleY += VEL_PADDLE;
        }
        if(QuickGame_Button_Held(PSP_CTRL_CROSS) && (bluePaddleY > PADDLE_HEIGHT - (PADDLE_HEIGHT/2.0f))) {
            bluePaddleY -= VEL_PADDLE;
        }
    }
}

int updateSelectedMenuOption(int selectedOption, int maxOption) {
        if(QuickGame_Button_Pressed(PSP_CTRL_UP)) {
            QuickGame_Audio_Play(ping, 0);
            selectedOption--;
        }

        if(QuickGame_Button_Pressed(PSP_CTRL_DOWN)) {
            QuickGame_Audio_Play(pong, 0);
            selectedOption++;
        }

        if(selectedOption > maxOption) {
            selectedOption = 1;
        }

        if(selectedOption < 1) {
            selectedOption = maxOption;
        }
        return selectedOption;
}

void update(double dt) {
    QuickGame_Input_Update();

    animateBall(dt);
    updateAnimations();


    switch(currentState) {
        case VIEWING_START :
            if(!startAnimShown) {
                animateStart();
            }
            selectedStartOption = updateSelectedMenuOption(selectedStartOption, 3);
            ballYX[0][1] = startMenuOptionCoords[selectedStartOption-1][0];
            ballYX[0][0] = startMenuOptionCoords[selectedStartOption-1][1];

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedStartOption == 1) {
                if(endlessMode) {
                    currentState = ENDLESS_LOADED_NOT_STARTED;
                } else {
                    currentState = VIEWING_PRE_SCROLL;
                }
                break;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedStartOption == 2) {
                currentState = VIEWING_SETTINGS;
                break;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedStartOption == 3) {
                currentState = VIEWING_CREDITS;
                currentCredit = 0;
                break;
            }
            break;
        case VIEWING_SETTINGS :
            for(int i = 0; i < 5; i++)
                ballYX[i+1][0] = settingsMenuOptionCoords[i][1];

            selectedSettingsOption = updateSelectedMenuOption(selectedSettingsOption, 5);
            ballYX[0][1] = settingsMenuOptionCoords[selectedSettingsOption-1][0];
            ballYX[0][0] = settingsMenuOptionCoords[selectedSettingsOption-1][1];

            if((QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) || QuickGame_Button_Pressed(PSP_CTRL_LEFT) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT)) && selectedSettingsOption == 1) {
                allowGlitch = !allowGlitch;
            }

            if(allowGlitch) {
                ballYX[1][1] = 188;
            } else {
                ballYX[1][1] = 296;
            }

            if(selectedSettingsOption == 2) {
                if((QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT))) {
                    difficultyLevel++;
                }
                if (QuickGame_Button_Pressed(PSP_CTRL_LEFT)) {
                    difficultyLevel--;
                }
                if (difficultyLevel > 3) {
                    difficultyLevel = 1;
                }
                if (difficultyLevel < 1) {
                    difficultyLevel = 3;
                }
            }

            if (difficultyLevel == 1) {
                ballYX[2][1] = 258;
            }

            if (difficultyLevel == 2) {
                ballYX[2][1] = 316;
            }

            if (difficultyLevel == 3) {
                ballYX[2][1] = 386;
            }

        
            if((QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) || QuickGame_Button_Pressed(PSP_CTRL_LEFT) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT)) && selectedSettingsOption == 3) {
                faceControls = !faceControls;
            }

            if(!faceControls) {
                ballYX[3][1] = 246;
            } else {
                ballYX[3][1] = 376;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedSettingsOption == 4) {
                complexPhysics = !complexPhysics;
            }

            if(!complexPhysics) {
                ballYX[4][1] = 214;
            } else {
                ballYX[4][1] = 342;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedSettingsOption == 5) {
                endlessMode = !endlessMode;
            }

            if(!endlessMode) {
                ballYX[5][1] = 148;
            } else {
                ballYX[5][1] = 290;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) {
                currentState = VIEWING_START;
                selectedSettingsOption = 1;
                for(int i=1; i<6; i++)
                    ballYX[i][0] = -30.0f;
            }
            break;
        case VIEWING_CREDITS :
            ballYX[0][1] = -30.0f;
            ballYX[0][0] = -30.0f;
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                currentCredit++;
                if(currentCredit > 4) {
                    currentState = VIEWING_START;
                }
            }
            break;
        case VIEWING_PRE_SCROLL:
            ballYX[0][1] = 500.0f;
            updateScroll();
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                moveToNextMission();
            }
            break;
        case LOADED_NOT_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                isBackgroundScrolling = true;
                ballVel = 100.0f;
                ballYX[1][1] = 318;
                ballYX[1][0] = 188;
                ballYX[2][1] = 338;
                ballYX[2][0] = 188;
                ballYX[3][1] = 358;
                ballYX[3][0] = 188;
                randomizeRunVariables();
                randomizeObstacles();
                currentState = STARTED;
            }
            break;
        case STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_START)) {
                currentState = PAUSED;
                break;
            }

            if(QuickGame_Sprite_Intersects(animBall[0][currBallAnim], endBall)) {
                animateRunComplete();
            } else {
                updateBall(dt);
            }
            if(allowGlitch && QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                    randomizeRunVariables();
            }

            updateControls();

            checkBallCollision(dt);

            if(QuickGame_Timer_Elapsed(&timer) >= runLength) {
                animateEndNode(dt);
            }

            if(QuickGame_Timer_Elapsed(&timer) >= wallTimer && (currentMission - difficultyLevel) > 0) {
                animateWall();
            }

            if(QuickGame_Timer_Elapsed(&timer) >= flipPadTimer && (currentMission - difficultyLevel) > 1) {
                animateFlipPad();
            }
        break;
    case PAUSED :
    case ENDLESS_PAUSED:
        if (QuickGame_Button_Pressed(PSP_CTRL_START)) {
            if(currentState == ENDLESS_PAUSED) {
                currentState = ENDLESS_STARTED;
            } else {
                currentState = STARTED;
            }
        }
        if (QuickGame_Button_Pressed(PSP_CTRL_CROSS)) {
            currentState = VIEWING_START;
        }
        break;
    case GAME_COMPLETE:
        isGameComplete = true;
    case RUN_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            moveToNextRun();
        break;
    case MISSION_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            currentState = VIEWING_POST_SCROLL;
        break;
    case VIEWING_POST_SCROLL:
        ballYX[0][1] = 500.0f;
        updateScroll();
        if(isFinalScreenShowing && QuickGame_Button_Pressed(PSP_CTRL_START)) {
            resetGame();
            break;
        }
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
            if(isGameComplete = true) {
                isFinalScreenShowing = true;
            } else {
                textScrollY = -248.0f;
                currentScroll++;
                currentState = VIEWING_PRE_SCROLL;
            }
        }
        break;
    case MOSTLY_DEAD:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            resetRun();
        break;
    case ALL_DEAD:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            resetGame();
        break;
    case ENDLESS_LOADED_NOT_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                isBackgroundScrolling = true;
                ballVel = 100.0f;
                randomizeRunVariables();
                randomizeObstacles();
                currentState = ENDLESS_STARTED;
            }
            break;
    case ENDLESS_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_START)) {
                currentState = ENDLESS_PAUSED;
                break;
            }

            updateBall(dt);
            
            if(allowGlitch && QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                    randomizeRunVariables();
            }

            updateControls();

            checkBallCollision(dt);

            if(QuickGame_Timer_Elapsed(&timer) >= wallTimer) {
                animateWall();
            }

            if(QuickGame_Timer_Elapsed(&timer) >= flipPadTimer) {
                animateFlipPad();
            }

            if(wallYX[0][0] < 0) {
                randomizeWall();
            }

            if(flipPadY < 0) {
                randomizeFlip();
            }
        break;
    case ENDLESS_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
            resetGame();
            currentState = VIEWING_START;
        }
        break;
    }
}

void draw() {
    QuickGame_Graphics_Start_Frame();
    QuickGame_Graphics_Clear();
    
    drawBgScroll();

    switch(currentState) {
        case VIEWING_SETTINGS :
            QuickGame_Sprite_Draw(settingsScreen);
            for(int i = 0; i < 6; i++) {
                QuickGame_Sprite_Draw(animBall[i][currBallAnim]);
            }
            break;
        case VIEWING_CREDITS :
            QuickGame_Sprite_Draw(credits[currentCredit]);
            break;
        case VIEWING_START :
            if (startAnimShown) {
                QuickGame_Sprite_Draw(startScreen);
                QuickGame_Sprite_Draw(animBall[0][currBallAnim]);
            } else {
                QuickGame_Sprite_Draw(titlePink);
                QuickGame_Sprite_Draw(startWhiteScreen);
            }

            break;
        case PAUSED:
        case LOADED_NOT_STARTED :
            drawRemainingAttempts();
            QuickGame_Sprite_Draw(mission[currentMission-1]);
            QuickGame_Sprite_Draw(run[currentRun-1]);
        case STARTED :
            QuickGame_Sprite_Draw(endNode[currEndNodeAnim]);
            QuickGame_Sprite_Draw(endBall);
        case ENDLESS_LOADED_NOT_STARTED:
        case ENDLESS_STARTED:
            QuickGame_Sprite_Draw(wall);
            QuickGame_Sprite_Draw(flipPad);
            QuickGame_Sprite_Draw(pinkPaddle);
            QuickGame_Sprite_Draw(bluePaddle);
            QuickGame_Sprite_Draw(animBall[0][currBallAnim]);
            if(showBlueSplash) {
                QuickGame_Sprite_Draw_Flipped(blueSplashTop[currSplashAnim], splashFlipsBpTb[0]);
                QuickGame_Sprite_Draw_Flipped(blueSplashBottom[currSplashAnim], splashFlipsBpTb[1]);
            }
            if(showPinkSplash) {
                QuickGame_Sprite_Draw_Flipped(pinkSplashTop[currSplashAnim], splashFlipsBpTb[2]);
                QuickGame_Sprite_Draw_Flipped(pinkSplashBottom[currSplashAnim], splashFlipsBpTb[3]);
            }
            break;
        case MOSTLY_DEAD:
            drawRemainingAttempts();
            QuickGame_Sprite_Draw(packetlost);
            break;
        case ALL_DEAD:
            QuickGame_Sprite_Draw(gameover);
            break;
        case RUN_COMPLETE:
            QuickGame_Sprite_Draw(runCompleteScreen);
            break;
        case MISSION_COMPLETE:
            QuickGame_Sprite_Draw(missionCompleteScreen);
            break;
        case GAME_COMPLETE:
            QuickGame_Sprite_Draw(gameCompleteScreen);
            break;
        case VIEWING_PRE_SCROLL:
        case VIEWING_POST_SCROLL:    
            if(isFinalScreenShowing) {
                QuickGame_Sprite_Draw(finalScreen);
            } else {
                QuickGame_Sprite_Draw(scrolls[currentScroll]);
            }
            break;
        case ENDLESS_COMPLETE:
            drawScore();
            break;

    }

    QuickGame_Graphics_End_Frame(true);
}

void loadSprites() {
    QGTexInfo bgForeTexInfo = {.filename = "./assets/sprites/bgFore.png", .flip = true, .vram = 0 };
    bgFore = QuickGame_Sprite_Create_Contained(240, 192, 512, 512, bgForeTexInfo);

    QGTexInfo bgTexInfo = {.filename = "./assets/sprites/bgBack2X.png", .flip = true, .vram = 0 };
    bg = QuickGame_Sprite_Create_Contained(240, 192, 512, 512, bgTexInfo);

    QGTexInfo pink = { .filename = "./assets/sprites/pink.png", .flip = true, .vram = 0 };
    pinkPaddle = QuickGame_Sprite_Create_Contained(LEFT_PADDLE_LANE_X, 120, 8, PADDLE_HEIGHT, pink);

    QGTexInfo blue = { .filename = "./assets/sprites/blue.png", .flip = true, .vram = 0 };
    bluePaddle = QuickGame_Sprite_Create_Contained(RIGHT_PADDLE_LANE_X, 120, 8, PADDLE_HEIGHT, blue);

    QGTexInfo wallTex = { .filename = "./assets/sprites/wall.png", .flip = false, .vram = 0 };
    wall = QuickGame_Sprite_Create_Contained(wallYX[0][1], wallYX[0][0], 8, 40, wallTex);

    QGTexInfo flipPadTex = { .filename = "./assets/sprites/flipPad.png", .flip = false, .vram = 0 };
    flipPad = QuickGame_Sprite_Create_Contained(flipPadX, flipPadY, 40, 40, flipPadTex);

    QGTexInfo endBallTex = { .filename = "./assets/sprites/endBall.png", .flip = true, .vram = 0 };
    endBall = QuickGame_Sprite_Create_Contained(160 - 4 , endNodeY, 12, 12, endBallTex);

    QGTexInfo attemptsTex = { .filename = "./assets/sprites/attempts.png", .flip = true, .vram = 0 };
    attempts = QuickGame_Sprite_Create_Contained(216, 190, 174, 21, attemptsTex);

    QGTexInfo packetlostTex = { .filename = "./assets/sprites/packetlost.png", .flip = true, .vram = 0 };
    packetlost = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, packetlostTex);

    QGTexInfo gameoverTex = { .filename = "./assets/sprites/gameover.png", .flip = true, .vram = 0 };
    gameover = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, gameoverTex);

    QGTexInfo runCompleteTex = { .filename = "./assets/sprites/runComplete.png", .flip = true, .vram = 0 };
    runCompleteScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, runCompleteTex);

    QGTexInfo missionCompleteTex = { .filename = "./assets/sprites/missionComplete.png", .flip = true, .vram = 0 };
    missionCompleteScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, missionCompleteTex);

    QGTexInfo gameCompleteTex = { .filename = "./assets/sprites/gameComplete.png", .flip = true, .vram = 0 };
    gameCompleteScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, gameCompleteTex);

    QGTexInfo finalScreenTex = { .filename = "./assets/sprites/gameCompleteFinal.png", .flip = true, .vram = 0 };
    finalScreen = QuickGame_Sprite_Create_Contained(274, 136, 360, 40, finalScreenTex);

    QGTexInfo startTex = { .filename = "./assets/sprites/start.png", .flip = true, .vram = 0 };
    startScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, startTex);

    QGTexInfo startWhiteTex = { .filename = "./assets/sprites/startWhite.png", .flip = true, .vram = 0 };
    startWhiteScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, startWhiteTex);

    QGTexInfo titlePinkTex = { .filename = "./assets/sprites/titlePink.png", .flip = true, .vram = 0 };
    titlePink = QuickGame_Sprite_Create_Contained(101.5f, 300, 17, 24, titlePinkTex);

    QGTexInfo settingsTex = { .filename = "./assets/sprites/options.png", .flip = true, .vram = 0 };
    settingsScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, settingsTex);

    for(int i = 0; i < 6; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/scrolls/%d.png", i);

        QGTexInfo scrollTex = { .filename = filename, .flip = true, .vram = 0 };
        scrolls[i] = QuickGame_Sprite_Create_Contained(240, -512, 512, 512,  scrollTex);
    }

    for(int i = 0; i < 5; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/credits/%d.png", i);

        QGTexInfo creditsTex = { .filename = filename, .flip = true, .vram = 0 };
        credits[i] = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, creditsTex);
    }

    for(int j = 0; j < 7; j++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/ball/%d.png", j);

        QGTexInfo ballTex = { .filename = filename, .flip = true, .vram = 0 };
        for(int i = 0; i < 6; i++) {
            animBall[i][j] = QuickGame_Sprite_Create_Contained(160, 136, BALL_RADIUS * 2, BALL_RADIUS * 2, ballTex);
        }
    }

    for(int i = 0; i < 3; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/mission/%d.png", i);

        QGTexInfo missionTex = { .filename = filename, .flip = true, .vram = 0 };
        mission[i] = QuickGame_Sprite_Create_Contained(240, 162, 174, 21, missionTex);
    }

    for(int i = 0; i < 5; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/run/%d.png", i);

        QGTexInfo runTex = { .filename = filename, .flip = true, .vram = 0 };
        run[i] = QuickGame_Sprite_Create_Contained(240, 136, 174, 21, runTex);
    }

    for(int i = 0; i < 10; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/nums/%d.png", i);

        QGTexInfo sc = { .filename = filename, .flip = true, .vram = 0 };
        nums[i] = QuickGame_Sprite_Create_Contained(240, 136, 32, 64, sc);
    }

    for(int i = 0; i < 6; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/blueSplash/%d.png", i);

        QGTexInfo splashTex = { .filename = filename, .flip = false, .vram = 0 };
        blueSplashTop[i] = QuickGame_Sprite_Create_Contained(240, 136, 16, 16, splashTex);
        blueSplashBottom[i] = QuickGame_Sprite_Create_Contained(240, 136, 16, 16, splashTex);
    }

    for(int i = 0; i < 6; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/pinkSplash/%d.png", i);

        QGTexInfo splashTex = { .filename = filename, .flip = false, .vram = 0 };
        pinkSplashTop[i] = QuickGame_Sprite_Create_Contained(240, 136, 16, 16, splashTex);
        pinkSplashBottom[i] = QuickGame_Sprite_Create_Contained(240, 136, 16, 16, splashTex);
    }

    for(int i = 0; i < 29; i++) {
        char filename[256];
        sprintf(filename, "./assets/sprites/endNode/%d.png", i);

        QGTexInfo endNodeTex = { .filename = filename, .flip = false, .vram = 0 };
        endNode[i] = QuickGame_Sprite_Create_Contained(160, endNodeY, 32, 32, endNodeTex);
    }
}

void loadAudio() {
    ping = QuickGame_Audio_Load( "./assets/audio/uipack1/MENU A_Select.wav" , false, false);
    pong = QuickGame_Audio_Load( "./assets/audio/uipack1/MENU A - Back.wav" , false, false);
    clear = QuickGame_Audio_Load( "./assets/audio/uipack1/MESSAGE-B_Accept.wav" , false, false);
    fail = QuickGame_Audio_Load( "./assets/audio/uipack1/ALERT_Error.wav" , false, false);

    runMusic1 = QuickGame_Audio_Load( "./assets/audio/music/raining_gif(loop).wav" , true, false);
    runMusic2 = QuickGame_Audio_Load( "./assets/audio/music/tgfcoder-FrozenJam-SeamlessLoop.wav" , true, false);
}

int main(int argc, char *argv[]) {
    if(QuickGame_Init() < 0)
        return 1;

    QuickGame_Audio_Init();
    QuickGame_Graphics_Set2D();
    QuickGame_Timer_Start(&timer);

    loadSprites();
    loadAudio();
    resetGame();

    srand(time(NULL));

    while(true) {
        update(QuickGame_Timer_Delta(&timer));
        draw();
    }
    
    QuickGame_Audio_Terminate();
    QuickGame_Terminate();
    return 0;
}