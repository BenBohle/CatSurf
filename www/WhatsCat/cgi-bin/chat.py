#!/usr/bin/env python3
import datetime
import fcntl
import json
import os
import sys
import uuid
from pathlib import Path
from urllib.parse import parse_qs


DATA_FILE = Path(__file__).with_name("chat_messages.json")
MAX_BODY_BYTES = 64 * 1024
MAX_MESSAGES = 1000
MAX_NAME_LEN = 32
MAX_TEXT_LEN = 1000


def send_json(status_code, payload):
    body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    status_text = {
        200: "OK",
        201: "Created",
        400: "Bad Request",
        405: "Method Not Allowed",
        413: "Payload Too Large",
        500: "Internal Server Error",
    }.get(status_code, "OK")

    sys.stdout.write(f"Status: {status_code} {status_text}\r\n")
    sys.stdout.write("Content-Type: application/json; charset=utf-8\r\n")
    sys.stdout.write("Cache-Control: no-store\r\n")
    sys.stdout.write(f"Content-Length: {len(body)}\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.flush()
    sys.stdout.buffer.write(body)


def safe_str(value, max_len):
    text = str(value or "").strip()
    if len(text) > max_len:
        text = text[:max_len]
    return text


def now_iso():
    return datetime.datetime.utcnow().isoformat(timespec="seconds") + "Z"


def ensure_store_exists():
    if not DATA_FILE.exists():
        DATA_FILE.write_text("[]", encoding="utf-8")


def _load_messages_from_handle(fh):
    fh.seek(0)
    content = fh.read().strip()
    if not content:
        return []
    try:
        data = json.loads(content)
    except Exception:
        return []
    return data if isinstance(data, list) else []


def read_messages():
    ensure_store_exists()
    with DATA_FILE.open("r", encoding="utf-8") as fh:
        fcntl.flock(fh.fileno(), fcntl.LOCK_SH)
        messages = _load_messages_from_handle(fh)
        fcntl.flock(fh.fileno(), fcntl.LOCK_UN)
        return messages


def append_message(message):
    ensure_store_exists()
    with DATA_FILE.open("r+", encoding="utf-8") as fh:
        fcntl.flock(fh.fileno(), fcntl.LOCK_EX)
        messages = _load_messages_from_handle(fh)
        messages.append(message)
        if len(messages) > MAX_MESSAGES:
            messages = messages[-MAX_MESSAGES:]
        fh.seek(0)
        json.dump(messages, fh, ensure_ascii=False, separators=(",", ":"))
        fh.truncate()
        fh.flush()
        os.fsync(fh.fileno())
        fcntl.flock(fh.fileno(), fcntl.LOCK_UN)


def delete_message(message_id):
    ensure_store_exists()
    with DATA_FILE.open("r+", encoding="utf-8") as fh:
        fcntl.flock(fh.fileno(), fcntl.LOCK_EX)
        messages = _load_messages_from_handle(fh)
        before = len(messages)
        messages = [m for m in messages if str(m.get("id", "")) != message_id]
        fh.seek(0)
        json.dump(messages, fh, ensure_ascii=False, separators=(",", ":"))
        fh.truncate()
        fh.flush()
        os.fsync(fh.fileno())
        fcntl.flock(fh.fileno(), fcntl.LOCK_UN)
        return before - len(messages)


def read_request_body():
    raw_length = os.environ.get("CONTENT_LENGTH", "0")
    try:
        length = int(raw_length)
    except ValueError:
        length = 0
    if length < 0:
        length = 0
    if length > MAX_BODY_BYTES:
        raise ValueError("payload too large")
    return sys.stdin.read(length) if length > 0 else ""


def parse_post_payload():
    body = read_request_body()
    if not body:
        return {}

    content_type = os.environ.get("CONTENT_TYPE", "").lower()
    if "application/json" in content_type:
        try:
            data = json.loads(body)
            return data if isinstance(data, dict) else {}
        except Exception:
            return {}

    parsed = parse_qs(body, keep_blank_values=True)
    return {k: (v[0] if isinstance(v, list) and v else "") for k, v in parsed.items()}


def handle_get():
    messages = read_messages()
    send_json(200, {"messages": messages})


def handle_post():
    payload = parse_post_payload()
    sender = safe_str(payload.get("sender", "Anonymous"), MAX_NAME_LEN)
    text = safe_str(payload.get("text", ""), MAX_TEXT_LEN)

    if not text:
        send_json(400, {"error": "text is required"})
        return

    message = {
        "id": uuid.uuid4().hex,
        "sender": sender if sender else "Anonymous",
        "text": text,
        "timestamp": now_iso(),
    }

    append_message(message)
    send_json(201, {"message": message})


def handle_delete():
    query = parse_qs(os.environ.get("QUERY_STRING", ""), keep_blank_values=True)
    target_id = safe_str((query.get("id", [""])[0]), 128)
    if not target_id:
        send_json(400, {"error": "id query parameter required"})
        return

    deleted = delete_message(target_id)
    send_json(200, {"ok": True, "deleted": deleted})


def main():
    method = os.environ.get("REQUEST_METHOD", "GET").upper()
    try:
        if method == "GET":
            handle_get()
        elif method == "POST":
            handle_post()
        elif method == "DELETE":
            handle_delete()
        else:
            send_json(405, {"error": "method not allowed"})
    except ValueError as exc:
        if "payload too large" in str(exc):
            send_json(413, {"error": "payload too large"})
        else:
            send_json(400, {"error": "bad request"})
    except Exception:
        send_json(500, {"error": "internal server error"})


if __name__ == "__main__":
    main()
