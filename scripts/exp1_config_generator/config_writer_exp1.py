import session_config_strings, user_config_strings, user_status_strings
import itertools
import numpy as np
import shutil

class SessionConfigWriter:
  def __init__(self, filename):
    self.file = open(filename, 'w')
    self.file.write(session_config_strings.config_start_string)

  def writeGlobalConfig(self):
    self.file.write(session_config_strings.global_session_config_string)

  def writeTargetConfig(self):
    self.file.write(session_config_strings.targets_config_string)

  def writeSessionConfig(self,sessionCodes):
    self.file.write(session_config_strings.sessions_config_start_string)
    
    for sessionCode in sessionCodes:
      self.file.write(session_config_strings.session_config_start_string)

      # Config common to all sessions
      tmp_str = '''
      "frameRate": 60, // Session frame rate (in frames per second)
      "id": "'''

      tmp_str += sessionCode
      tmp_str += '",'
      
      # Warp settings
      if "C1" in sessionCode: # Condition 1: no warp & low latency
        tmp_str += '''
      "warpMethod": "none",
      "frameDelay": 0,'''
      elif "C2" in sessionCode: # Condition 2: no warp & high latency
        tmp_str += '''
      "warpMethod": "none",
      "frameDelay": 5,'''
      elif "C3" in sessionCode: # Condition 3: rotation-only, image-warp
        tmp_str += '''
      "warpMethod": "RIW",
      "frameDelay": 5,'''
      elif "C4" in sessionCode: # Condition 4: rotation-only, camera-warp
        tmp_str += '''
      "warpMethod": "RCW",
      "frameDelay": 5,'''
      elif "C5" in sessionCode: # Condition 5: rotation-and-translation, camera-warp
        tmp_str += '''
      "warpMethod": "FCW",
      "frameDelay": 5,'''

      # Wall settings
      if "10m" in sessionCode: # Wall separation 10 m
        tmp_str += '''
      "sceneName": "FPSci Square Map (10m gap)",'''
      elif "1m" in sessionCode: # Wall separation 1 m
        tmp_str += '''
      "sceneName": "FPSci Square Map (1m gap)",'''
      elif "50cm" in sessionCode: # Wall separation 50 cm
        tmp_str += '''
      "sceneName": "FPSci Square Map (50cm gap)",'''

      # Weapon settings
      if "H" in sessionCode: # Hit task
        tmp_str += '''
      "cooldownThickness": 1,
      "cooldownColor": Color4(1.0,1.0,1.0,0.75),
      "weapon": #include("weapon/latewarp_exp1_H.Any");'''
    
        # Training or real
        if "T" in sessionCode: # Training session
          tmp_str += '''
      "expMode": "training", // Experiment mode ("real" or "training")
      "clickPhotonMode": "minimum", // "minimum" or "total"
      "trials": [
        {"ids": ["stray", ],"count": 15},
        {"ids": ["Cw", ],"count": 5},
        {"ids": ["CC", ],"count": 5},
      ]'''
    
        elif "R" in sessionCode: # Real session
          tmp_str += '''
      "expMode": "real", // Experiment mode ("real" or "training")
      "clickPhotonMode": "total", // "minimum" or "total"
      "trials": [
        {"ids": ["stray", ],"count": 15},
        {"ids": ["Cw", ],"count": 5},
        {"ids": ["CC", ],"count": 5},
      ]'''

      elif "L" in sessionCode: # Laser task
        tmp_str += '''
      "renderWeaponStatus": false,
      "weapon": #include("weapon/latewarp_exp1_L.Any");'''
        # Training or real
        if "T" in sessionCode: # Training session
          tmp_str += '''
      "expMode": "training", // Experiment mode ("real" or "training")
      "clickPhotonMode": "minimum", // "minimum" or "total"
      "trials": [
        {"ids": ["stray", ],"count": 15},
        {"ids": ["Cw", ],"count": 5},
        {"ids": ["CC", ],"count": 5},
      ]'''
    
        elif "R" in sessionCode: # Real session
          tmp_str += '''
      "expMode": "real", // Experiment mode ("real" or "training")
      "clickPhotonMode": "total", // "minimum" or "total"
      "trials": [
        {"ids": ["stray", ],"count": 15},
        {"ids": ["Cw", ],"count": 5},
        {"ids": ["CC", ],"count": 5},
      ]'''

      self.file.write(tmp_str)

      self.file.write(session_config_strings.session_config_end_string)
    self.file.write(session_config_strings.sessions_config_end_string)

  def close(self):
    self.file.write(session_config_strings.config_end_string)
    self.file.close()

