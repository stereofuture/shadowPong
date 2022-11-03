#include <QuickGame.h>
#include <gu2gl.h>
#include <pspctrl.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pspkernel.h>
#include <pspdebug.h>

/*
TODOS
1. Add ending scroll
5. Add high score mode
6. Improve sprite detail
6a. Add depth to BG scroll
6b. Animate end node
6c. Animate paddles
6c1. Add splash for ball collision
6c2. Add up/down animation
7. Add alternate physics (simple V complex)
9. Add story messages
11. Add other obstacles (blocks, flips)
12. Add powerups (speed boost, paddle embiggener)
16. Change ball launch button to be compatible with glitch (CROSS currently moves and glitches)
17. Clean up
17a. Align variable, asset and sprite naming
17c. Split into separate files
20. Add ability to back out of current run
*/ 

/*
POST TODOS
8c. Difficulty - Obstacle frequency
15. Add scroll speed
15a. Synch music with scroll speed
19. Figure out paddle top 10.0f requirement
21. Add Settings option to pause state
*/

/*
WONTDOS
16. Re-implement char overflowable_ball_vel
*/

QGSprite_t bg, attempts, pinkPaddle, bluePaddle, ball, endNode, endBall;
QGSprite_t startScreen, settingsScreen
, packetlost, gameover, runCompleteScreen, missionCompleteScreen, gameCompleteScreen, gameCompleteScrollScreen;
QGSprite_t credits[5];
QGSprite_t score[10];
QGSprite_t animBall[7];
QGSprite_t animBall0[7];
QGSprite_t animBall1[7];
QGSprite_t animBall2[7];
QGSprite_t run[5];
QGSprite_t mission[3];


QGAudioClip_t ping, pong, fail, clear, levelMusic1, levelMusic2;

QGTimer timer;

typedef enum {
    PAUSED,
    LOADED_NOT_STARTED,
    STARTED,
    VIEWING_CREDITS,
    VIEWING_SETTINGS,
    VIEWING_START,
    RUN_COMPLETE,
    MISSION_COMPLETE,
    GAME_COMPLETE,
    MOSTLY_DEAD,
    ALL_DEAD,
} GAMESTATES;

GAMESTATES current_state;

float ball_y, ball_x;
float ball0_y, ball0_x;
float ball1_y, ball1_x;
float ball2_y, ball2_x;
float pinkPaddle_y, bluePaddle_y;
float endNode_y, endNode_x;
float ball_vel;
float vel_mod;
float bg_scroll_vel;
int current_score;
bool glitched;
bool ballUp;
bool ballRight;
bool scroll_bg;
float run_length;
int currentRun;
int currentMission;
int remainingAttempts;
int selectedStartOption;
int selectedSettingsOption;

int startMenuOptionCoords[3][2] = {
    {174, 130},
    {148, 116},
    {160, 102}
};

int settingsMenuOptionCoords[5][2] = {
    {14, 175},
    {14, 160},
    {14, 145},
    {14, 130},
    {14, 115}
};

float paddle_height = 50.0f;
float endNode_width = 22.0f;
float screen_height = 272.0f;
float screen_width = 480.0f;
float vel_paddle = 5.0f;
float left_paddle_lane_x = 10.0f;
float right_paddle_lane_x = 473.0f;
float ball_height = 7.0f;
// ~35s for max starting at 100.0f and +0.1f
//  ~11s for max starting at 100.0f and +0.3f
float vel_max = 300.0f;
int collision_delay = 0;
int currentCredit = 0;
int difficultyLevel = 1;
float end_animation_length = 5.0f;

bool faceControls = false;
bool allowGlitch = false;

int curr_ball_anim = 0;
float anim_time = 0.0f;

void animate_ball(double dt) {
    anim_time += dt;

    if(anim_time > 0.15f) {
        curr_ball_anim++;
        anim_time = 0.0f;

        if(curr_ball_anim == 7)
            curr_ball_anim = 0;
    }
}

void draw_bg_scroll() {
    if(scroll_bg) {
        glTexOffset(0.0f, timer.total * 0.2f);
    }
    bg->transform.position.y = 128;
    QuickGame_Sprite_Draw(bg);

    glTexOffset(0.0f, 0.0f);
}

void draw_remaining_attempts() {
    if(remainingAttempts > 2) {
        QuickGame_Sprite_Draw(animBall2[curr_ball_anim]);
    }

    if(remainingAttempts > 1) {
        QuickGame_Sprite_Draw(animBall1[curr_ball_anim]);;
    }

    if(remainingAttempts > 0) {
        QuickGame_Sprite_Draw(animBall0[curr_ball_anim]);
    }

    QuickGame_Sprite_Draw(attempts);
}

