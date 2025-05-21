from flask_socketio import SocketIO

def register_socketio_handlers(socketio: SocketIO):
    """
    Register all SocketIO event handlers.
    """
    @socketio.on('connect')
    def handle_connect():
        print('Client connected')
        socketio.emit('message', 'Connected to server')

    @socketio.on('message')
    def handle_message(message):
        print('Received message:', message)
