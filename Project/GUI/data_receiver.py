import time

import json


from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot
from PyQt5.QtNetwork import QTcpSocket
from PyQt5.QtSerialPort import QSerialPort


class DataReceiver(QObject):
    packet_received = pyqtSignal(str)

    def __init__(self, parent=None, port='/dev/ttyUSB0', host="localhost", test=False):
        super().__init__(parent)
        self.test = test
        if test:
            self.data_source = QTcpSocket()
            self.port = port
            self.host = host
        if not test:
            self.data_source = QSerialPort(port)
            self.data_source.setBaudRate(115200)

    def connect_to_server(self):
        if self.test:
            self.data_source.connected.connect(self.start_receiving)
            self.data_source.readyRead.connect(self.receive_data)
            self.data_source.error.connect(self.handle_error)
            self.data_source.connectToHost(self.host, self.port)
        else:
            self.data_source.open(QSerialPort.ReadOnly)  # Open the serial port for reading
            self.data_source.readyRead.connect(self.receive_data)
            self.data_source.error.connect(self.handle_error_socket)

    def start_receiving(self):
        self.data_source.write("START".encode())

    def receive_data(self):
        while True:
            if self.data_source.bytesAvailable():
                try:
                    line = self.data_source.readLine().data().decode('utf-8')
                except UnicodeDecodeError as e:
                    print(f"Error: {e}")

                if line.startswith('{') and line.endswith('}\r\n'):
                    self.packet_received.emit(line.strip("\r\n"))

            time.sleep(0.1)

    @pyqtSlot(QTcpSocket.SocketError)
    def handle_error(self, error):
        print("Socket error:", error)

    def handle_error_socket(self, error):
        print("Serial error:", error)


