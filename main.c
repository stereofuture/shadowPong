#include <QuickGame.h>
#include <gu2gl.h>
#include <pspctrl.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <pspkernel.h>
#include <pspdebug.h>

/*
TODOS
6. Improve sprite detail
6b. Animate end node
6c. Animate paddles
6c1. Add splash for ball collision
6c2. Add up/down animation
7. Animate title with paddle H
9. Add story messages
17. Clean up
17a. Align variable, asset and sprite naming
17b. Optimize redundant code (checking flip every update)
17c. Split into separate files
17d. Use more loops for sprite creation
*/ 

/*
POST TODOS
4. Add checks to move obstalce to ensure they don't overlap node and eachother
12. Add powerups (speed boost, paddle embiggener, multiball)
13. Add pause quit instruction screen
15. Add scroll speed
15a. Synch music with scroll speed
19. Figure out paddle top 10.0f requirement
21. Add Settings option to pause state
*/

/*
WONTDOS
16. Re-implement char overflowable_ball_vel
*/

QGSprite_t bg, bgFore, attempts, pinkPaddle, bluePaddle, ball, endNode, endBall, wall, flipPad;
QGSprite_t startScreen, settingsScreen
, packetlost, gameover, runCompleteScreen, missionCompleteScreen, gameCompleteScreen, gameCompleteScrollScreen, finalScreen;
QGSprite_t credits[5];
QGSprite_t score[10];
QGSprite_t animBall[6][7];
QGSprite_t run[5];
QGSprite_t mission[3];
QGSprite_t nums[10];

enum QGDirection direction;

QGAudioClip_t ping, pong, fail, clear, levelMusic1, levelMusic2;

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
    VIEWING_ENDING,
    MOSTLY_DEAD,
    ALL_DEAD,
} GAMESTATE;

GAMESTATE current_state;

float ball_yx[6][2];
float pinkPaddle_y, bluePaddle_y;
float wall_x, wall_y;
float flipPad_x, flipPad_y;
float endNode_y, endNode_x;
float ball_vel;
float ball_vel_x, ball_vel_y;
float ending_scroll_y;
float vel_mod;
float bg_scroll_vel;
float bg_scroll_offset;
float bg_back_scroll_offset;
int current_score;
bool glitched;
bool ballUp;
bool ballRight;
bool scroll_bg;
bool show_final_screen;
float run_length;
float flipPad_timer;
float wall_timer;
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

bool faceControls = false;
bool allowGlitch = false;
bool complexPhysics = false;
bool endlessMode = false;

int curr_ball_anim = 0;
float anim_time = 0.0f;
float scroll_time = 0.0f;

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
        bg_back_scroll_offset = timer.total * 0.15f;
        bg_scroll_offset = timer.total * 0.08f;
    }
    glTexOffset(0.0f, bg_back_scroll_offset);
    QuickGame_Sprite_Draw(bg);
    glTexOffset(0.0f, bg_scroll_offset);
    QuickGame_Sprite_Draw(bgFore);
    glTexOffset(0.0f, 0.0f);
}

void draw_remaining_attempts() {
    for(int i = 0; i < remainingAttempts; i++){
        QuickGame_Sprite_Draw(animBall[i+1][curr_ball_anim]);
    }
    QuickGame_Sprite_Draw(attempts);
}

void draw_ending_scroll() {
    glTexOffset(0.0f, timer.total * -0.025f);
    QuickGame_Sprite_Draw(gameCompleteScrollScreen);
    glTexOffset(0.0f, 0.0f);
}

void draw_score(){
    int digits = snprintf( NULL, 0, "%d", current_score);

    float xoff = -((float)digits-1) / 2.0f;
    xoff *= 32.0f;

    float xn = 0.0f;

    int s = current_score;
    for(int i = 0; i < digits; i++){
        int c = s % 10;
        s /= 10;

        nums[c]->transform.position.x = -xoff + 240 - xn;
        nums[c]->transform.position.y = 192;

        xn += 32.0f;

        QuickGame_Sprite_Draw(nums[c]);
    }
}

void update_ending_scroll(double dt) {
    scroll_time += dt;
    ending_scroll_y += scroll_time * 0.05f;
    if(ending_scroll_y > 600.0f) {
        show_final_screen = true;
        scroll_time = 0.0f;
    } else {
        gameCompleteScrollScreen->transform.position.y = ending_scroll_y;   
    }
}

