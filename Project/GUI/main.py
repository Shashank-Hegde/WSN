# Standard library imports
import itertools
import json
import logging
import math
import signal
import sys
from datetime import datetime
from pathlib import Path
from random import randint
from typing import List
import networkx as nx

# Third-party library imports
from PyQt5.QtCore import QThread, QSize, Qt, QPoint, QTimer
from PyQt5.QtGui import QPixmap, QPainter, QPen, QVector2D, QColor, QFont
from PyQt5.QtWidgets import (QApplication, QMainWindow, QLabel, QPushButton, QWidget, QVBoxLayout,
                             QScrollArea, QGridLayout, QProgressBar, QFrame, QTabWidget, QAction, QGraphicsScene,
                             QGraphicsView, QListWidget)

# Local application imports
from data_receiver import DataReceiver

# Configure basic logging parameters
logging.basicConfig(filename="packet_log.txt", level=logging.INFO, format="%(asctime)s - %(message)s")

# Define various constants
LOG_FILE = "packet_log.txt"
LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)s — %(name)s — %(levelname)s — %(funcName)s:%(lineno)d — %(message)s"
PLAYER_IMAGE_PATH = Path(__file__).parent / "assets"
MAX_NO_OF_NODES = 7
DEFAULT_NODE_SIZE = QSize(100, 100)
IMAGE_SCALING = 80
TEST_ARGUMENT = "test"
PORT_NUMBER = '/dev/ttyUSB0'
# PORT_NUMBER = 8000

# Logger initialization
logging.basicConfig(level=logging.INFO, format=LOG_FORMAT)
LOGGER = logging.getLogger(__name__)


# Custom dictionary that automatically assigns an increasing integer value as the value for missing keys
class AutoLenDict(dict):
    def __missing__(self, key):
        self[key] = len(self)
        return self[key]


# Custom progress bar that shows more detailed text
class CustomProgressBar(QProgressBar):
    def text(self):
        return f"{self.value()} ({self.minimum()} - {self.maximum()})"


