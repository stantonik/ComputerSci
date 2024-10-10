const socket = new WebSocket("ws://elock.local/ws");

socket.addEventListener('open', () => {
        console.log('WebSocket connection opened');
        ask_lock_state();
});

socket.addEventListener('message', (event) => {
        console.log('Message from server:', event.data);
        ws_callback(JSON.parse(event.data));
});

socket.addEventListener('close', (event) => {
        console.log('WebSocket connection closed', event);
});

socket.addEventListener('error', (event) => {
        console.error('WebSocket error:', event);
});

document.getElementById('sendcmd-button').addEventListener('click', function () {
        const text_input = document.getElementById('cmd-input');
        const message = text_input.value;

        if (message) {
                socket.send(message);
                text_input.value = '';
        }
});


const lock_container = document.getElementById('lock-container');
const blurry_div = document.getElementById("blurry");

var lock_state;

function ws_callback(data)
{
        if ("state" in data)
        {
                lock_state = data.state;
                toggle_gui_lock();
        }
}

function ask_lock_state()
{
        socket.send("state");
}

function toggle_gui_lock()
{
        if (lock_state == "close")
        {
                lock_container.classList.add('close');
                blurry_div.classList.add('close');
        }
        else
        {
                lock_container.classList.remove('close');
                blurry_div.classList.remove('close');
        }
}

function toggle_lock()
{
        if (lock_state == "close")
        {
                socket.send("open");
        }
        else
        {
                socket.send("close");
        }
}

lock_container.addEventListener('click', toggle_lock);

