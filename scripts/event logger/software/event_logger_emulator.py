import numpy as np
rand = np.random.random

class LoggerEmulator:
    def __init__(self, event_prob):
        self.p = event_prob
        self.lookup = ["M1", "M2", "PD", "SW"]
        self.time = 0

    # Dummy flush function
    def flush(self): 
        return

    # Produce a dummy string (or don't)
    def readline(self):
        # Randomly produce events for now
        if rand() < self.p:
            self.time += int(1e6*rand())
            event = self.lookup[int(4*rand())]
            return "{0}:{1}".format(self.time, event).encode('utf-8')
        return "".encode('utf-8')

    # Dummy write function for now
    def write(self, s):
        if(s == "on\n"): print("Autoclick on")
        elif(s == "off\n"): print("Autoclick off")



