from app.imports import *
from app.helpers.utils.general.time import *


def totalMinutes(report):
    reportByUser = {}
    for entry in report:
        if entry["username"] in reportByUser:
            reportByUser[entry["username"]].append(entry)
        else:
            reportByUser[entry["username"]] = [entry]
    totalMinutes = 0
    for userReport in reportByUser.values():
        userMinutes = loginsToMinutes(userReport)
        for userMinutesEntry in userMinutes:
            totalMinutes += userMinutesEntry["minutes"]
    return totalMinutes


def loginsToMinutes(report):
    """Turns an array of login history to an array of time online per day

    Args:
        report (arr[dict]): Report entries from login_history sql table, MUST be sorted in ascending order

    Returns:
        arr[dict]: The output array
    """
    index = 1
    output = []
    minutesOnline = 0

    while report is not None and index < len(report):
        earlyTime = dt.strptime(report[index - 1]["timestamp"], "%m-%d-%Y, %H:%M:%S")
        lateTime = dt.strptime(report[index]["timestamp"], "%m-%d-%Y, %H:%M:%S")

        if earlyTime.date() == lateTime.date():  # Same day
            if report[index]["action"] != report[index - 1]["action"]:
                if report[index]["action"] == "logoff":
                    deltaTime = lateTime - earlyTime
                    minutesOnline += deltaTime.seconds / 60
            else:
                if report[index]["action"] == "logon":
                    deltaTime = lateTime - earlyTime
                    minutesOnline += deltaTime.seconds / 60
        else:
            if (
                report[index - 1]["action"] == "logon"
            ):  # User session continued to next day
                midnight = dt.combine(lateTime.date(), dt.min.time())
                deltaTime = midnight - earlyTime
                minutesOnline += deltaTime.seconds / 60
            if minutesOnline > 1:
                output.append(
                    {
                        "timestamp": int(dt.timestamp(earlyTime)),
                        "timestamp_datetime": unixToDate(int(dt.timestamp(earlyTime))),
                        "minutes": int(minutesOnline),
                    }
                )
            minutesOnline = 0
            if (
                report[index - 1]["action"] == "logon"
            ):  # User session continued to next day
                midnight = dt.combine(lateTime.date(), dt.min.time())
                deltaTime = lateTime - midnight
                minutesOnline += deltaTime.seconds / 60

        if index == len(report) - 1:  # Last entry
            if report[index]["action"] == "logon":
                deltaTime = dt.now() - lateTime
                minutesOnline += deltaTime.seconds / 60
            if minutesOnline > 1:
                output.append(
                    {
                        "timestamp": int(dt.timestamp(earlyTime)),
                        "timestamp_datetime": unixToDate(int(dt.timestamp(earlyTime))),
                        "minutes": int(minutesOnline),
                    }
                )
        index += 1
    return output


def extractFeature(feature_name, cleaned_df, scale):
    transformed_df = cleaned_df[
        cleaned_df.contents.str.contains(feature_name, na=False, regex=False)
    ]
    transformed_df["contents"] = [
        float(content.split(": ")[1]) for content in transformed_df["contents"]
    ]
    transformed_df["time"] = [
        time_str.strftime("%H:%M:%S.%f")[:-3] for time_str in transformed_df["time"]
    ]
    transformed_df.loc[:, ["contents", "time"]]

    if transformed_df.shape[0] > 1:
        feature_std = np.std(transformed_df["contents"])
        feature_median = np.median(transformed_df["contents"])
        feature_mean = np.mean(transformed_df["contents"])
        feature_range = [
            min(transformed_df["contents"]),
            max(transformed_df["contents"]),
        ]

        return {
            "summary_statistics": {
                "mean": feature_mean,
                "median": feature_median,
                "standard_deviation": feature_std,
                "range": feature_range,
            },
            "output": [
                {
                    "time": transformed_df.iloc[
                        i, transformed_df.columns.get_loc("time")
                    ],
                    "value": transformed_df.iloc[
                        i, transformed_df.columns.get_loc("contents")
                    ]
                    * scale,
                }
                for i in range(0, transformed_df.shape[0])
            ],
        }
    else:
        return {
            "summary_statistics": {
                "mean": None,
                "median": None,
                "standard_deviation": None,
                "range": None,
            },
            "output": [],
        }
