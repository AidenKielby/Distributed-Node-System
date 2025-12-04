import time
import pctree
import json

# Track in-flight async jobs:
# key = unique task ID
# value = { "future": PyFuture, "client": int, "file": str, "function": str }
active_tasks = {}
task_id_counter = 0


output_path = "results.json"  # global so results can be written


def wait_for_clients():
    while True:
        clients = pctree.client_list().strip()
        client_list = [c for c in clients.split("\n") if c.strip()]
        for i in range(len(client_list)):
            pctree.ping_client(i)
        if client_list:
            break
        time.sleep(0.5)
    return True


def get_all_json_requests(file_path, was_error):
    """Return all requests as a list, and clear them from the file."""
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
        requests = data.get('requests', [])
        data['requests'] = []
        with open(file_path, 'w') as f:
            json.dump(data, f, indent=4)
        return requests
    except FileNotFoundError:
        if not was_error:
            print(f"Error: '{file_path}' not found.")
    except json.JSONDecodeError:
        if not was_error:
            print(f"Error: Invalid JSON in '{file_path}'.")
    return []


def add_json(file_path, result, list_loc):
    with open(file_path, 'r') as f:
        data = json.load(f)
    data[list_loc].append(result)
    with open(file_path, 'w') as f:
        json.dump(data, f, indent=4)


def main(request_path, output_path_local):
    global task_id_counter
    global output_path
    output_path = output_path_local

    print("MAKE SURE TO STOP WITH CTRL+C. DON'T CLOSE TERMINAL!")
    print("[MASTER] Starting master server...")

    pctree.start_master()
    was_error = False

    while True:
        finished_ids = []
        wait_for_clients()
        clients = pctree.client_list().strip().split("\n")

        # --- submit all tasks at once ---
        requests = get_all_json_requests(request_path, was_error)
        if requests:
            was_error = False
        else:
            was_error = True

        for req in requests:
            client_index = req['client']
            file = req['file']
            function = req['function']

            if client_index < len(clients):
                try:
                    fut = pctree.send_file_async(client_index, file, function)
                    t_id = task_id_counter
                    task_id_counter += 1

                    active_tasks[t_id] = {
                        "future": fut,
                        "client": client_index,
                        "file": file,
                        "function": function
                    }

                    print(active_tasks)

                    print(f"[MASTER] Task {t_id} dispatched to client {client_index}")

                except Exception as e:
                    print(f"[MASTER] Error submitting task: {e}")
                    add_json(output_path, {
                        "client": client_index,
                        "file": file,
                        "function": function,
                        "result": f"Error: {e}"
                    }, 'results')
            else:
                # push back into requests.json if client missing
                add_json(request_path, {
                    "client": client_index,
                    "file": file,
                    "function": function
                }, "requests")

        # --- check all futures for completion immediately ---
        for tid, info in list(active_tasks.items()):
            fut = info["future"]
            #print("future: ", fut)
            #print(active_tasks)
            if fut.done():  # PyFuture has .done()
                try:
                    result = fut.result()
                except Exception as e:
                    result = f"Error: {e}"
                    continue

                # write result immediately
                add_json(output_path, {
                    "client": info["client"],
                    "file": info["file"],
                    "function": info["function"],
                    "result": result
                }, "results")

                print(f"[MASTER] Task {tid} finished: {result}")
                finished_ids.append(tid)

        # remove finished tasks
        for tid in finished_ids:
            del active_tasks[tid]
            #print("deleted: ", tid)

        # tiny sleep prevents CPU burn
        time.sleep(0.2)


if __name__ == "__main__":
    try:
        main("requests.json", "results.json")
    except KeyboardInterrupt:
        print("\n[MASTER] Caught Ctrl+C — shutting down...")
    finally:
        pctree.kill_master()
        print("[MASTER] Master killed safely.")
