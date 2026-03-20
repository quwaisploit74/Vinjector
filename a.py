from flask import Flask, render_template_string, request
import pyautogui

app = Flask(__name__)
pyautogui.FAILSAFE = False # Agar kursor tidak error di pojok layar

HTML = """
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        body { margin: 0; background: #1a1a1a; color: white; font-family: sans-serif; overflow: hidden; }
        #pad { width: 100vw; height: 70vh; background: #333; display: flex; align-items: center; justify-content: center; touch-action: none; border-bottom: 2px solid #444; }
        .btns { display: flex; height: 30vh; }
        .btn { flex: 1; display: flex; align-items: center; justify-content: center; font-weight: bold; font-size: 20px; border-right: 1px solid #444; }
        .btn:active { background: #555; }
    </style>
</head>
<body>
    <div id="pad">TOUCHPAD</div>
    <div class="btns">
        <div class="btn" onclick="fetch('/click?b=left')">KLIK KIRI</div>
        <div class="btn" onclick="fetch('/click?b=right')">KANAN</div>
    </div>
    <script>
        let pad = document.getElementById('pad');
        let lastX = 0; let lastY = 0;
        pad.addEventListener('touchmove', (e) => {
            let t = e.touches[0];
            if(lastX !== 0) {
                let dx = (t.clientX - lastX) * 2;
                let dy = (t.clientY - lastY) * 2;
                fetch(`/move?x=${dx}&y=${dy}`);
            }
            lastX = t.clientX; lastY = t.clientY;
        });
        pad.addEventListener('touchend', () => { lastX = 0; lastY = 0; });
    </script>
</body>
</html>
"""

@app.route('/')
def index(): return HTML

@app.route('/move')
def move():
    x, y = float(request.args.get('x')), float(request.args.get('y'))
    pyautogui.moveRel(x, y)
    return 'ok'

@app.route('/click')
def click():
    pyautogui.click(button=request.args.get('b'))
    return 'ok'

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
  
