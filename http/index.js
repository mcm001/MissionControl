// Create WebSocket connection.
var serverUrl = "ws://" + window.location.hostname;
if (window.location.port !== '') {
serverUrl += ':' + 9002;
}
socket = new WebSocket(serverUrl, 'frcvision');

// Connection opened
socket.addEventListener('open', (event) => {
    socket.send('{}');
});

// Listen for messages
socket.addEventListener('message', (event) => {
    console.log('Message from server ', event.data);
});
