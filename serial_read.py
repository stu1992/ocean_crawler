import serial
import time
import requests
import time
import json

url = "http://localhost:5167/addHistory"
# 1. Configure the serial port and baud rate
# 'timeout=1' prevents the script from hanging forever if no data arrives
PORT = 'COM3' 
BAUD_RATE = 9600
try:
    ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
    print(f"Connected to Arduino on {PORT}")
    
    # 2. Wait for Arduino to reset
    # Opening the serial port automatically resets most Arduinos.
    # We must wait 2 seconds for the bootloader to finish.
    time.sleep(2) 
    
    # 3. Read incoming data continuously
    while True:
        if ser.in_waiting > 0: # Check if bytes are waiting in the buffer
            # Read a line of data until a newline character (\n) is found
            raw_data = ser.readline() 
            
            # Decode binary bytes to a readable UTF-8 string and strip whitespace
            decoded_string = raw_data.decode('utf-8').strip()
            
            print(decoded_string)
            if decoded_string[:1] == "[":
                print("found a reading")
                print(decoded_string)
                #payload = [{"timestamp": 1700, "longitude": 0.0, "latitude" : 0.0}]
                response = requests.post(url, json=json.loads(decoded_string))
                print(response.status_code)


            
except serial.SerialException as e:
    print(f"Error connecting to serial port: {e}")
except KeyboardInterrupt:
    print("\nProgram stopped by user.")
finally:
    # 4. Safely close the port when exiting
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed.")