void draw_ending_scroll() {
    glTexOffset(0.0f, timer.total * 0.1f);
    bg->transform.position.y = 128;
    QuickGame_Sprite_Draw(gameCompleteScrollScreen);

    glTexOffset(0.0f, 0.0f);
}

void randomize_level_variables() {
    int start_seed = rand() % 4;

    switch(start_seed) {
        case 0 :
            ballUp = false; 
            ballRight = false;
            break;
        case 1 :
            ballUp = true; 
            ballRight = true;
            break;
        case 2 :
            ballUp = true; 
            ballRight = false;
            break;
        case 3 :
            ballUp = false; 
            ballRight = true;
            break;
        default :
            ballUp = false; 
            ballRight = false;
    }

    endNode_x = 100 + rand() % 280;
}

void reset_game() {
    ball_y = 136.0f;
    ball_x = 240.0f;
    pinkPaddle_y = 136.0f;
    bluePaddle_y = 136.0f;
    glitched = false;
    ball_vel = 0.0f;
    vel_mod = 1.0f;
    endNode_y = 400.0f;
    // First run on any difficulty is always the same length
    run_length = 5.0f;
    scroll_bg = false;
    currentCredit = 0;
    current_state = LOADED_NOT_STARTED;

    QuickGame_Timer_Reset(&timer);
    QuickGame_Audio_Play(levelMusic1, 1);
}

void reset_game_completely() {
    reset_game();
    currentMission = 1;
    remainingAttempts = 3;
    currentRun = 1;
    selectedStartOption = 1;
    selectedSettingsOption = 1;
    current_state = VIEWING_START;

    QuickGame_Audio_Play(levelMusic2, 1);
}

void move_to_next_level() {
    ball_y = 136.0f;
    ball_x = 240.0f;
    pinkPaddle_y = 136.0f;
    bluePaddle_y = 136.0f;
    current_state = LOADED_NOT_STARTED;
    glitched = false;
    ball_vel = 0.0f;
    vel_mod = 1.0f;
    endNode_y = 400.0f;
    endNode_x = 160;
    remainingAttempts = 3;
    // First run - ~3s, Second run - ~ 7s?
    run_length = 5.0f + (difficultyLevel * currentRun * 1.0f);
    vel_max = 300.0f + ((difficultyLevel - 1) * 25.0f);
}

void checkDeath() {
    current_state = MOSTLY_DEAD;
    remainingAttempts--;
    if(remainingAttempts < 0) {
        current_state = ALL_DEAD;
    }
}

void checkVictory() {
    current_state = RUN_COMPLETE;
    currentRun++;
    if(currentRun > 5) {
        currentMission++;
        currentRun = 1;
        current_state = MISSION_COMPLETE;
    }
    if(currentMission > 2) {
        current_state = GAME_COMPLETE;
    }
}

void animate_endNode() {
    if(endNode_y > 120.0f) {
        endNode_y -= 1.0f;
    } else {
        scroll_bg = false;
    }
}

void animation_update() {
    animBall[curr_ball_anim]->transform.position.y = ball_y;
    animBall[curr_ball_anim]->transform.position.x = ball_x;

    animBall0[curr_ball_anim]->transform.position.y = ball0_y;
    animBall0[curr_ball_anim]->transform.position.x = ball0_x;

    animBall1[curr_ball_anim]->transform.position.y = ball1_y;
    animBall1[curr_ball_anim]->transform.position.x = ball1_x;
    
    animBall2[curr_ball_anim]->transform.position.y = ball2_y;
    animBall2[curr_ball_anim]->transform.position.x = ball2_x;

    pinkPaddle->transform.position.y = pinkPaddle_y;
    bluePaddle->transform.position.y = bluePaddle_y;

    endNode->transform.position.y = endNode_y;
    endNode->transform.position.x = endNode_x;
    endBall->transform.position.y = endNode_y - 3;
    endBall->transform.position.x = endNode_x - 3;

    // if(glitched == true) {
    //     bluePaddle->transform.position.x = left_paddle_lane_x;
    //     pinkPaddle->transform.position.x = right_paddle_lane_x;
    // }
}

void animate_runComplete() {

    float endNode_center_y = endNode_y - 3;
    float endNode_center_x = endNode_x - 3;

    if ((endNode_center_y - 0.2f) > ball_y || (endNode_center_y + 0.2f) < ball_y) {

        if(endNode_center_y != ball_y) {
            ball_y += (endNode_center_y - ball_y)/20.0f;
        }
        if(endNode_center_x != ball_x) {
            ball_x += (endNode_center_x - ball_x)/20.0f;
        }
    } else {
        QuickGame_Audio_Play(clear, 0);
        checkVictory();
    }
}

