from datetime import datetime, timezone
from pathlib import Path
import sqlite3
from typing import Literal

from fastapi import FastAPI, Request
from fastapi.responses import FileResponse
from pydantic import BaseModel, Field

app = FastAPI(title="Smart Farm Backend", version="1.0.0")

DB_PATH = Path(__file__).resolve().parent / "smart_farm.db"
DASHBOARD_PATH = Path(__file__).resolve().parent.parent / "dashboard.html"
MAX_READINGS = 200


class SensorPayload(BaseModel):
    sensor_id: str
    location: str
    light: int = Field(..., ge=0)
    temperature_c: float
    humidity_percent: float = Field(..., ge=0, le=100)
    day_night: Literal["DAY", "NIGHT"] = "DAY"


def get_db_connection() -> sqlite3.Connection:
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db() -> None:
    with get_db_connection() as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp_utc TEXT NOT NULL,
                client_ip TEXT NOT NULL,
                sensor_id TEXT NOT NULL,
                location TEXT NOT NULL,
                light INTEGER NOT NULL,
                temperature_c REAL NOT NULL,
                humidity_percent REAL NOT NULL,
                day_night TEXT NOT NULL
            )
            """
        )
        conn.commit()


def row_to_reading(row: sqlite3.Row) -> dict:
    return {
        "id": row["id"],
        "timestamp_utc": row["timestamp_utc"],
        "client_ip": row["client_ip"],
        "sensor_id": row["sensor_id"],
        "location": row["location"],
        "light": row["light"],
        "temperature_c": row["temperature_c"],
        "humidity_percent": row["humidity_percent"],
        "day_night": row["day_night"],
    }


@app.on_event("startup")
def on_startup() -> None:
    init_db()


@app.get("/")
def health_check() -> dict:
    return {
        "status": "ok",
        "service": "smart-farm-backend",
        "message": "POST sensor data to /sensor-data",
    }


@app.get("/dashboard")
def dashboard() -> FileResponse:
    return FileResponse(DASHBOARD_PATH)


@app.post("/sensor-data")
async def ingest_sensor_data(payload: SensorPayload, request: Request) -> dict:
    client_ip = request.client.host if request.client else "unknown"

    reading = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "client_ip": client_ip,
        "sensor_id": payload.sensor_id,
        "location": payload.location,
        "light": payload.light,
        "temperature_c": payload.temperature_c,
        "humidity_percent": payload.humidity_percent,
        "day_night": payload.day_night,
    }

    with get_db_connection() as conn:
        cursor = conn.execute(
            """
            INSERT INTO readings (
                timestamp_utc,
                client_ip,
                sensor_id,
                location,
                light,
                temperature_c,
                humidity_percent,
                day_night
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                reading["timestamp_utc"],
                reading["client_ip"],
                reading["sensor_id"],
                reading["location"],
                reading["light"],
                reading["temperature_c"],
                reading["humidity_percent"],
                reading["day_night"],
            ),
        )
        conn.commit()
        reading["id"] = cursor.lastrowid

    print(
        f"[RX] {reading['timestamp_utc']} | {reading['sensor_id']} | {reading['location']} | "
        f"Light={reading['light']} | Temp={reading['temperature_c']}C | Hum={reading['humidity_percent']}%"
    )

    return {"ok": True, "message": "Data received", "reading": reading}


@app.post("/")
async def ingest_sensor_data_legacy(payload: SensorPayload, request: Request) -> dict:
    return await ingest_sensor_data(payload, request)


@app.get("/readings")
def list_recent_readings(limit: int = 20) -> dict:
    safe_limit = max(1, min(limit, MAX_READINGS))

    with get_db_connection() as conn:
        rows = conn.execute(
            """
            SELECT id, timestamp_utc, client_ip, sensor_id, location, light, temperature_c, humidity_percent, day_night
            FROM readings
            ORDER BY id DESC
            LIMIT ?
            """,
            (safe_limit,),
        ).fetchall()

    items = [row_to_reading(row) for row in reversed(rows)]

    return {
        "count": len(items),
        "items": items,
    }


@app.get("/api/sensors")
def dashboard_data() -> dict:
    with get_db_connection() as conn:
        rows = conn.execute(
            """
            SELECT r1.id, r1.sensor_id, r1.location, r1.temperature_c, r1.humidity_percent, r1.timestamp_utc
            FROM readings r1
            INNER JOIN (
                SELECT sensor_id, MAX(id) AS max_id
                FROM readings
                GROUP BY sensor_id
            ) r2 ON r1.sensor_id = r2.sensor_id AND r1.id = r2.max_id
            ORDER BY r1.id DESC
            """
        ).fetchall()

        deluge_count_row = conn.execute(
            """
            SELECT COUNT(*) AS nighttime_count
            FROM readings
            WHERE day_night = 'NIGHT'
            """
        ).fetchone()

    sensors = [
        {
            "sensor_id": row["sensor_id"],
            "location_name": row["location"],
            "temperature": row["temperature_c"],
            "humidity": row["humidity_percent"],
            "status": "active",
            "timestamp": row["timestamp_utc"],
        }
        for row in rows
    ]

    return {
        "connected": len(sensors) > 0,
        "sensors": sensors,
        "deluge_count": deluge_count_row["nighttime_count"] if deluge_count_row else 0,
    }
