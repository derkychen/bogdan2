import serial
import time
from pathlib import Path

SERIAL_PORT = '/dev/ttyUSB0'                # Update this to your serial port
BAUD_RATE = 115200                          # Arbitrary baud rate
JSON_FILE = "instructions/example.json"     # Path to your JSON file

def send_json_file(serial_port: str, json_file:str) -> None:
    path = Path(json_file)

    with path.open("r", encoding="uf-8") as file:
        json_text = file.read()
    if not json_text.endswith("\n"):
        json_text += "\n"
    
    with serial.Serial(serial_port, BAUD_RATE, timeout=2) as ser:
        time.sleep(2)

        ser.write(json_text.encode('utf-8'))
        ser.flush()

    print("Sent JSON:")
    print(json_text)

if __name__ == "__main__":
    send_json_file(SERIAL_PORT, JSON_FILE)