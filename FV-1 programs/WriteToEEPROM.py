import serial
import time


file_path = "name_of_your_programs_file.bin"
PORT = 'COM3'  #change port
BAUD = 115200

try:
    with open(file_path, "rb") as f:
        payload = f.read()

    print(f"loaded: {file_path}")
    print(f"size: {len(payload)} b")
    print("-" * 50)

    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)

    start_time = time.time()

    for i, b in enumerate(payload):
        ser.write(bytes([b]))

        response = ser.read(1)

        if response == b'K':    #sign to send another byte
            print(f"{b:02X}", end=" ")
            if (i + 1) % 16 == 0:
                print(f" | Adr: {i - 15}-{i}")
        elif response == b'E':
            print(f"\n[Error] I2C Error at byte {i} (0x{b:02X})!")
            break
        else:
            print(f"\n[TIMEOUT] Arduino silent at byte {i} (0x{b:02X}).")
            break

    print("\n" + "-" * 50)
    print(f"Done. time: {round(time.time() - start_time, 2)}s")
    ser.close()

except FileNotFoundError:
    print(f"Error: {file_path} not found!")
except Exception as e:
    print(f"Error: {e}")