void randomize_flip() {
    flipPad_timer = fmod(rand(), run_length);
    flipPad_y = 300.0f + fmod(rand(),100.0f);
    flipPad_x = 180 + rand() % 120;
}

void randomize_wall() {
    wall_timer = fmod(rand(), run_length);
    wall_y = 300.0f + fmod(rand(),100.0f);
    wall_x = 120 + rand() % 240;
}

void randomize_obstacles() {
    randomize_flip();
    randomize_wall();
}

void randomize_level_variables() {
    int start_seed = rand() % 4;

    switch(start_seed) {
        case 0 :
            ballUp = false; 
            ballRight = false;
            ball_vel_x = ball_vel;
            ball_vel_y = -ball_vel;
            break;
        case 1 :
            ballUp = true; 
            ballRight = true;
            ball_vel_x = ball_vel;
            ball_vel_y = ball_vel;
            break;
        case 2 :
            ballUp = true; 
            ballRight = false;
            ball_vel_x = -ball_vel;
            ball_vel_y = ball_vel;
            break;
        case 3 :
            ballUp = false; 
            ballRight = true;
            ball_vel_x = -ball_vel;
            ball_vel_y = -ball_vel;
            break;
        default :
            ballUp = false; 
            ballRight = false;
            ball_vel_x = ball_vel;
            ball_vel_y = -ball_vel;
    }

    endNode_x = 100 + rand() % 280;
}

void reset_game() {
    ball_yx[0][0] = 136.0f;
    ball_yx[0][1] = 240.0f;
    pinkPaddle_y = 136.0f;
    bluePaddle_y = 136.0f;
    wall_y = 300.0f;
    flipPad_y = 300.0f;
    glitched = false;
    ball_vel = 0.0f;
    ball_vel_x = 0.0f;
    ball_vel_y = 0.0f;
    vel_mod = 1.0f;
    endNode_y = 400.0f;

    scroll_bg = false;
    currentCredit = 0;
    if(endlessMode) {
        current_state = ENDLESS_LOADED_NOT_STARTED;
    } else {
        current_state = LOADED_NOT_STARTED;
    }

    QuickGame_Timer_Reset(&timer);
    QuickGame_Audio_Play(levelMusic1, 1);
}

void reset_game_completely() {
    reset_game();
    currentMission = 1;
    remainingAttempts = 3;
    currentRun = 1;
    current_score = 0;
    selectedStartOption = 1;
    selectedSettingsOption = 1;
    current_state = VIEWING_START;
    show_final_screen = false;
    ending_scroll_y = -254.0f;
    // First run on any difficulty is always the same length
    run_length = 5.0f;

    QuickGame_Audio_Play(levelMusic2, 1);
}

