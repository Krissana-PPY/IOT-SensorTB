import math
import re
from datetime import datetime
import logging
import requests
from config import (
    distance, distance_true, angle_x, angle_y, sub_pallet, sub_row, current_id,
    last_row_id, last_max_range_id, last_max_range_value, url
)

# --- Data Formatting Helpers ---

def FormatPalletData(np):
    """Format pallet data for web interface."""
    return {"pallet_v": np}

def FormatPointData(dt):
    """Format point data for web interface."""
    return {'point': dt}

def FormatAllZero():
    """Format zero data for web interface."""
    return {'pallet_v': 0, 'point': 0}

def FormatPointZero():
    """Format zero point data for web interface."""
    return {'point': 0}

def FormatCurrentRow(row):
    """Format current row data for web interface."""
    return {'c_row_v': row}

def FormatRow(row):
    """Format row data for web interface."""
    return {'row_v': row}

# --- Data Extraction and Calculation ---

def ExtractNumericValues(input_str):
    """Extract numeric values from the input string."""
    return re.findall(r'[-+]?\d*\.\d+|\d+', input_str)

def CollectSensorData(input_str):
    """Collect and append data from the input string to the respective lists."""
    data = ExtractNumericValues(input_str)
    angle_x.append(float(data[0]))
    angle_y.append(float(data[1]))
    distance.append(float(data[2]))

def ConvertAngle(input_angle):
    """Convert angle from degrees to radians."""
    return float(input_angle * (math.pi / 180))

def CalDistacetrue(distance_measure, angle_y_val):
    """Calculate the true distance using the distance and angle."""
    adj_distance = distance_measure * math.cos(ConvertAngle(angle_y_val))
    distance_true.append(adj_distance)

def CalculateAverageDistance():
    """Calculate the average true distance."""
    if distance_true:
        valid_distances = []
        for d in distance_true:
            close_values = [other for other in distance_true if abs(d - other) <= 0.6 and d != other]
            if len(close_values) > 0:
                valid_distances.append(d)
        if len(valid_distances) == 0:
            avg = -1
        else:
            avg = sum(valid_distances) / len(valid_distances)
        return avg
    return 0

def CalResultPallet(average_distance):
    """Calculate the number of pallets based on the average distance."""
    from config import current_id
    max_range = (GetMaxRange(current_id) * 1.2) + 4.2
    pallet = math.floor((max_range - average_distance) / 1.215)
    sub_pallet.append(pallet)
    return pallet

def SumPallet():
    """Sum the number of pallets in the sub_pallet list."""
    return sum(math.floor(p) for p in sub_pallet if p >= 0)

def ClearLists(*lists):
    """Clear all lists passed as arguments."""
    for l in lists:
        l.clear()

# --- API Communication ---

def GetFirstRowId(current_id):
    """Get the first ROW_ID and NumberOfPages from the REST API."""
    api_url = url + f"Location/{current_id}"
    try:
        response = requests.get(api_url)
        response.raise_for_status()
        data = response.json()
        row_id_val = data.get("rowId")
        number_of_pages = data.get("numberOfPages")
        return row_id_val, number_of_pages
    except Exception as e:
        logging.error(f"Error fetching ROW_ID and NumberOfPages from API: {e}")
        return None, None

def GetMaxRange(current_id):
    """Get the MAX_RANGE from the REST API, cache by id."""
    from config import last_max_range_id, last_max_range_value
    if last_max_range_id == current_id and last_max_range_value is not None:
        return last_max_range_value
    api_url = url + f"Location/{current_id}"
    try:
        response = requests.get(api_url)
        response.raise_for_status()
        data = response.json()
        last_max_range_id = current_id
        last_max_range_value = data.get("area")
        return last_max_range_value
    except Exception as e:
        logging.error(f"Error fetching MAX_RANGE from API: {e}")
        return None

def PostStockAndRowPallet(row_id):
    """Send ROW_ID and current date as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "Stock"
    payload = {
        "rowId": row_id,
        "pallet": 0,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload)
        response.raise_for_status()
        logging.info(f"POST stock_and_row_pallet for ROW_ID: {row_id}.")
    except Exception as e:
        logging.error(f"Error POST stock_and_row_pallet: {e}")

def PostEachPallet(row_id, sub_row_val, distance_val, angle_x_val, angle_y_val):
    """Send each pallet data as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "RowEachPallet"
    payload = {
        "rowId": row_id,
        "palletNo": sub_row_val,
        "distance": distance_val,
        "angleX": angle_x_val,
        "angleY": angle_y_val,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload)
        response.raise_for_status()
        print(response)
        logging.info(f"POST each_pallet for ROW_ID: {row_id}, SUB_ROW: {sub_row_val}.")
    except Exception as e:
        logging.error(f"Error POST each_pallet: {e}")

def PostRowPallet(row_id, sub_row_val, sub_pallet_val):
    """Send ROW_PALLET data as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "RowPallet"
    if sub_pallet_val < 0:
        sub_pallet_val = -1
    payload = {
        "rowId": row_id,
        "palletNo": sub_row_val,
        "eachPallet": sub_pallet_val,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload)
        response.raise_for_status()
        print(response)
        logging.info(f"POST ROW_PALLET for ROW_ID: {row_id}, SUB_ROW: {sub_row_val}.")
    except Exception as e:
        logging.error(f"Error POST ROW_PALLET: {e}")

def PostStock(row_id, pallets_total):
    """Send STOCK update as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "Stock"
    payload = {
        "rowId": row_id,
        "pallet": pallets_total,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload)
        response.raise_for_status()
        print(response)
        logging.info(f"POST STOCK update for ROW_ID: {row_id}.")
    except Exception as e:
        logging.error(f"Error POST STOCK update: {e}")

def DeleteRow(row_id):
    """Send delete row command as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    url = ""
    payload = {
        "type": "delete_row",
        "row_id": row_id,
        "update_date": current_date
    }
    try:
        response = requests.post(url, json=payload)
        response.raise_for_status()
        logging.info(f"POST delete row for ROW_ID: {row_id}.")
    except Exception as e:
        logging.error(f"Error POST delete row: {e}")
