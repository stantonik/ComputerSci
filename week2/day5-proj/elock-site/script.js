
const body = document.body;
const overlay = document.getElementById("overlay");

/* WebSocket handle */
const socket = new WebSocket("ws://elock.local/ws");

socket.addEventListener('open', () => {
        console.log('WebSocket connection opened');
        overlay.style.display = 'none';
        body.classList.remove('disable-interaction');
});

socket.addEventListener('message', (event) => {
        console.log('Message from server:', event.data);
        ws_callback(JSON.parse(event.data));
});

socket.addEventListener('close', (event) => {
        console.log('WebSocket connection closed', event);
        overlay.style.display = 'flex';
        body.classList.add('disable-interaction');
});

socket.addEventListener('error', (event) => {
        console.error('WebSocket error:', event);
        overlay.innerHTML = "Connection failed. Retrying..."
        overlay.style.display = 'flex';
        body.classList.add('disable-interaction');
});

/* Lock handle */
const lock_container = document.getElementById('lock-container');
const blurry_div = document.getElementById("blurry");
const warning_div = document.getElementById("warning-div");

const close_sound = new Audio("sound/lockclose.mp3");
const open_sound = new Audio("sound/lockopen.mp3");
const error_sound = new Audio("sound/error.mp3");

var lock_state;
var is_error = false;

function ws_callback(data)
{
        if ("state" in data)
        {
                lock_state = data.state;
                toggle_gui_lock();
        }
        else if ("error" in data)
        {
                let interval;
                if (data.error != "success")
                {
                        is_error = true;
                        warning_div.style.opacity = 1;
                        warning_div.classList.add("error");
                        interval = setInterval(() => {
                                error_sound.play();
                        }, 1000);
                }
                else
                {
                        is_error = false;
                        warning_div.style.opacity = 0;
                        warning_div.classList.remove("error");
                        clearTimeout(interval);
                }
        }
}

function toggle_gui_lock()
{
        if (lock_state == "close")
        {
                lock_container.classList.add('close');
                blurry_div.classList.add('close');
        }
        else if (lock_state == "open")
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
                open_sound.play();
        }
        else if (lock_state == "open")
        {
                socket.send("close");
                close_sound.play();
        }
}

lock_container.addEventListener('click', toggle_lock);