void move_to_next_level() {
    ball_yx[0][0] = 136.0f;
    ball_yx[0][1] = 240.0f;
    pinkPaddle_y = 136.0f;
    bluePaddle_y = 136.0f;
    current_state = LOADED_NOT_STARTED;
    glitched = false;
    ball_vel = 0.0f;
    ball_vel_x = 0.0f;
    ball_vel_y = 0.0f;
    vel_mod = 1.0f;
    wall_y = 320.0f;
    flipPad_y = 320.0f;
    endNode_y = 400.0f;
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
    if(currentMission > 3) {
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

void animate_wall() {
    wall_y -= 0.2f;
}

void animate_flipPad() {
    flipPad_y -= 0.2f;
}

void animation_update() {

    for(int i = 0; i < 6; i++){
        animBall[i][curr_ball_anim]->transform.position.y = ball_yx[i][0];
        animBall[i][curr_ball_anim]->transform.position.x = ball_yx[i][1];
    }

    pinkPaddle->transform.position.y = pinkPaddle_y;
    bluePaddle->transform.position.y = bluePaddle_y;

    wall->transform.position.y = wall_y;
    wall->transform.position.x = wall_x;
    flipPad->transform.position.y = flipPad_y;
    flipPad->transform.position.x = flipPad_x;

    endNode->transform.position.y = endNode_y;
    endNode->transform.position.x = endNode_x;
    endBall->transform.position.y = endNode_y - 3;
    endBall->transform.position.x = endNode_x - 3;

    if(glitched == true) {
        bluePaddle->transform.position.x = left_paddle_lane_x;
        pinkPaddle->transform.position.x = right_paddle_lane_x;
    } else {
        bluePaddle->transform.position.x = right_paddle_lane_x;
        pinkPaddle->transform.position.x = left_paddle_lane_x;
    }
}

void animate_runComplete() {

    float endNode_center_y = endNode_y - 3;
    float endNode_center_x = endNode_x - 3;

    if ((endNode_center_y - 0.2f) > ball_yx[0][0] || (endNode_center_y + 0.2f) < ball_yx[0][0]) {

        if(endNode_center_y != ball_yx[0][0]) {
            ball_yx[0][0] += (endNode_center_y - ball_yx[0][0])/20.0f;
        }
        if(endNode_center_x != ball_yx[0][1]) {
            ball_yx[0][1] += (endNode_center_x - ball_yx[0][1])/20.0f;
        }
    } else {
        QuickGame_Audio_Play(clear, 0);
        checkVictory();
    }
}

void update_ball(double dt) {
    if(complexPhysics) {
        ball_yx[0][0] += ball_vel_y * dt;
        ball_yx[0][1] += ball_vel_x * dt;
    } else {
        if (ballUp == true) {
            ball_yx[0][0] += ball_vel * dt;
        } else {
            ball_yx[0][0] -= ball_vel * dt;
        }

        if(ballRight == true) {
            ball_yx[0][1] += ball_vel * dt;
        } else {
            ball_yx[0][1] -= ball_vel *dt;
        }
    }

    if(ball_vel < vel_max) {
        ball_vel += (difficultyLevel * 0.1f);
    }
    if(ball_vel_y < vel_max) {
        ball_vel_y += (difficultyLevel * 0.1f);
    }
    if(ball_vel_x < vel_max) {
        ball_vel_x += (difficultyLevel * 0.1f);
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
            ball_yx[0][1] = startMenuOptionCoords[selectedStartOption-1][0];
            ball_yx[0][0] = startMenuOptionCoords[selectedStartOption-1][1];

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedStartOption == 1) {
                if(endlessMode) {
                    current_state = ENDLESS_LOADED_NOT_STARTED;
                } else {
                    current_state = LOADED_NOT_STARTED;
                }
                break;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedStartOption == 2) {
                current_state = VIEWING_SETTINGS;
                break;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedStartOption == 3) {
                current_state = VIEWING_CREDITS;
                currentCredit = 0;
                break;
            }
            break;
        case VIEWING_SETTINGS :
            ball_yx[1][0] = 175;
            ball_yx[2][0] = 160;
            ball_yx[3][0] = 145;
            ball_yx[4][0] = 130;
            ball_yx[5][0] = 115;

            selectedSettingsOption = update_selected_menu_option(selectedSettingsOption, 5);
            ball_yx[0][1] = settingsMenuOptionCoords[selectedSettingsOption-1][0];
            ball_yx[0][0] = settingsMenuOptionCoords[selectedSettingsOption-1][1];

            if((QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) || QuickGame_Button_Pressed(PSP_CTRL_LEFT) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT)) && selectedSettingsOption == 1) {
                allowGlitch = !allowGlitch;
            }

            if(allowGlitch) {
                ball_yx[1][1] = 188;
            } else {
                ball_yx[1][1] = 296;
            }

            if(selectedSettingsOption == 2) {
                if((QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT))) {
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
                ball_yx[2][1] = 258;
            }

            if (difficultyLevel == 2) {
                ball_yx[2][1] = 316;
            }

            if (difficultyLevel == 3) {
                ball_yx[2][1] = 386;
            }

        
            if((QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) || QuickGame_Button_Pressed(PSP_CTRL_LEFT) || QuickGame_Button_Pressed(PSP_CTRL_RIGHT)) && selectedSettingsOption == 3) {
                faceControls = !faceControls;
            }

            if(!faceControls) {
                ball_yx[3][1] = 246;
            } else {
                ball_yx[3][1] = 376;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedSettingsOption == 4) {
                complexPhysics = !complexPhysics;
            }

            if(!complexPhysics) {
                ball_yx[4][1] = 214;
            } else {
                ball_yx[4][1] = 342;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE) && selectedSettingsOption == 5) {
                endlessMode = !endlessMode;
            }

            if(!endlessMode) {
                ball_yx[5][1] = 148;
            } else {
                ball_yx[5][1] = 290;
            }

            if(QuickGame_Button_Pressed(PSP_CTRL_CROSS)) {
                current_state = VIEWING_START;
                selectedSettingsOption = 1;
                ball_yx[1][0] = -20;
                ball_yx[2][0] = -20;
                ball_yx[3][0] = -20;
                ball_yx[4][0] = -20;
                ball_yx[5][0] = -20;
            }
            break;
        case VIEWING_CREDITS :
            ball_yx[0][1] = -20;
            ball_yx[0][0] = -20;
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                currentCredit++;
                if(currentCredit > 4) {
                    current_state = VIEWING_START;
                }
            }
            break;
        case LOADED_NOT_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)){
                scroll_bg = true;
                ball_vel = 100.0f;
                ball_yx[1][1] = 260;
                ball_yx[1][0] = 187;
                ball_yx[2][1] = 280;
                ball_yx[2][0] = 187;
                ball_yx[3][1] = 300;
                ball_yx[3][0] = 187;
                randomize_level_variables();
                randomize_obstacles();
                current_state = STARTED;
            }
            break;
        case STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_START)) {
                current_state = PAUSED;
                break;
            }

            if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], endBall)){
                animate_runComplete();
            } else {
                update_ball(dt);
            }
            if(allowGlitch && QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                    randomize_level_variables();
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
                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], pinkPaddle)) {
                    QuickGame_Audio_Play(ping, 0);
                    if(complexPhysics) {
                        ball_vel_x = -ball_vel_x;
                        ball_vel_y = ball_vel_y - (pinkPaddle_y - ball_yx[0][0]) * 5;
                    } else {
                        ballRight = !ballRight;
                    }
                    collision_delay = 5;
                }
                
                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], wall)) {
                    QuickGame_Audio_Play(pong, 0);
                    direction = QuickGame_Sprite_Intersect_Direction(animBall[0][curr_ball_anim], wall);
                    if(complexPhysics) {
                        if(direction == QG_DIR_LEFT || direction == QG_DIR_RIGHT) {
                            ball_vel_x = -ball_vel_x;
                        } 
                        ball_vel_y = ball_vel_y - (wall_y - ball_yx[0][0]) * 5;
                    } else {
                        if(direction == QG_DIR_LEFT || direction == QG_DIR_RIGHT) {
                            ballRight = !ballRight;
                        } else {
                            ballUp = !ballUp;
                        }

                    }
                    collision_delay = 5;
                }

                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], flipPad)) {
                    QuickGame_Audio_Play(pong, 0);
                    glitched = !glitched;
                    collision_delay = 12;
                }

                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], bluePaddle)) {
                    QuickGame_Audio_Play(ping, 0);
                    if(complexPhysics) {
                        ball_vel_x = -ball_vel_x;
                        ball_vel_y = ball_vel_y - (bluePaddle_y - ball_yx[0][0]) * 5;
                    } else {
                        ballRight = !ballRight;
                    }
                    collision_delay = 5;
                }

            }

            if(QuickGame_Timer_Elapsed(&timer) >= run_length){
                animate_endNode();
            }

            if(QuickGame_Timer_Elapsed(&timer) >= wall_timer && (currentMission - difficultyLevel) > 0){
                animate_wall();
            }

            if(QuickGame_Timer_Elapsed(&timer) >= flipPad_timer && (currentMission - difficultyLevel) > 1){
                animate_flipPad();
            }

            if(ball_yx[0][0] < ball_height) {
                ballUp=true;
                ball_vel_y = -ball_vel_y;
            }

            if(ball_yx[0][0] > screen_height) {
                ballUp=false;
                ball_vel_y = -ball_vel_y;
            }

            if(ball_yx[0][1] < 0 || ball_yx[0][1] > screen_width) {
                QuickGame_Audio_Play(fail, 0);
                checkDeath();
                // ballRight=!ballRight;
            }
        break;
    case PAUSED :
        if (QuickGame_Button_Pressed(PSP_CTRL_START)) {
            current_state = STARTED;
        }
        if (QuickGame_Button_Pressed(PSP_CTRL_CROSS)) {
            current_state = VIEWING_START;
        }
        break;
    case RUN_COMPLETE:
    case MISSION_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            move_to_next_level();
        break;
    case GAME_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            current_state = VIEWING_ENDING;
        break;
    case VIEWING_ENDING:
        ball_yx[0][1] = 500.0f;
        update_ending_scroll(dt);
        if(show_final_screen && QuickGame_Button_Pressed(PSP_CTRL_START)) {
            reset_game_completely();
            break;
        }
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
            show_final_screen = true;
        }
        break;
    case MOSTLY_DEAD:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            reset_game();
        break;
    case ALL_DEAD:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) 
            reset_game_completely();
        break;
    case ENDLESS_LOADED_NOT_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)){
                scroll_bg = true;
                ball_vel = 100.0f;
                ball_yx[1][1] = 260;
                ball_yx[1][0] = 187;
                ball_yx[2][1] = 280;
                ball_yx[2][0] = 187;
                ball_yx[3][1] = 300;
                ball_yx[3][0] = 187;
                randomize_level_variables();
                randomize_obstacles();
                current_state = ENDLESS_STARTED;
            }
            break;
    case ENDLESS_STARTED :
            if(QuickGame_Button_Pressed(PSP_CTRL_START)) {
                current_state = ENDLESS_PAUSED;
                break;
            }

            if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], endBall)){
                animate_runComplete();
            } else {
                update_ball(dt);
            }
            if(allowGlitch && QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
                    randomize_level_variables();
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
                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], pinkPaddle)) {
                    QuickGame_Audio_Play(ping, 0);
                    if(complexPhysics) {
                        ball_vel_x = -ball_vel_x;
                        ball_vel_y = ball_vel_y - (pinkPaddle_y - ball_yx[0][0]) * 5;
                    } else {
                        ballRight = !ballRight;
                    }
                    collision_delay = 5;
                    current_score++;
                }
                
                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], wall)) {
                    QuickGame_Audio_Play(pong, 0);
                    direction = QuickGame_Sprite_Intersect_Direction(animBall[0][curr_ball_anim], wall);
                    if(complexPhysics) {
                        if(direction == QG_DIR_LEFT || direction == QG_DIR_RIGHT) {
                            ball_vel_x = -ball_vel_x;
                        } 
                        ball_vel_y = ball_vel_y - (wall_y - ball_yx[0][0]) * 5;
                    } else {
                        if(direction == QG_DIR_LEFT || direction == QG_DIR_RIGHT) {
                            ballRight = !ballRight;
                        } else {
                            ballUp = !ballUp;
                        }

                    }
                    collision_delay = 5;
                }

                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], flipPad)) {
                    QuickGame_Audio_Play(pong, 0);
                    glitched = !glitched;
                    collision_delay = 12;
                }

                if(QuickGame_Sprite_Intersects(animBall[0][curr_ball_anim], bluePaddle)) {
                    QuickGame_Audio_Play(ping, 0);
                    if(complexPhysics) {
                        ball_vel_x = -ball_vel_x;
                        ball_vel_y = ball_vel_y - (bluePaddle_y - ball_yx[0][0]) * 5;
                    } else {
                        ballRight = !ballRight;
                    }
                    collision_delay = 5;
                    current_score++;
                }

            }

            if(QuickGame_Timer_Elapsed(&timer) >= wall_timer){
                animate_wall();
            }

            if(QuickGame_Timer_Elapsed(&timer) >= flipPad_timer){
                animate_flipPad();
            }

            if(ball_yx[0][0] < ball_height) {
                ballUp=true;
                ball_vel_y = -ball_vel_y;
            }

            if(ball_yx[0][0] > screen_height) {
                ballUp=false;
                ball_vel_y = -ball_vel_y;
            }

            if(wall_y < 0) {
                randomize_wall();
            }

            if( flipPad_y < 0) {
                randomize_flip();
            }

            if(ball_yx[0][1] < 0 || ball_yx[0][1] > screen_width) {
                // ballRight=!ballRight;
                QuickGame_Audio_Play(fail, 0);
                current_state = ENDLESS_COMPLETE;
            }
        break;
    case ENDLESS_PAUSED :
        if (QuickGame_Button_Pressed(PSP_CTRL_START)) {
            current_state = ENDLESS_STARTED;
        }
        if (QuickGame_Button_Pressed(PSP_CTRL_CROSS)) {
            reset_game_completely();
            current_state = VIEWING_START;
        }
        break;
    case ENDLESS_COMPLETE:
        if(QuickGame_Button_Pressed(PSP_CTRL_CIRCLE)) {
            reset_game_completely();
            current_state = VIEWING_START;
        }
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
            for(int i = 0; i < 6; i++){
                QuickGame_Sprite_Draw(animBall[i][curr_ball_anim]);
            }
            break;
        case VIEWING_CREDITS :
            QuickGame_Sprite_Draw(credits[currentCredit]);
            break;
        case VIEWING_START :
            QuickGame_Sprite_Draw(startScreen);
            QuickGame_Sprite_Draw(animBall[0][curr_ball_anim]);
            break;
        case PAUSED:
        case LOADED_NOT_STARTED :
            draw_remaining_attempts();
            QuickGame_Sprite_Draw(mission[currentMission-1]);
            QuickGame_Sprite_Draw(run[currentRun-1]);
        case STARTED :
            QuickGame_Sprite_Draw(endNode);
            QuickGame_Sprite_Draw(endBall);
        case ENDLESS_LOADED_NOT_STARTED:
        case ENDLESS_STARTED:
            QuickGame_Sprite_Draw(wall);
            QuickGame_Sprite_Draw(flipPad);
            QuickGame_Sprite_Draw(pinkPaddle);
            QuickGame_Sprite_Draw(bluePaddle);
            QuickGame_Sprite_Draw(animBall[0][curr_ball_anim]);
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
            break;
        case VIEWING_ENDING:
            if(show_final_screen) {
                QuickGame_Sprite_Draw(finalScreen);
            } else {
                QuickGame_Sprite_Draw(gameCompleteScrollScreen);
            }
            break;
        case ENDLESS_COMPLETE:
            draw_score();
            break;

    }

    QuickGame_Graphics_End_Frame(true);
}

