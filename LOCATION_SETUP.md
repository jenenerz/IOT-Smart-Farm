# Location-Based Data Separation Guide

## Overview
The Smart Farm system now separates sensor data by location (North, South, East, West) in both the UI and Supabase database. This makes it easy to identify which field each sensor reading came from.

## Setup Instructions

### Step 1: Configure Arduino Nodes
For each Arduino/ESP8266 node, update the following lines in `smart_farm.ino`:

```cpp
const char SENSOR_ID[] = "NODE_01";      // Unique ID for each node
const char LOCATION_NAME[] = "North";    // One of: "North", "South", "East", "West"
```

**Example Configuration:**
- Node 1: `SENSOR_ID = "NODE_01"`, `LOCATION_NAME = "North"`
- Node 2: `SENSOR_ID = "NODE_02"`, `LOCATION_NAME = "South"`
- Node 3: `SENSOR_ID = "NODE_03"`, `LOCATION_NAME = "East"`
- Node 4: `SENSOR_ID = "NODE_04"`, `LOCATION_NAME = "West"`

### Step 2: How Data is Organized

#### In Supabase
When sensor data is sent to Supabase, it includes location information:
- **Feed Name**: Includes location label, e.g., `"temperature [North]"`, `"humidity [South]"`
- **Value**: The sensor reading
- **Created At**: Timestamp of the reading

#### In the UI
The dashboard now displays:

1. **Location Tabs**: Click to filter data by location
   - 📊 All Locations (shows all data)
   - 🧭 North
   - 🧭 South
   - 🧭 East
   - 🧭 West

2. **Data Table**: Shows filtered sensor logs with:
   - Record ID
   - Location (color-coded with location emoji)
   - Value (sensor reading)
   - Timestamp

3. **Panel Header**: Dynamically updates to show which location is selected

### Step 3: Data Flow

```
Arduino Node (with location) 
    ↓
POST /sensor-data (backend/main.py)
    ↓
SQLite Database (stores location field)
    ↓
Supabase (saves with location label)
    ↓
UI Dashboard (filtered by location tabs)
```

## Features

### Location Colors
- **North**: Red/Orange (#c84a1a)
- **South**: Blue (#0d6eaa)
- **East**: Purple (#7b1fa2)
- **West**: Amber/Gold (#b07a10)

### Smart Location Detection
The system automatically extracts location from:
1. Square brackets in feed name: `[North]`, `[South]`, etc.
2. Location keywords in sensor data
3. Node mapping (if configured in NODE_MAP)

## API Endpoint

When sending data from Arduino to backend:

```
POST /sensor-data
Content-Type: application/json

{
  "sensor_id": "NODE_01",
  "location": "North",
  "light": 450,
  "temperature_c": 28.5,
  "humidity_percent": 65.0,
  "day_night": "DAY"
}
```

The backend automatically saves this to SQLite with the location field preserved.

## Troubleshooting

### Data not showing in specific location tab
1. Check Arduino `LOCATION_NAME` is set correctly (exactly "North", "South", "East", or "West")
2. Verify backend is running and receiving data
3. Check Supabase connection in dashboard
4. Refresh the page and check if location filter buttons appear

### Location name not recognized
- Ensure you use exact case: `"North"`, not `"north"` or `"NORTH"`
- Update the LOCATION_MAP in index.html if using custom location names
- Check the feed name in Supabase - it should contain the location in brackets

### All data shows as "Unknown" location
- Verify the feed_name in Supabase includes location information
- Check that `saveToSupabase()` function in index.html is properly embedding location labels
- Review browser console for any JavaScript errors

## Testing Checklist

- [ ] Arduino is sending data with correct LOCATION_NAME
- [ ] Backend receives and logs data with location field
- [ ] Supabase shows feed names with location labels
- [ ] UI displays location tabs
- [ ] Clicking location tabs filters data correctly
- [ ] Location colors display in the table
- [ ] Panel header updates when switching locations

## Next Steps

1. Deploy Arduino code to 4 different nodes with different locations
2. Monitor the dashboard to see data from each location
3. Use Supabase for data analysis by location
4. Add location-based alerts if needed (future feature)