void update_ball(double dt) {
    if (ballUp == true) {
        ball_y += ball_vel * dt;
    } else {
        ball_y -= ball_vel * dt;
    }

    if(ballRight == true) {
        ball_x += ball_vel * dt;
    } else {
        ball_x -= ball_vel *dt;
    }

    if(ball_vel < vel_max) {
        ball_vel += (difficultyLevel * 0.1f);
    }
}

int update_selected_menu_option(int selectedOption, int maxOption) {
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

    animate_ball(dt);
    animation_update();


    switch(current_state) {
        case VIEWING_START :
            selectedStartOption = update_selected_menu_option(selectedStartOption, 3);
            ball_x = startMenuOptionCoords[selectedStartOption-1][0];
            ball_y = startMenuOptionCoords[selectedStartOption-1][1];

            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS) && selectedStartOption == 1) {
                current_state = LOADED_NOT_STARTED;
                break;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS) && selectedStartOption == 2) {
                current_state = VIEWING_SETTINGS;
                break;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS) && selectedStartOption == 3) {
                current_state = VIEWING_CREDITS;
                currentCredit = 0;
                break;
            }
            break;
        case VIEWING_SETTINGS :
            ball0_y = 175;
            ball1_y = 160;
            ball2_y = 145;

            selectedSettingsOption = update_selected_menu_option(selectedSettingsOption, 5);
            ball_x = settingsMenuOptionCoords[selectedSettingsOption-1][0];
            ball_y = settingsMenuOptionCoords[selectedSettingsOption-1][1];

            if((QuickGame_Button_Pressed(PSP_CTRL_CROSS) || QuickGame_Button_Pressed(PSP_CTRL_LEFT) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT)) && selectedSettingsOption == 1) {
                allowGlitch = !allowGlitch;
            }

            if(allowGlitch) {
                ball0_x = 188;
            } else {
                ball0_x = 296;
            }

            if(selectedSettingsOption == 2) {
                if((QuickGame_Button_Pressed(PSP_CTRL_CROSS) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT))) {
                    difficultyLevel++;
                }
                if (QuickGame_Button_Pressed(PSP_CTRL_LEFT)){
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
                ball1_x = 258;
            }

            if (difficultyLevel == 2) {
                ball1_x = 316;
            }

            if (difficultyLevel == 3) {
                ball1_x = 386;
            }

        
            if((QuickGame_Button_Pressed(PSP_CTRL_CROSS) || QuickGame_Button_Pressed(PSP_CTRL_LEFT) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT)) && selectedSettingsOption == 3) {
                    faceControls = !faceControls;
            }

            if(!faceControls) {
                ball2_x = 246;
            } else {
                ball2_x = 376;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS) && selectedSettingsOption == 4) {
                // TODO: Implement physics option
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                current_state = VIEWING_START;
                selectedSettingsOption = 1;
                ball0_y = -20;
                ball1_y = -20;
                ball2_y = -20;
            }
            break;
        case VIEWING_CREDITS :
            ball_x = -20;
            ball_y = -20;
            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) {
                currentCredit++;
                if(currentCredit > 4) {
                    current_state = VIEWING_START;
                }
            }
            break;
        case LOADED_NOT_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)){
                scroll_bg = true;
                ball_vel = 100.0f;
                ball0_x = 260;
                ball0_y = 187;
                ball1_x = 280;
                ball1_y = 187;
                ball2_x = 300;
                ball2_y = 187;
                randomize_level_variables();
                current_state = STARTED;
            }
            break;
        case STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_START)) {
                current_state = PAUSED;
                break;
            }

            if(QuickGame_Sprite_Intersects(animBall[curr_ball_anim], endBall)){
                animate_runComplete();
            } else {
                update_ball(dt);
            }
            
            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)){
                if(allowGlitch) {
                    randomize_level_variables();
                }
            }

            if(!faceControls) {
                if(QuickGame_Button_Held(PSP_CTRL_LTRIGGER)){
                    if(QuickGame_Button_Held(PSP_CTRL_UP) && (pinkPaddle_y < screen_height - (paddle_height/2.0f) + 10.0f)){
                        pinkPaddle_y += vel_paddle;
                    }
                    if(QuickGame_Button_Held(PSP_CTRL_DOWN) && (pinkPaddle_y > paddle_height - (paddle_height/2.0f))){
                        pinkPaddle_y -= vel_paddle;
                    }
                }

                if(QuickGame_Button_Held(PSP_CTRL_RTRIGGER)){
                    if(QuickGame_Button_Held(PSP_CTRL_UP) && (bluePaddle_y < screen_height - (paddle_height/2.0f) + 10.0f)){
                        bluePaddle_y += vel_paddle;
                    }
                    if(QuickGame_Button_Held(PSP_CTRL_DOWN) && (bluePaddle_y > paddle_height - (paddle_height/2.0f))){
                        bluePaddle_y -= vel_paddle;
                    }
                }
            } else {
                if(QuickGame_Button_Held(PSP_CTRL_UP) && (pinkPaddle_y < screen_height - (paddle_height/2.0f) + 10.0f)){
                    pinkPaddle_y += vel_paddle;
                }
                if(QuickGame_Button_Held(PSP_CTRL_DOWN) && (pinkPaddle_y > paddle_height - (paddle_height/2.0f))){
                    pinkPaddle_y -= vel_paddle;
                }

                if(QuickGame_Button_Held(PSP_CTRL_TRIANGLE) && (bluePaddle_y < screen_height - (paddle_height/2.0f) + 10.0f)){
                    bluePaddle_y += vel_paddle;
                }
                if(QuickGame_Button_Held(PSP_CTRL_CROSS) && (bluePaddle_y > paddle_height - (paddle_height/2.0f))){
                    bluePaddle_y -= vel_paddle;
                }
            }

            if(collision_delay >= 0) {
                collision_delay--;
            } else {
                if(QuickGame_Sprite_Intersects(animBall[curr_ball_anim], bluePaddle)) {
                    QuickGame_Audio_Play(ping, 0);
                    ballRight = !ballRight;
                    collision_delay = 5;
                }
                
                if(QuickGame_Sprite_Intersects(animBall[curr_ball_anim], pinkPaddle)) {
                    QuickGame_Audio_Play(pong, 0);
                    ballRight = !ballRight;
                    collision_delay = 5;
                }
            }

            if(QuickGame_Timer_Elapsed(&timer) >= run_length){
                animate_endNode();
            }

            if(ball_y < ball_height)
                ballUp=true;

            if(ball_y > screen_height)
                ballUp=false;

            if(ball_x < 0 || ball_x > screen_width) {
                QuickGame_Audio_Play(fail, 0);
                checkDeath();
                // ballRight=!ballRight;
            }
        break;
    case PAUSED :
        if (QuickGame_Button_Pressed(PSP_CTRL_START)) {
            current_state = STARTED;
        }
        break;
    case RUN_COMPLETE:
    case MISSION_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) 
            move_to_next_level();
        break;
    case GAME_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) 
            reset_game_completely();
        break;
    case MOSTLY_DEAD:
        if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) 
            reset_game();
        break;
    case ALL_DEAD:
        if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) 
            reset_game_completely();
        break;
    }
}

