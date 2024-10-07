const socket = new WebSocket('wss://stanleyesp.local/socket');

socket.addEventListener('open', () => {
        console.log('WebSocket connection opened');
        socket.send(JSON.stringify({ message: 'Hello, Server!' }));
});

socket.addEventListener('message', (event) => {
        console.log('Message from server:', event.data);
        document.getElementById("log").innerHTML = event.data;
});

socket.addEventListener('close', (event) => {
        console.log('WebSocket connection closed', event);
});

socket.addEventListener('error', (event) => {
        console.error('WebSocket error:', event);
});

// Sending a simple text message
socket.send('Hello, Server!');

// Sending JSON data
const jsonData = { type: 'greeting', content: 'Hello, Server!' };
socket.send(JSON.stringify(jsonData));

// Sending binary data (e.g., an ArrayBuffer)
const buffer = new ArrayBuffer(10); // Create a buffer of 10 bytes
socket.send(buffer);
