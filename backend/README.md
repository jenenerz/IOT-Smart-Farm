# Smart Farm FastAPI Backend

This backend is compatible with the current Arduino sketch settings:
- Host: `192.168.1.13`
- Port: `8000`
- Path: `/sensor-data`
- Method: `POST`
- Content-Type: `application/json`

## 1) Install dependencies

```powershell
cd backend
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

## 2) Run the server

```powershell
uvicorn main:app --host 0.0.0.0 --port 8000
```

## 3) Endpoints

- `GET /` : health check
- `POST /sensor-data` : receives Arduino JSON payload
- `POST /` : legacy compatibility path (same behavior)
- `GET /readings?limit=20` : recent readings from SQLite
- `GET /dashboard` : serves `dashboard.html`
- `GET /api/sensors` : dashboard-friendly latest sensor summary

## 4) Storage

- The backend now stores data in `backend/smart_farm.db` (SQLite).
- The database/table are auto-created on server startup.

## Example payload from Arduino

```json
{
  "sensor_id": "NODE_01",
  "location": "North_Field",
  "light": 481,
  "temperature_c": 30.4,
  "humidity_percent": 59.0,
  "day_night": "DAY"
}
```