void draw() {
    QuickGame_Graphics_Start_Frame();
    QuickGame_Graphics_Clear();
    
    draw_bg_scroll();

    switch(current_state) {
        case VIEWING_SETTINGS :
            QuickGame_Sprite_Draw(settingsScreen);
            QuickGame_Sprite_Draw(animBall0[curr_ball_anim]);
            QuickGame_Sprite_Draw(animBall1[curr_ball_anim]);
            QuickGame_Sprite_Draw(animBall2[curr_ball_anim]);
            break;
        case VIEWING_CREDITS :
            QuickGame_Sprite_Draw(credits[currentCredit]);
            break;
        case VIEWING_START :
            QuickGame_Sprite_Draw(startScreen);
            break;
        case STARTED :
            QuickGame_Sprite_Draw(endNode);
            QuickGame_Sprite_Draw(endBall);
            break;
        case LOADED_NOT_STARTED :
            draw_remaining_attempts();
            QuickGame_Sprite_Draw(mission[currentMission-1]);
            QuickGame_Sprite_Draw(run[currentRun-1]);
            break;
        case MOSTLY_DEAD:
            draw_remaining_attempts();
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
            // TODO: Add epilogue scroll
            break;
    }

    QuickGame_Sprite_Draw(pinkPaddle);
    QuickGame_Sprite_Draw(bluePaddle);
    QuickGame_Sprite_Draw(animBall[curr_ball_anim]);

    QuickGame_Graphics_End_Frame(true);
}

