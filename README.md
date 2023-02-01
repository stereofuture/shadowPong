Single player version of Pong with a cyberpunk aesthetic. Created for the RE:Start! 2022 PSP Game Jam

Gameplay:
- Meant to be played like pong. Player should use the paddles to keep the ball in play long enough to reach the exist node.
- Player has three extra balls to try and finish each run.
- Obstacles
-- Walls - Works like a paddle, reversing the direction of the ball as it comes in contact.
-- Flip Pads - Flip the paddle controls. Controls will stay flipped until the pad is hit again or the ball is lost.

Settings:
- Glitch
-- Enabled - Theme for the jam was "Glitched" so I left in a glitch where hitting CIRCLE will trigger the pre-run randomization. This will randomly move the ball, the ball's current direction, the locations of the obstacles and end goal, as well as resetting the velocity.
-- Disabled - CIRCLE does nothing during gameplay.

- Difficulty
-- 1,2,3 - The higher the number, the longer that runs are before the goal node appears, the faster the maximum ball velocity and the faster the ball reaches the maximum.

- Controls
-- Regular - Controls are to hold L or R and use UP and DOWN to move the respective paddle. When controls are flipped, the L controls the right paddle and the R controls the left. These were the original controls for the Jam.
-- Alternate - Controls are to use UP and DOWN along with CROSS and TRIANGLE to move the respective side. When controls are flipped, the D-Pad will control the right paddle, while the face buttons will control the left paddle.

- Physics
-- Regular - Original, simpler physics implementation from the Jam. Where on a paddle or wall the ball collides has no bearing on the ball's trajectory.
-- Alternate - Physics are more in line with other paddle games where the position where the ball collides with the paddle will change the angle at which it bounces.

- Mode
-- Story - Single player pong gameplay, but with story elements. Mode is broken down into three separate missions with each mission requiring the completion of five runs with the allotted number of attempts. Runs get progressively harder in each mission with the introduction of obstacles. Within a mission, runs get progressively longer.
-- Endless - Highscore mode. Obstacles loop through and ball velocity increases as run goes on.

Credits
- Several assets in the game were provided for free by other creatives. View the credits to see links to their work.

Bugs
- Game was my first attempt at a GameJam, my first time coding in C and my first time using the QuickGame API. It is at a good stopping point, but feel free to file bugs or make suggestions as I would like to revisit the code at some point after I have completed a few more projects.

Note for Artists
- The art was done by hand and on the Jam's deadline so I recognize it isn't very good. If you would like to do a re-skin please contact me. I am willing to pay for good art, particularly if we can collaborate on future projects.
- Audio is what I could find for free in an hour or so of searching. I love the tracks I found and think the sound effects fit pretty well. However, similar to the art, if you are an audio creative, I would be happy to pay a small amount to get a proper audio treatment.