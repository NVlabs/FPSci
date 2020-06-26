config_start_string = '''
{
'''

config_end_string = '''
}
'''

global_user_config_string = '''
    currentUser = "JK"; 
    settingsVersion = 1; 
'''

users_config_start_string = '''
    users = ('''

users_config_end_string = '''
    );'''

user_config_start_string = '''
        {'''

user_config_end_string = '''
        },'''

user_common_config_string = '''
            mouseDPI = 3200; 
            reticleColor = ( 
                Color4(1, 0, 0, 1 ), 
                Color4(1, 0, 0, 1 ) ); 
            
            reticleIndex = 39; 
            reticleScale = ( 0.75, 0.75 ); 
            reticleShrinkTime = 0.3; 
'''

user_specific_config_strings = {
  'DM':'''
            cmp360 = 28; 
            turnScale = Vector2(1, 1 );''',
  'EP':'''
            cmp360 = 17; 
            turnScale = Vector2(1, 1 );''',
  'CM':'''
            cmp360 = 17; 
            turnScale = Vector2(1, 1 );''',
  'LN':'''
            cmp360 = 20; 
            turnScale = Vector2(1, 1 );''',
  'MC':'''
            cmp360 = 20; 
            turnScale = Vector2(1, -1 );''',
  'JC':'''
            cmp360 = 22; 
            turnScale = Vector2(1, 1 );''',
  'MiB':'''
            cmp360 = 13; 
            turnScale = Vector2(1, 1 );''',
  'JA':'''
            cmp360 = 18; 
            turnScale = Vector2(1, 1 );''',
  'AE':'''
            cmp360 = 14.4; 
            turnScale = Vector2(1, 1 );''',
  'MaB':'''
            cmp360 = 18; 
            turnScale = Vector2(1, 1 );''',
  'SS':'''
            cmp360 = 30; 
            turnScale = Vector2(1, 1 );''',
  'JK':'''
            cmp360 = 30; 
            turnScale = Vector2(1, 1 );''',
}