# Widget that displays nodes
class NodesWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.nodes_layout = QGridLayout()
        self.setLayout(self.nodes_layout)

        self.graph = nx.Graph()  # Create a networkx graph
        self.scene = QGraphicsScene(self)
        self.view = QGraphicsView(self.scene)

        self.nodes_layout.addWidget(self.view)

        self.switch_image = QPixmap(str(PLAYER_IMAGE_PATH / "switch.jpg"))
        self.gateway_image = QPixmap(str(PLAYER_IMAGE_PATH / "gateway.jpg"))

        self.colors = itertools.cycle(['#0072BD', '#D95319', '#EDB120', '#7E2F8E', '#77AC30', '#4DBEEE', '#A2142F'])

        # Initialize timer and set its interval to 10000 milliseconds (10 seconds)
        self.reset_edge_timer = QTimer(self)
        self.reset_edge_timer.setInterval(2000)
        self.reset_edge_timer.timeout.connect(self.reset_edges)

        # Initialize a second timer with an interval of 10000 milliseconds (10 seconds)
        self.timer_for_hops = QTimer(self)
        self.timer_for_hops.setInterval(1000)  # Set the interval as per your requirements
        self.timer_for_hops.timeout.connect(self.update_hop_counts)
        self.timer_for_hops.start()

    def resizeEvent(self, event):
        super().resizeEvent(event)  # Let the superclass handle the event first
        self.draw_graph()  # Redraw the graph

    def update_hop_counts(self):

        for node, data in self.graph.nodes(data=True):
            if data.get('node_type', False) == 'N' and (datetime.now() - data.get('timestamp', datetime.now())).total_seconds() >= 12  :
                data['hops'] = 255

    def reset_edges(self):
        # Remove all edges from the graph
        self.graph.remove_edges_from(self.graph.edges())
        # Redraw the graph
        self.draw_graph()


    # Draws a path between the nodes specified in the 'path' parameter
    def draw_path(self, path):

        # self.graph.remove_edges_from(self.graph.edges())  # Remove all edges from the graph

        # Check if the nodes for the new edges exist, else create nodes
        for node in path:
            if node not in self.graph.nodes:
                self.graph.add_node(node, node_type = "N", hops=1, timestamp = datetime.now())
            elif self.graph.nodes[node]['node_type'] == "N":
                # Add code to set the number of hops to 0, node_type to "N"
                self.graph.nodes[node]['hops'] = 1
                self.graph.nodes[node]['timestamp'] = datetime.now()

        # for node in path:
        #     attrs = self.graph.nodes[node]
        #     if 'hops' in attrs and int(attrs['hops']) >= MAX_NO_OF_NODES-2:
        #         return  # If 'hops' attribute does not exist or is 5 or more, return without plotting


        # Get the next color from the generator
        edge_color = next(self.colors)

        # Add edges in the graph in NetworkX
        for i in range(len(path) - 1):
            u = path[i]
            v = path[i + 1]
            if not self.graph.has_edge(u, v):
                self.graph.add_edge(u, v, color=edge_color)

        self.draw_graph()  # Redraw the graph
        self.reset_edge_timer.start()

    def draw_graph(self, clear_edges=False):
        # Clear the previous nodes and edges in the scene
        self.scene.clear()

        # Position the nodes using a layout algorithm (e.g., spring layout) with maximum spacing
        scale = 5.0
        k = 15
        pos = nx.spring_layout(self.graph, seed=42, scale=scale, k=k)
        square_size = 50 * scale  # Increase the node size relative to k

        # Draw the edges
        # Draw the edges
        for u, v, data in self.graph.edges(data=True):
            # Get the color from the edge data, if not available, use black
            edge_color = data.get('color', 'black')

            # Create a pen with the color and increased thickness
            pen = QPen(QColor(edge_color))
            pen.setWidth(15)  # Increase the pen width

            # Draw the line from the center of the node with the color and increased thickness
            self.scene.addLine(
                pos[u][0] * 200 + square_size / 2, pos[u][1] * 200 + square_size / 2,
                pos[v][0] * 200 + square_size / 2, pos[v][1] * 200 + square_size / 2,
                pen
            )

        # Draw the nodes
        for node, attrs in self.graph.nodes(data=True):
            node_type = attrs.get("node_type", "")
            node_address = attrs.get("node_address", "")
            node_status = attrs.get("still_active", False)
            hops = int(attrs.get("hops", 0))
            node_color = QColor.fromHsl(120, 130, 127) if hops < 5 else QColor.fromHsl(0, 130, 127)

            square = self.scene.addRect(pos[node][0] * 200, pos[node][1] * 200, square_size, square_size,
                                        QPen(Qt.NoPen), node_color)

            # Depending on the node type, use a different image
            if node_type == "G":
                player_image = self.gateway_image
            elif node_type == "S":
                player_image = self.switch_image
            else:
                player_image = QPixmap(str(PLAYER_IMAGE_PATH / f"{node}.jpg"))

            # Scale the image to the desired size and add it on top of the square
            player_image = player_image.scaled(QSize(int(square_size), int(square_size)), Qt.KeepAspectRatio)

            # Scale the image to slightly less than the size of the square to create a border effect
            image_scale_factor = 0.8  # Adjust this as needed
            image_size = int(square_size * image_scale_factor)
            player_image = player_image.scaled(QSize(image_size, image_size), Qt.KeepAspectRatio)

            # Offset the image position to center it within the square
            image_offset = (square_size - image_size) / 2
            self.scene.addPixmap(player_image).setPos(pos[node][0] * 200 + image_offset,
                                                      pos[node][1] * 200 + image_offset)

            text = f"{node}\n"
            if node_type and node_address:  # Only fill the text if attributes are available
                text = f"{text}Type: {node_type}\nAddress: {node_address}\nTimestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
            text_item = self.scene.addText(text, QFont("Arial", int(8 * scale)))  # Increase the font size relative to k
            text_item.setDefaultTextColor(node_color)
            text_item.setPos(pos[node][0] * 200 + square_size + 5, pos[node][1] * 200 + 10)


        self.view.fitInView(self.scene.sceneRect(), Qt.KeepAspectRatio)

    def update_nodes(self, routing_table):

        print(routing_table)

        for i, (key, routing_table_entry) in enumerate(routing_table.items()):
            node_address = routing_table_entry["node_address"]
            node_status = True if routing_table_entry["still_active"].lower() == 'true' else False
            defined_node_number = routing_table_entry["node_id"]

            # Eliminate nodes that are not valid.
            if not(0<int(defined_node_number)<=MAX_NO_OF_NODES):
                continue

            if defined_node_number not in self.graph.nodes:
                self.graph.add_node(defined_node_number,
                                    node_type=routing_table_entry["node_type"],
                                    node_address=node_address, still_active=node_status, node_id=defined_node_number,
                                    hops=routing_table_entry["hops"])

            else:
                self.graph.nodes[defined_node_number]["node_type"] = routing_table_entry["node_type"]
                self.graph.nodes[defined_node_number]["node_address"] = node_address
                self.graph.nodes[defined_node_number]["still_active"] = node_status
                self.graph.nodes[defined_node_number]["hops"] = routing_table_entry["hops"]

        # self.graph.remove_edges_from(self.graph.edges())  # Remove all edges from the graph
        self.draw_graph()

    # Clears the images from all the nodes


