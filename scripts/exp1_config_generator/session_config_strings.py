config_start_string = '''
{
'''

config_end_string = '''
}
'''

global_session_config_string = '''
  // Information related to this file
  "settingsVersion": 1, // Used for parsing this file

  // General settings for the experiment
  "appendingDescription": "default", // Description of this file
  //"sceneName": "FPSci Square Map (1m gap)",
  //"sceneName": "FPSci Square Map",
  //"sceneName": "Shooting Range Textures",
  "feedbackDuration": 0.0, // Time allocated for providing user feedback
  "readyDuration": 0.25, // Time allocated for preparing for trial5:045:04 PM 12/6/20195:04 PM 12/6/20195:04 PM 12/6/20195:04 PM 12/6/20195:04 PM 12/6/20195:04 PM 12/6/2019 PM 12/6/2019
  "taskDuration": 20.0, // Maximum duration allowed for completion of the task
  "horizontalFieldOfView": 103.0, // Field of view (horizontal) for the user
   clickPhotonColors = (
     Color3(0.2, 0.2, 0.2 ),
     Color3(0.8, 0.8, 0.8 ) );
   clickPhotonSide = "right";
   clickPhotonSize = Vector2(0.1, 0.1);
   clickPhotonVertPos = 0.5;
   clickPhotonMode="system";
   renderClickPhoton = true; 
  "horizontalFieldOfView":  103.0,            // Field of view (horizontal) for the user in degrees
  "moveRate": 4.0,                            // Player move rate (0 for no motion)
  "mouseEnabled" : false,
  "playerAxisLock" = {false, true, true},     // Player can only move horizontally
  "jumpVelocity": 0.0,                       // Player can't jump
  "jumpInterval": 0.0,                        // Minimum jump interval
  "jumpTouch": true,                          // Require touch for jump
  "playerHeight":  1.5,                       // Normal player height
  "crouchHeight": 0.8,                        // Crouch height
  //"playerGravity": Vector3(0.0, -5.0, 0.0),   // Player gravity

  "showHUD":  true,               // Show the player HUD (banner, ammo, health bar)
  "showBanner":  true,            // Control the banner at the top of the screen (shows time, score, and session % complete)
  "bannerLargeFontSize": 30.0,    // Large font size to use in the banner (% complete)
  "bannerSmallFontSize": 14.0,    // Small font size to use in the banner (time remaining and score)
  "hudFont": "console.fnt",       // Font to use for the HUD (fixed with highly suggested!)

  // Basic weapon configuration
  "weapon": {
    "maxAmmo": 100000, // Maximum number of clicks to allow per trial
    "firePeriod": 0, // Minimum period between shots (in seconds) to allow during the experiment
    "damagePerSecond": 1,
    "autoFire": true, // Whether the weapon fires automatically when the left mouse is held down
  },

'''

sessions_config_start_string = '''
  "sessions": ['''

sessions_config_end_string = '''
  ],'''

session_config_start_string = '''
    {'''

session_config_end_string = '''
    },'''

targets_config_string = '''
  "targets": [
    {
      "id": "stray",
      "speed": [ 3.0, 5.0 ],
      "visualSize": [ 0.75, 0.75 ],
      "destSpace" : "world",
      "spawnBounds": AABox {
        Point3(-0.0, 2.0, -15.0),
        Point3(0.0, 2.0, -15.0),
      },
      "moveBounds": AABox {
        Point3(-5.0, 0.3, -15.0),
        Point3(5.0, 4.0, -15.0),
      },
      "motionChangePeriod": [ 0.8, 1.5 ],
      "jumpEnabled": false,
      "respawnCount": 0,
      "axisLocked": [false,false,true],
    },
    #include("Cw.Any"),
    #include("CC.Any"),
  ],
'''