void load_sprites() {
    QGTexInfo bgTexInfo = {.filename = "./assets/sprites/bg.png", .flip = true, .vram = 0 };
    bg = QuickGame_Sprite_Create_Contained(240, 192, 512, 512, bgTexInfo);

    QGTexInfo pink = { .filename = "./assets/sprites/pink.png", .flip = true, .vram = 0 };
    pinkPaddle = QuickGame_Sprite_Create_Contained(left_paddle_lane_x, 120, 8, paddle_height, pink);

    QGTexInfo blue = { .filename = "./assets/sprites/blue.png", .flip = true, .vram = 0 };
    bluePaddle = QuickGame_Sprite_Create_Contained(right_paddle_lane_x, 120, 8, paddle_height, blue);

    QGTexInfo ballTex = { .filename = "./assets/sprites/ball.png", .flip = true, .vram = 0 };

    QGTexInfo endBallTex = { .filename = "./assets/sprites/endBall.png", .flip = true, .vram = 0 };
    endBall = QuickGame_Sprite_Create_Contained(160 - 4 , endNode_y, 12, 12, endBallTex);

    QGTexInfo endNodeTex = { .filename = "./assets/sprites/endNode.png", .flip = true, .vram = 0 };
    endNode = QuickGame_Sprite_Create_Contained(160, endNode_y, 22, 22, endNodeTex);

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

    QGTexInfo gameCompleteScrollTex = { .filename = "./assets/sprites/gameCompleteScroll.png", .flip = true, .vram = 0 };
    gameCompleteScrollScreen = QuickGame_Sprite_Create_Contained(240, 136, 586, 558 , gameCompleteScrollTex);

    QGTexInfo startTex = { .filename = "./assets/sprites/start.png", .flip = true, .vram = 0 };
    startScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, startTex);

    QGTexInfo settingsTex = { .filename = "./assets/sprites/options.png", .flip = true, .vram = 0 };
    settingsScreen = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, settingsTex);

    for(int i = 0; i < 5; i++){
        char filename[256];
        sprintf(filename, "./assets/sprites/credits/%d.png", i);

        QGTexInfo creditsTex = { .filename = filename, .flip = true, .vram = 0 };
        credits[i] = QuickGame_Sprite_Create_Contained(240, 136, 512, 128, creditsTex);
    }

    for(int i = 0; i < 7; i++){
        char filename[256];
        sprintf(filename, "./assets/sprites/ball/%d.png", i);

        QGTexInfo ballTex = { .filename = filename, .flip = true, .vram = 0 };
        animBall[i] = QuickGame_Sprite_Create_Contained(160, 136, 14, 14, ballTex);
        animBall0[i] = QuickGame_Sprite_Create_Contained(260, 187, 14, 14, ballTex);
        animBall1[i] = QuickGame_Sprite_Create_Contained(280, 187, 14, 14, ballTex);
        animBall2[i] = QuickGame_Sprite_Create_Contained(300, 187, 14, 14, ballTex);
    }

    for(int i = 0; i < 3; i++){
        char filename[256];
        sprintf(filename, "./assets/sprites/mission/%d.png", i);

        QGTexInfo missionTex = { .filename = filename, .flip = true, .vram = 0 };
        mission[i] = QuickGame_Sprite_Create_Contained(240, 170, 174, 21, missionTex);
    }

    for(int i = 0; i < 5; i++){
        char filename[256];
        sprintf(filename, "./assets/sprites/run/%d.png", i);

        QGTexInfo runTex = { .filename = filename, .flip = true, .vram = 0 };
        run[i] = QuickGame_Sprite_Create_Contained(240, 150, 174, 21, runTex);
    }
}

void load_audio() {
    ping = QuickGame_Audio_Load( "./assets/audio/uipack1/MENU A_Select.wav" , false, false);
    pong = QuickGame_Audio_Load( "./assets/audio/uipack1/MENU A - Back.wav" , false, false);
    clear = QuickGame_Audio_Load( "./assets/audio/uipack1/MESSAGE-B_Accept.wav" , false, false);
    fail = QuickGame_Audio_Load( "./assets/audio/uipack1/ALERT_Error.wav" , false, false);

    levelMusic1 = QuickGame_Audio_Load( "./assets/audio/music/raining_gif(loop).wav" , true, false);
    levelMusic2 = QuickGame_Audio_Load( "./assets/audio/music/tgfcoder-FrozenJam-SeamlessLoop.wav" , true, false);
}

int main(int argc, char *argv[]) {
    pspDebugScreenInit();
    if(QuickGame_Init() < 0)
        return 1;

    QuickGame_Audio_Init();
    QuickGame_Graphics_Set2D();
    QuickGame_Timer_Start(&timer);

    load_sprites();
    load_audio();
    reset_game_completely();

    srand(time(NULL));

    while(true) {
        update(QuickGame_Timer_Delta(&timer));
        draw();
    }
    
    QuickGame_Audio_Terminate();
    QuickGame_Terminate();
    return 0;
}