# Widget that represents a player card
class PlayerCardWidget(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setFrameShape(QFrame.StyledPanel)
        self.setFrameShadow(QFrame.Raised)
        self.setLineWidth(0)

        # Define the widget's style
        self.setStyleSheet("""
            QFrame {
                background-color: #FFFFFF;
                border: 1px solid #BDBDBD;
                border-radius: 2px;
                margin: 5px;
                padding: 10px;
            }
            QLabel {
                font-size: 18px;
            }
            QProgressBar {
                border: 2px solid #000;
                border-radius: 5px;
            }
            QProgressBar::chunk {
                background-color: #05B8CC;
                width: 20px;
            }
        """)

        # Create labels and progress bars
        self.player_label = QLabel()
        self.pressure_bar = CustomProgressBar()
        self.pressure_bar.setMinimum(0)
        self.pressure_bar.setMaximum(2000)

        self.heart_rate_bar = CustomProgressBar()
        self.heart_rate_bar.setMinimum(0)
        self.heart_rate_bar.setMaximum(2000)

        self.battery_level = CustomProgressBar()
        self.battery_level.setMinimum(3000)
        self.battery_level.setMaximum(3700)

        # Create layout and add widgets to it
        layout = QVBoxLayout()
        layout.addWidget(self.player_label)
        layout.addWidget(QLabel('Pressure:'))
        layout.addWidget(self.pressure_bar)
        layout.addWidget(QLabel('Heart Rate:'))
        layout.addWidget(self.heart_rate_bar)
        layout.addWidget(QLabel('Battery Level:'))
        layout.addWidget(self.battery_level)

        self.setLayout(layout)

    # Update the player card with new data
    def update_card(self, player, pressure, oximeter, battery_level):
        min_pressure, max_pressure = self.pressure_bar.minimum(), self.pressure_bar.maximum()
        min_heart_rate, max_heart_rate = self.heart_rate_bar.minimum(), self.heart_rate_bar.maximum()
        min_battery_level, max_battery_level = self.battery_level.minimum(), self.battery_level.maximum()

        self.player_label.setText(f"Player: {player}")

        self.pressure_bar.setValue(int(pressure))
        self.pressure_bar.setStyleSheet(self.generate_stylesheet(int(pressure), min_pressure, max_pressure))

        self.heart_rate_bar.setValue(int(oximeter))
        self.heart_rate_bar.setStyleSheet(self.generate_stylesheet(int(oximeter), min_heart_rate, max_heart_rate))

        self.battery_level.setValue(int(battery_level))
        self.battery_level.setStyleSheet(
            self.generate_stylesheet(int(battery_level), min_battery_level, max_battery_level))

    # Generate the color for the progress bar based on the value
    def generate_stylesheet(self, value, min_value, max_value):
        normalized_value = (value - min_value) / (max_value - min_value) * 100
        hue = (100 - normalized_value) * 1.2  # Hue value range is 0-120 for red to green
        color = QColor.fromHsl(max(min(120, int(hue)), 0), 100, 127)  # Reduced saturation to 150 for more muted colors
        color_str = f"rgb({color.red()},{color.green()},{color.blue()})"
        return f"""
            QProgressBar::chunk {{
                background-color: {color_str};
                width: 20px;
            }}
        """


# Widget that represents the message display area
class MessageWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        layout = QVBoxLayout()
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(20)  # Add some spacing between widgets

        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)

        self.content_widget = QWidget()
        self.content_layout = QVBoxLayout(self.content_widget)  # Adjust this to QVBoxLayout
        self.content_layout.setSpacing(20)  # Add some spacing between widgets

        # Create and add labels
        label1 = QLabel("Player Vitals:")
        self.content_layout.addWidget(label1)

        scroll_area.setWidget(self.content_widget)
        layout.addWidget(scroll_area)

        message_list_label = QLabel("Message Log:")
        layout.addWidget(message_list_label)

        self.messageList = QListWidget()  # Add a QListWidget to display messages
        layout.addWidget(self.messageList)
        self.setLayout(layout)
        self.players = {}

    # This function can be called to add a new message to the list
    def add_message_to_list(self, timestamp, message):
        self.messageList.addItem(f"{timestamp}: {message}")

    # Update the messages displayed for each player
    def update_message(self, player_data):
        player = player_data["player"]

        if player not in self.players:
            self.players[player] = PlayerCardWidget()
            self.content_layout.addWidget(self.players[player])

        self.players[player].update_card(player, player_data["pressure"], player_data["oximeter"],
                                         player_data["battery"])


