"""Exercise an already running app's external control, leaving its model fitted."""
import json
import pathlib
import sys
import time
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from control import request

directory = sys.argv[1]
records = []
def call(command, **kwargs):
    result = request(directory, dict(command=command, **kwargs))
    records.append(dict(command=command, arguments=kwargs, result=result))
    return result

initial = call("list")
assert initial["items"], "No real model loaded"
changed = call("set_transform", index=0, scale=.4, x=120, y=-50)
assert abs(changed["scale"]-.4)<1e-6 and changed["position"]==[120,-50]
time.sleep(.3)
queried = call("list")["items"][0]
assert abs(queried["scale"]-.4)<1e-6 and queried["position"]==[120,-50]
assert all(abs(a-b)<.01 for a,b in zip(changed["bounds"],queried["bounds"])), "Stale command response bounds"
for invalid in (dict(command="set_transform",index=0,scale=-1,x=999),
                dict(command="set_transform",index=999,scale=1),
                dict(command="fit",index=0,margin=10000),
                dict(command="unknown",index=0)):
    try:
        request(directory, invalid)
    except RuntimeError as error:
        records.append(dict(rejected=invalid,error=str(error)))
    else:
        raise AssertionError("Invalid request accepted")
after = call("list")["items"][0]
assert after["position"]==queried["position"] and after["scale"]==queried["scale"], "Invalid request mutated model"
fitted = call("fit",index=0,margin=90)
time.sleep(.3)
after = call("list")["items"][0]
assert after["scale"]==fitted["scale"] and after["position"]==fitted["position"]
path=pathlib.Path(sys.argv[2])
path.write_text(json.dumps(dict(status="passed",records=records),ensure_ascii=False,indent=2),encoding="utf-8")
print("PASS: live model query, scale/position, response bounds, invalid-request atomicity, fit and persistence across frames")
