import sys
import time
import json
import random
import signal
import socket

MAX_NO_OF_NODES = 5

# Virtual Socket Device
class VirtualSocketDevice:
    def __init__(self, address='localhost', port=12345):
        self.address = address
        self.port = port
        self.sock = None
        self.client_connection = None

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def open(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((self.address, self.port))
        self.sock.listen()
        print(f"Socket listening on {self.address}:{self.port}")
        self.client_connection, addr = self.sock.accept()
        print('Connected by', addr)

    def close(self):
        if self.client_connection:
            self.client_connection.close()
        if self.sock:
            self.sock.close()
        self.sock = None
        self.client_connection = None

    def write_packet(self, packet):
        try:
            self.client_connection.sendall(packet.encode())
        except (BrokenPipeError, ConnectionResetError) as e:
            print("Broken Pipe, attempting reconnect.")
            self.close()
            self.open()

# Rest of the code remains the same...

class DataGenerator:

    def __init__(self, max_nodes = MAX_NO_OF_NODES):
        self.max_nodes = max_nodes
        self.node_addresses = [(random.randint(0, 255), random.randint(0, 255)) for i in range(max_nodes)]

    @staticmethod
    def generate_random_path(max_length):
        length = random.randint(2,max_length)
        digits = list(range(1,length))
        random.shuffle(digits)
        digits.append(0)

        return digits


    # Generate a random sensor message
    # Generate a random sensor message
    def generate_sensor_message(self):
        force = random.randint(0, 2000)
        oximeter = random.randint(40, 170)
        battery_level = random.randint(3000, 3700)
        path = int(''.join(map(str, self.generate_random_path(max_length=5))))

        sensor_message = {
            "Force": force,
            "Oximeter": oximeter,
            "Path": path,
            "Battery": battery_level,
        }
        return sensor_message

    # Generate a random routing table
    def generate_routing_table(self):
        routing_table = {}

        random.shuffle(self.node_addresses)

        for i in range(self.max_nodes):
            node_address = ".".join(map(str, self.node_addresses[i]))
            hops = random.randint(0, self.max_nodes+2)
            next_hop = ".".join(map(str, random.choice(self.node_addresses)))
            node_id = random.randint(1, 10)

            if i == 0:
                node_type = "G"
                hops = 0
            else:
                node_type = random.choice(["S", "N"])

            still_active = bool(random.getrandbits(1))

            routing_table[f"Entry {i+1}"] = {
                "node_address": node_address,
                "hops": str(hops),
                "next_hop": next_hop,
                "node_id": str(node_id),
                "node_type": node_type,
                "still_active": str(still_active).lower()
            }

        return routing_table
# Main function
def main():
    # Configuration
    # Register the SIGINT handler
    def sigint_handler(signal, frame):
        print("SIGINT received. Closing the virtual serial device...")
        try:
            if virtual_device:
                virtual_device.close()
        except Exception as e:
            print(f"Exception occured while handling SIGINT: {e}")
        sys.exit(0)

    signal.signal(signal.SIGINT, sigint_handler)
    d = DataGenerator(max_nodes=MAX_NO_OF_NODES)

    with VirtualSocketDevice(port=8000) as virtual_device:
        # Generate and send packets
        while True:
            sensor_message = d.generate_sensor_message()
            routing_table = d.generate_routing_table()

            sensor_packet = json.dumps(sensor_message) + "\r\n"
            routing_table_packet = json.dumps(routing_table) + "\r\n"

            print(f"Writing Packet {sensor_packet}, {len(sensor_message)=}")
            virtual_device.write_packet(sensor_packet)
            print(f"Packet {sensor_packet} written")
            time.sleep(1)
            
            print(f"Writing Packet {routing_table_packet}, {len(routing_table_packet)=}")
            virtual_device.write_packet(routing_table_packet)
            print(f"Packet {routing_table_packet} written")
            time.sleep(1)

            

if __name__ == "__main__":
    main()