void load_sprites() {
    QGTexInfo bgForeTexInfo = {.filename = "./assets/sprites/bgFore.png", .flip = true, .vram = 0 };
    bgFore = QuickGame_Sprite_Create_Contained(240, 192, 512, 512, bgForeTexInfo);

    QGTexInfo bgTexInfo = {.filename = "./assets/sprites/bgBack2X.png", .flip = true, .vram = 0 };
    bg = QuickGame_Sprite_Create_Contained(240, 192, 512, 512, bgTexInfo);

    QGTexInfo pink = { .filename = "./assets/sprites/pink.png", .flip = true, .vram = 0 };
    pinkPaddle = QuickGame_Sprite_Create_Contained(left_paddle_lane_x, 120, 8, paddle_height, pink);

    QGTexInfo blue = { .filename = "./assets/sprites/blue.png", .flip = true, .vram = 0 };
    bluePaddle = QuickGame_Sprite_Create_Contained(right_paddle_lane_x, 120, 8, paddle_height, blue);

    QGTexInfo wallTex = { .filename = "./assets/sprites/wall.png", .flip = false, .vram = 0 };
    wall = QuickGame_Sprite_Create_Contained(wall_x, wall_y, 8, 40, wallTex);

    QGTexInfo flipPadTex = { .filename = "./assets/sprites/flipPad.png", .flip = false, .vram = 0 };
    flipPad = QuickGame_Sprite_Create_Contained(flipPad_x, flipPad_y, 40, 40, flipPadTex);

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
    gameCompleteScrollScreen = QuickGame_Sprite_Create_Contained(240, -512, 512, 512, gameCompleteScrollTex);

    QGTexInfo finalScreenTex = { .filename = "./assets/sprites/gameCompleteFinal.png", .flip = true, .vram = 0 };
    finalScreen = QuickGame_Sprite_Create_Contained(274, 136, 360, 40, finalScreenTex);

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

    for(int j = 0; j < 7; j++){
        char filename[256];
        sprintf(filename, "./assets/sprites/ball/%d.png", j);

        QGTexInfo ballTex = { .filename = filename, .flip = true, .vram = 0 };
        for(int i = 0; i < 6; i++){
            animBall[i][j] = QuickGame_Sprite_Create_Contained(160, 136, 14, 14, ballTex);
        }
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

    for(int i = 0; i < 10; i++){
        char filename[256];
        sprintf(filename, "./assets/sprites/nums/%d.png", i);

        QGTexInfo sc = { .filename = filename, .flip = true, .vram = 0 };
        nums[i] = QuickGame_Sprite_Create_Contained(240, 136, 32, 64, sc);
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