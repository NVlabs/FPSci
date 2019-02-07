# abstract-fps

## Dependency setting for Psychophysics library

Set up an environment variable as the following (note that we are not distributing it yet!)
- name: PSYCHOPHYSICS
- value: the root folder of psychophysics-lib
 
## TODO (bold face for more urgent)
Progress tracking sheet (only accessible to developers): https://docs.google.com/spreadsheets/d/1rxkRC-GVi-nCIIElz8XpO-7lhCwv9De2x2wF_ZM9Vnk/edit?usp=sharing
- **Avoid re-compiling whenever experimental condition changes. Possibly we can exploit the drag-drop feature of G3D, using an input file.**
    - ~~**Can we change frame rates without re-executing the file?**~~ Yes
- **Implementation**
    - **Reaction experiment**
        - **On-screen feedback message, success or failure of a trial + performance (latency) if success**
    - **Targeting experiment**
        - **Disable view reset. Ready state ends when user clicks on the center target**
        - **View change seems to be quantized**
- **Characterize various frame rates in terms of click-photon latency values** Joohwan
- **Pilot run both experiments, analyze, make sure data make sense.** Joohwan
- **Integrate Morgan's latest change in abstract-fps2** Joohwan
- **Support for input device configuration**
    - **Start with mouse. Options.**
        - **1. We enter the 360 distance in a text box.**
        - **2. User enter into mouse adjusting mode, and adjust mouse sensitivity using hot keys or UI.**
        - **3. Include it in the settings file, and becomes effective as the file drop-downed.**
        - **SUGGESTED [Ben]: Import mouse settings from a local file/runtime argument instead of having the user enter them into code/a text box.**
        - **Many games have anisotropic sensitivity for horizontal and vertical mouse motion. Add this.**
- SUGGESTED [Ben]: Add an (optional?) visual indicator for the target if it ever goes off-screen (don't think this should happen, but would be good to include)
- Go through current code structure and re-org it if necessary.
- Independent control of latency for input / presentation (currently we only have presentation): Morgan implemented it in G3D
    - First check it whether it is indeed necessary.
- Improve projectile animation (we first need to discuss 'how')
- Time to add latency measuring tools!
- Full record of target motion
    - Analyze motion characteristics and subjects performance, infer difficulty level from the data (Ben)
        - Come up w/ some basic metrics that summarize path smoothness (i.e. integrated jerk magnitude)
        - Look more closely at the motion of the target object just before/during the mouse down event

## Instructions for running the experiment

You are trying to hit the target as quickly as possible. There is no penalty for missing a shot.

- "hitscan" is one shot kills.
- "tracking" is click and hold to hit the target repeatedly until dead.
- "projectile" includes some prediction for projectile travel time.

1. See the following chart for the ordering of experimental conditions.
    - H for hitscan, T for tracking, P for projectile.
    - Experiment 1: Effect of refresh rate
        - Joohwan, Rachel (Rachel ran the last two conditions in the flipped order. See db file if not sure what it means.)
            - H240 H120 H60 T60 T120 T240
        - Josef
            - H60 H240 H120 T120 T240 T60
        - Zander
            - T60 T240 T120 H120 H240 H60
        - Ben
            - T240 T120 T60 H60 H120 H240
    - Experiment 2: Effect of latency alone
        - Keep refresh rate to 240hz, only change numFrameDelay, weaponType, and expVersion (training or real)
        - Joohwan
            - T2, T1, T0, H0, H1, H2
        - Zander, Josef
            - T1, T0, T2, H2, H0, H1
        - Rachel
            - T0, T2, T1, H1, H2, H0
2. For each condition, you have to run two sessions: training and real
3. For each run (e.g. In Experiment 1, 2 runs per 8 conditions makes 16 runs), you have to launch the app with the corresponding modifications on parameters in line 33~40 in App.cpp.
    - Note that you have to be exact on weaponType name! (either 'hitscan' or 'tracking')
    - Use datafile name 'ver1.db' for Experiment 1 and 'ver2.db' for Experiment 2.
4. When you see a message saying 'DONE' on the screen, exit it by pressing an esc key.
5. Repeat the above until you complete all the conditions.
6. Add your name to your db file and upload to NXP google drive under the following folder: Esports / Abstract FPS Experiment / data_VSS2019_abstract