# Main application window
class MainWindow(QMainWindow):
    def __init__(self, test, port):
        super().__init__()
        self.nodes_widget = NodesWidget()
        self.nodes_widget.setWindowTitle("Nodes")  # Add title
        self.message_widget = MessageWidget()
        self.message_widget.setWindowTitle("Messages")  # Add title

        # create tab widget
        self.tab_widget = QTabWidget()
        self.tab_widget.addTab(self.nodes_widget, "Nodes")  # add nodes widget to the first tab
        self.tab_widget.addTab(self.message_widget, "Messages")  # add messages widget to the second tab

        # set tab widget as the central widget of the main window
        self.setCentralWidget(self.tab_widget)

        self.data_receiver = DataReceiver(test=test, port=port)
        self.data_receiver.packet_received.connect(self.update_gui)

        self.toolbar = self.addToolBar('Exit')
        self.exit_action = QAction('Exit', self)
        self.exit_action.triggered.connect(self.safe_exit)
        self.toolbar.addAction(self.exit_action)

        self.socket_thread = QThread()
        self.data_receiver.moveToThread(self.socket_thread)
        self.data_receiver.connect_to_server()
        self.socket_thread.start()

        self.setStyleSheet("""
            QMainWindow {
                background-color: #444444;
            }
        """)

    def safe_exit(self):
        LOGGER.info("Exiting")
        self.close_threads()
        QApplication.quit()

    def close_threads(self):
        try:
            self.socket_thread.quit()
        except Exception as e:
            LOGGER.exception("Exception when closing threads: ", e)

    def update_gui(self, packet):
        try:
            json_data = json.loads(packet)
            LOGGER.info(json_data)

            if "Entry 1" in json_data:
                routing_table = json_data
                self.nodes_widget.update_nodes(routing_table)
                LOGGER.info("Routing Received")

            elif "Force" in json_data:
                player_data = {"player": str(json_data["Path"])[0], "pressure": json_data["Force"],
                               "oximeter": json_data["Oximeter"], "battery": json_data["Battery"]}
                self.message_widget.update_message(player_data)
                self.nodes_widget.draw_path(list(str(json_data["Path"])))

                message = f'Player: {str(json_data["Path"])[0]}, Pressure: {json_data["Force"]}, ' \
                          f'Heart Rate: {json_data["Oximeter"]}, Battery Level: {json_data["Battery"]}, ' \
                          f'Path: [{"->".join(list(str(json_data["Path"])))}]'
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                self.message_widget.add_message_to_list(timestamp=timestamp, message=message)

                LOGGER.info("Message Received")

            LOGGER.info(packet)
        except json.JSONDecodeError:
            LOGGER.error("Invalid JSON packet:", packet)
        except Exception as e:
            LOGGER.exception("Exception occurred:", e)


def close_socket(signal, frame):
    LOGGER.info("Closing socket")
    window.data_receiver.data_source.close()
    QApplication.quit()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, close_socket)
    signal.signal(signal.SIGTERM, close_socket)
    signal.signal(signal.SIGSEGV, close_socket)

    app = QApplication(sys.argv)
    window = MainWindow(test=TEST_ARGUMENT in sys.argv, port=PORT_NUMBER)
    window.show()

    sys.exit(app.exec_())
