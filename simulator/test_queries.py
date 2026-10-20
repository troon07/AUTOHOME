import urllib.request
import json

def send(txt):
    req = urllib.request.Request(
        'http://127.0.0.1:8000/api/voice',
        data=json.dumps({'text': txt}).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )
    res = json.loads(urllib.request.urlopen(req).read().decode('utf-8'))
    print(f"Input:  '{txt}'")
    print(f"Result: Action={res.get('action')}, Room={res.get('room')}, Device={res.get('device')}")
    print(f"Voice:  \"{res.get('spoken_response')}\"\n")

send("I am going to sleep")
send("It is too hot in here")
send("of bedrrom")
send("turn on the light")
send("office")
