const socket = new WebSocket('ws://10.1.224.124:80');

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
