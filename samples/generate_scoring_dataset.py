import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT_PATH = ROOT / "scoring-test-dataset-2026.csv"

CATEGORIES = [
    "U9",
    "U11",
    "U13",
    "U15",
    "U17",
    "U19",
    "30+",
    "40+",
    "50+",
    "60+",
    "70+",
]

FIRST_NAMES = {
    "M": ["Aaron", "Ben", "Carter", "Dylan", "Evan", "Felix", "Gavin", "Hugo", "Isaac", "Jasper"],
    "F": ["Ava", "Bella", "Chloe", "Daisy", "Ella", "Faye", "Grace", "Hannah", "Ivy", "Jade"],
}

LAST_NAMES = [
    "Adams", "Baker", "Clark", "Davis", "Evans", "Foster", "Garcia",
    "Hughes", "Irwin", "Johnson", "Kelly", "Lopez", "Mitchell", "Nguyen",
    "Owens", "Perez", "Quinn", "Reed", "Sanchez", "Turner", "Underwood",
]

PATTERNS = [
    {"label": "fastest", "obstacle": "00:43.00", "swim": "00:34.00"},
    {"label": "score_250", "obstacle": "01:05.00", "swim": "01:10.00"},
    {"label": "zero_score", "obstacle": "01:48.00", "swim": "02:15.00"},
    {"label": "random_a", "obstacle": "01:20.00", "swim": "01:26.00"},
    {"label": "random_b", "obstacle": "00:57.00", "swim": "01:01.00"},
]

LASER_REFERENCE_MS = {
    "U9": 240000,
    "U11": 240000,
    "U13": 320000,
    "U15": 460000,
    "U17": 630000,
    "U19": 800000,
    "30+": 690000,
    "40+": 690000,
    "50+": 690000,
    "60+": 420000,
    "70+": 420000,
}

SWIM_REFERENCE_MS = {
    "U9": 45000,
    "U11": 45000,
    "U13": 70000,
    "U15": 70000,
    "U17": 70000,
    "U19": 70000,
    "30+": 78000,
    "40+": 78000,
    "50+": 78000,
    "60+": 38000,
    "70+": 38000,
}

SWIM_REFERENCE_MS_F = {
    "30+": 90000,
    "40+": 90000,
    "50+": 90000,
    "60+": 43000,
    "70+": 43000,
}

LASER_TIME_OFFSETS_MS = {
    "fastest": -120000,
    "score_250": 250000,
    "zero_score": 500000,
    "random_a": 150000,
    "random_b": -60000,
}


def age_for(category: str) -> int:
    if category.startswith("U"):
        return int(category[1:])
    if category.endswith("+"):
        return int(category[:-1])
    return 18


def swim_time_for_category(category: str, gender: str, label: str) -> str:
    base_ms = SWIM_REFERENCE_MS.get(category, 70000)
    if gender.upper() == "F" and category in SWIM_REFERENCE_MS_F:
        base_ms = SWIM_REFERENCE_MS_F[category]

    if label == "score_250":
        total_seconds = max(0, int(round(base_ms / 1000)))
    elif label == "fastest":
        total_seconds = max(0, int(round((base_ms - 5000) / 1000)))
    elif label == "zero_score":
        total_seconds = max(0, int(round((base_ms + 80000) / 1000)))
    elif label == "random_a":
        total_seconds = max(0, int(round((base_ms + 25000) / 1000)))
    elif label == "random_b":
        total_seconds = max(0, int(round((base_ms - 15000) / 1000)))
    else:
        total_seconds = max(0, int(round(base_ms / 1000)))

    minutes, seconds = divmod(total_seconds, 60)
    return f"{minutes:02d}:{seconds:02d}"


def laser_time_for_category(category: str, label: str) -> str:
    time_ms = LASER_REFERENCE_MS.get(category, 800000) + LASER_TIME_OFFSETS_MS[label]
    total_seconds = max(0, int(round(time_ms / 1000)))
    minutes, seconds = divmod(total_seconds, 60)
    return f"{minutes:02d}:{seconds:02d}"


def build_rows():
    rows = []
    bib = 2001
    for category in CATEGORIES:
        for pattern_index, pattern in enumerate(PATTERNS):
            gender = "M" if pattern_index % 2 == 0 else "F"
            first = FIRST_NAMES[gender][(pattern_index + age_for(category)) % len(FIRST_NAMES[gender])]
            last = LAST_NAMES[(age_for(category) + pattern_index + len(category)) % len(LAST_NAMES)]
            affiliation = ["North", "South", "East", "West", "Central"][pattern_index % 5]
            dob = "2010-01-01"
            if category.startswith("U"):
                age_years = 18 - age_for(category)
                dob = f"20{(age_years + 7):02d}-0{(pattern_index % 6) + 1:1d}-10"
            else:
                base_year = 2000 - age_for(category)
                dob = f"{base_year + pattern_index}-05-15"

            rows.append({
                "bib": bib,
                "first_name": first,
                "last_name": last,
                "gender": gender,
                "category": category,
                "affiliation": f"Club-{affiliation}",
                "dob": dob,
                "ocr_time": pattern["obstacle"],
                "swim_time": swim_time_for_category(category, gender, pattern["label"]),
                "laser_run_time": laser_time_for_category(category, pattern["label"]),
            })
            bib += 1
    return rows


def main():
    rows = build_rows()
    fields = [
        "bib",
        "first_name",
        "last_name",
        "gender",
        "category",
        "affiliation",
        "dob",
        "ocr_time",
        "swim_time",
        "laser_run_time",
    ]
    with OUT_PATH.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Wrote {len(rows)} competitor rows to {OUT_PATH}")


if __name__ == "__main__":
    main()