class UserConfigWriter:
  def __init__(self, filename):
    self.file = open(filename, 'w')
    self.file.write(user_config_strings.config_start_string)

  def writeGlobalConfig(self):
    self.file.write(user_config_strings.global_user_config_string)

  def writeUserConfig(self,subjects):
    self.file.write(user_config_strings.users_config_start_string)
    
    for subject in subjects:
      self.file.write(user_config_strings.user_config_start_string)
      tmp_str = '''
            id = "'''
      tmp_str += subject
      tmp_str += '";'

      tmp_str += user_config_strings.user_specific_config_strings[subject]
      self.file.write(tmp_str)

      self.file.write(user_config_strings.user_common_config_string)
      self.file.write(user_config_strings.user_config_end_string)

    self.file.write(user_config_strings.users_config_end_string)

  def close(self):
    self.file.write(user_config_strings.config_end_string)
    self.file.close()

class UserStatusWriter:
  def __init__(self, filename):
    self.file = open(filename, 'w')
    self.file.write(user_status_strings.config_start_string)

  def writeGlobalConfig(self):
    self.file.write(user_status_strings.global_user_status_string)

  def writeUserStatus(self,subjects,sessions_dict):
    self.file.write(user_status_strings.users_status_start_string)

    for subject in subjects:
      self.file.write(user_status_strings.user_status_start_string)
      self.file.write(user_status_strings.user_common_status_string)

      tmp_str = '''
            id = "'''
      tmp_str += subject
      tmp_str += '";'

      tmp_str += '''
            sessions = (
'''
      for s in sessions_dict[subject]:
        tmp_str += '"' + s + '",'
      tmp_str += '''
            );'''

      self.file.write(tmp_str)
      self.file.write(user_status_strings.user_status_end_string)

    self.file.write(user_status_strings.users_status_end_string)

  def close(self):
    self.file.write(user_status_strings.config_end_string)
    self.file.close()


def main():
  # Write experiment config file.
  conditions = ['C1','C2','C3','C4','C5']
  trainings = ['T','R']
  weapons = ['L','H']
  walls = ['10m','1m','50cm']

  sessionCodes = [
    c + wa + we + t
    for (c, wa, we, t)
    in itertools.product(conditions, weapons, trainings, walls)
  ]

  s_filename = 'experimentConfig.Any'
  sw = SessionConfigWriter(s_filename)
  sw.writeGlobalConfig()
  sw.writeSessionConfig(sessionCodes)
  sw.writeTargetConfig()
  sw.close()

  # define subjects.
  subjects = ['DM','EP','CM','LN','MC','JC','MiB','JA','AE','MaB', 'SS', 'JK'] # the last subject is experimenter

  # Write user config file.
  uc_filename = 'userConfig.Any'
  ucw = UserConfigWriter(uc_filename)
  ucw.writeGlobalConfig()
  ucw.writeUserConfig(subjects)
  ucw.close()

  # Write user status file.
  user_specific_condition_orderings = {
    'DM':['C5','C4','C2','C1','C3',],
    'EP':['C3','C5','C4','C2','C1',],
    'CM':['C1','C2','C3','C4','C5',],
    'LN':['C2','C3','C1','C5','C4',],
    'MC':['C4','C1','C5','C3','C2',],
    'JC':['C3','C4','C2','C5','C1',],
    'MiB':['C5','C1','C3','C4','C2',],
    'JA':['C2','C3','C5','C1','C4',],
    'AE':['C4','C2','C1','C3','C5',],
    'MaB':['C1','C5','C4','C2','C3',],
    'SS':['C1','C2','C3','C4','C5',],
    'JK':['C1','C2','C3','C4','C5',],
  }

  sessions_dict = {}
  subConditionStrings = [
    wa + we + t
    for (wa, we, t)
    in itertools.product(weapons, trainings, walls)
  ]
  us_filename = 'userStatus.Any'
  usw = UserStatusWriter(us_filename)
  usw.writeGlobalConfig()
  for subject in subjects:
    session_ordering = [
      c + s
      for (c, s)
      in itertools.product(user_specific_condition_orderings[subject], subConditionStrings)
    ]
    sessions_dict[subject] = session_ordering

  usw.writeUserStatus(subjects, sessions_dict)
  usw.close()

  for src_filename in [s_filename, uc_filename, us_filename]:
    src = src_filename
    dst = "../../data-files/" + src_filename
    shutil.copyfile(src, dst)

if __name__ == "__main__":
  # execute only if run as a script
  main()