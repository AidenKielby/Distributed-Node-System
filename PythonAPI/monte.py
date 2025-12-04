import json
import time
import os

# ------------------------------
# MONTE FUNCTION
# ------------------------------
def monte(iterations: int) -> float:
    import random
    inside = 0
    for _ in range(iterations):
        x = random.random()
        y = random.random()
        if x*x + y*y <= 1:
            inside += 1
    return 4 * inside / iterations

# ------------------------------
# JSON UTILITIES
# ------------------------------
def create_requests_json():
    requests = [
        {
            "client": 0,
            "file": "monte.py",
            "function": "monte(100000000)"
        },
        {
            "client": 1,
            "file": "monte.py",
            "function": "monte(100000000)"
        }
    ]
    data = {"requests": requests}
    with open("requests.json", "w") as f:
        json.dump(data, f, indent=4)
    print("[INFO] requests.json created with Monte Carlo tasks.")

def wait_for_results():
    """Wait until both results are present in results.json."""
    found_clients = {}
    print("[INFO] Waiting for Monte Carlo results from clients 0 and 1...")

    while True:
        if not os.path.exists("results.json"):
            time.sleep(0.5)
            continue

        try:
            with open("results.json", "r") as f:
                data = json.load(f)
        except json.JSONDecodeError:
            time.sleep(0.5)
            continue

        results = data.get("results", [])
        for entry in results:
            if entry.get('file') == 'monte.py':
                client = entry.get('client')
                if client not in found_clients:
                    result_value = entry.get('result')
                    if isinstance(result_value, (int, float, str)):
                        found_clients[client] = result_value

        if 0 in found_clients and 1 in found_clients:
            print("[INFO] Both client results found!")
            return found_clients

        time.sleep(0.5)

# ------------------------------
# AVERAGING
# ------------------------------
def average_results(results_dict):
    values = list(results_dict.values())
    float_list = [float(x) for x in values]
    avg = sum(float_list) / len(float_list)
    return avg

# ------------------------------
# MAIN SCRIPT
# ------------------------------
if __name__ == "__main__":
    # Step 1: Create requests.json for the master
    create_requests_json()

    print("[INFO] Now start PCTreeMaster.py in another terminal to send tasks to clients.")
    input("Press Enter once the master is running...")

    # Step 2: Wait for results
    client_results = wait_for_results()

    # Step 3: Compute average
    avg = average_results(client_results)
    print(f"[RESULT] Monte Carlo average from clients 0 and 1: {avg}")
