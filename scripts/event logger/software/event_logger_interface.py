import serial
import numpy as np
from datetime import datetime
import time

validEventTypes = ['M1', 'M2', 'PD', 'SW']      # Valid event type strings

ADC_BIT_WIDTH = 10                  # ADC bit width (used for value checking)
MAX_ADC_VALUE = 2**ADC_BIT_WIDTH    # ADC max value (dervied from above) used for value checking

# Command set
ADC_ON = 'aon'              # ADC values 'on' or reported via USB
ADC_OFF = 'aoff'            # ADC values 'off' or not reported via USB
AUTOCLICK_ON = 'con'        # Autoclicking on, or occurring (currently just 1 click)
AUTOCLICK_OFF = 'coff'      # Autoclicking off, or not occurring
INFO = 'i'                  # Get device info (FW revision)

# Simple class for handling serial sync using DTR/CTS
class SerialSynchronizer:
    def __init__(self, comPort):
        self.com = serial.Serial(comPort)

    def sync(self):
        self.com.setDTR(1)
        self.com.setDTR(0)      # Toggle DTR to sync the logger
        return datetime.now()   # Return the time       
    

# Class for handling logging from the event logger
class EventLoggerInterface:

    class EmulationParams:
        def __init__(self, events, probability_dict):
            self.events = events
            self.prob = probability_dict

    def __init__(self, comPort, baudRate=115200, timeoutS=0.1, emulate=False, emuParams=None):
        self.emulate = emulate
        if emulate:
            self.emuParams = emuParams
            self.emuStart = datetime.now()
            self.emuQueue = []
        else:
            self.com = serial.Serial(comPort, baudRate, timeout=timeoutS)
            self.buffer = ""

    def flush(self):
        # Flush incoming data
        if not self.emulate: self.com.flushInput()

    def emulateLine(self):
        if self.emuParams is None: raise Exception("Need emulation parameters!")
        options = {}
        for event in self.emuParams.events:
            rval = np.random.random()
            if rval > (1-self.emuParams.prob[event]):
                options[event] = rval
        if len(options) == 0: return None
        else:
            e = max(options, key=self.emuParams.prob.get)
            return self.emulate_event(e)

    def emulate_event(self, event):
        timestamp_s = (datetime.now() - self.emuStart).total_seconds()
        return [timestamp_s, event]

    def parseString(self, string):
        if not ":" in string: return None
        # Get the timestamp (in s) and event type
        try:
            timestamp_s = float(string.split(':')[0])/1000000.0
            event_type = string.split(':')[1]
        except: return None
        event_type = event_type.strip()
        # Check if we have a known event (otherwise this is an ADC sample)
        if event_type not in validEventTypes:
            try: 
                aValue = int(event_type)
                return [timestamp_s, aValue]            # Return an ADC sample value
            except: return None
        else: return [timestamp_s, event_type]          # Return a known event type
    
    def parseLine(self):
        # Read a line and parse it
        if self.emulate: return self.emulateLine()
        else: line = self.com.readline().decode('utf-8').strip()
        return self.parseString(line)

    def parseLines(self):
        if self.emulate: 
            if len(self.emuQueue) > 0: return self.emuQueue     # If we have clicks to report, do that first
            else: return [self.emulateLine()]    # Otherwise just do a single line when in emulation mode (we can do this as fast as we want)
        else:
            output = []
            self.buffer += self.com.read(self.com.inWaiting()).decode('utf-8')      # Keep remnant characters in a buffer here                
            if '\n' in self.buffer:
                # Split lines and iterate through them to process for events 
                lines = self.buffer.split('\n')
                for line in lines:
                    evt = self.parseString(line)
                    if evt is not None: output.append(evt)
            self.buffer = self.buffer.split('\n')[-1]
            return output

    # Write data to the COM port
    def write(self, data):
        if not self.emulate: self.com.write(data)
        else: pass

    def send_cmd(self, cmd):
        self.write((cmd + '\n').encode('utf-8'))

    # Control ADC reporting remotly (reduces USB traffic)
    def set_adc_report(self, report):
        if report: self.send_cmd(ADC_ON)
        else: self.send_cmd(ADC_OFF)

    def adc_report_on(self):
        self.set_adc_report(True)

    def adc_report_off(self):
        self.set_adc_report(False)

    # Control auto click feature
    def set_mouse_down(self, active):
        if active: self.send_cmd(AUTOCLICK_ON)
        else: self.send_cmd(AUTOCLICK_OFF)
        self.com.flushOutput()
        
    def mouseDown(self):
        self.set_mouse_down(True)
        if self.emulate: self.emuQueue.append(self.emulate_event('M1'))

    def mouseUp(self):
        self.set_mouse_down(False)

    def click(self, duration_ms=100):
        if duration_ms > 100: raise Exception("Cannot produce click duration >100ms!")
        self.mouseDown()
        time.sleep(duration_ms/1000.0)
        self.mouseUp()

    # Simple method to get version from logger
    def get_fw_version(self):
        if self.emulate: return 'Hardware Event Logger Emulator'
        else:
            self.buffer += self.com.read(self.com.inWaiting()).decode('utf-8')      # Read any characters waiting in the buffer
            self.send_cmd(INFO)                                                     # Request device info
            time.sleep(0.05)
            lines = self.com.read(self.com.inWaiting()).decode('utf-8')
            for line in lines.split('\n'):
                line = line.strip()
                if 'Hardware Event Logger' in line: return line                     # If we find the version report it here
                else: self.buffer += line
            return None

    def get_analog_values(self, time_window_s=1, flush=True):
        if self.emulate: return [[0],[0]]     # Analog not supported in emulation for now!!!
        else:
            starttime = datetime.now()
            data = []
            times = []
            if flush: self.com.flushInput()
            while((datetime.now()-starttime).total_seconds() < time_window_s):
                result = self.parseLine()
                if result is None: continue
                [time,value] = result
                if(value in validEventTypes): continue
                if value > MAX_ADC_VALUE: continue
                times.append(time)
                data.append(value)
            return [times,data]
    
    def get_average_analog_value(self, time_window_s=1):
        [times,data] = self.get_analog_values()
        return np.mean(data)

            


