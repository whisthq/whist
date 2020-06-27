from app import *

from app.logger import *

report_bp = Blueprint("report_bp", __name__)


@report_bp.route("/report/<action>", methods=["GET", "POST"])
def statusReport(action, **kwargs):
    if action == "latest" and request.method == "GET":
        command = text(
            """
        SELECT * 
        FROM status_report
        ORDER BY timestamp DESC
        """
        )
        params = {}
        try:
            with engine.connect() as conn:
                latest = cleanFetchedSQL(conn.execute(command, **params).fetchone())
                return jsonify(latest), 200
        except:
            traceback.print_exc()
    elif action == "regionReport" and request.method == "POST":
        body = request.get_json()
        command = text("")
        if body["timescale"] == "day":
            command = text(
                """
            SELECT timestamp, users_online, eastus_unavailable, northcentralus_unavailable, southcentralus_unavailable
            FROM status_report
            ORDER BY timestamp DESC
            LIMIT 24
            """
            )
        elif body["timescale"] == "week":
            command = text(
                """
            SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
            FROM status_report
            GROUP BY 1
            ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
            LIMIT 7
            """
            )
        elif body["timescale"] == "month":
            command = text(
                """
            SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
            FROM status_report
            GROUP BY 1
            ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
            LIMIT 30
            """
            )
        params = {}
        try:
            with engine.connect() as conn:
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                if body["timescale"] == "week" or body["timescale"] == "month":
                    for row in report:
                        row["timestamp"] = int(row["timestamp"].timestamp())

                return jsonify(report), 200
        except:
            traceback.print_exc()
    elif action == "userReport" and request.method == "POST":
        body = request.get_json()
        today = dt.now()
        command = text(
            """
            SELECT *
            FROM login_history
            WHERE "username" = :username AND timestamp > :date AND is_user = true
            ORDER BY timestamp ASC
            """
        )
        params = {}
        if body["timescale"] == "day":
            command = text(
                """
                """
            )
        elif body["timescale"] == "week":
            lastWeek = today - datetime.timedelta(days=7)
            params = {
                "username": body["username"],
                "date": lastWeek.strftime("%m-%d-%y"),
            }
        elif body["timescale"] == "month":
            lastMonth = today - datetime.timedelta(days=30)
            print(lastMonth.strftime("%m-%d-%y"))
            params = {
                "username": body["username"],
                "date": lastMonth.strftime("%m-%d-%y"),
            }
        elif body["timescale"] == "beginningMonth":
            beginning_month = dt.strptime(
                "{year}-{month}-{day}".format(
                    year=today.year, month=today.month, day="1"
                ),
                "%Y-%m-%d",
            ).strftime("%m-%d-%y")
            params = {
                "username": body["username"],
                "date": beginning_month,
            }
        else:
            return jsonify({}), 404

        if "start_date" in body.keys():
            params = {
                "username": body["username"],
                "date": unixToDate(body["start_date"]).strftime("%m-%d-%y"),
            }

        try:
            with engine.connect() as conn:
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                output = []
                if (
                    body["timescale"] == "week"
                    or body["timescale"] == "month"
                    or body["timescale"] == "beginningMonth"
                    or "start_date" in body.keys()
                ):
                    output = loginsToMinutes(report)

                return jsonify(output), 200
        except:
            traceback.print_exc()
            return jsonify({}), 500
    elif action == "totalUsage" and request.method == "GET":
        today = dt.now()
        command = text(
            """
            SELECT *
            FROM login_history
            WHERE timestamp > :date AND is_user = true
            ORDER BY timestamp ASC
            """
        )
        try:
            with engine.connect() as conn:
                params = {
                    "date": dt.combine(today.date(), dt.min.time()).strftime(
                        "%m-%d-%y"
                    ),
                }
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                dayMins = totalMinutes(report)

                params = {
                    "date": (today - datetime.timedelta(days=7)).strftime("%m-%d-%y"),
                }
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                weekMins = totalMinutes(report)

                params = {
                    "date": (today - datetime.timedelta(days=30)).strftime("%m-%d-%y"),
                }
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                monthMins = totalMinutes(report)

                return (
                    jsonify({"day": dayMins, "week": weekMins, "month": monthMins}),
                    200,
                )
        except:
            traceback.print_exc()
            return jsonify({}), 500
    elif action == "signups" and request.method == "GET":
        today = dt.now()
        command = text(
            """
                SELECT COUNT(created)
                FROM users
                WHERE created > :timestamp
            """
        )
        try:
            with engine.connect() as conn:
                params = {
                    "timestamp": dt.combine(today.date(), dt.min.time()).timestamp(),
                }
                dayCount = cleanFetchedSQL(conn.execute(command, **params).fetchall())[
                    0
                ]["count"]
                params = {
                    "timestamp": (today - datetime.timedelta(days=7)).timestamp(),
                }
                weekCount = cleanFetchedSQL(conn.execute(command, **params).fetchall())[
                    0
                ]["count"]
                params = {
                    "timestamp": (today - datetime.timedelta(days=30)).timestamp()
                }
                monthCount = cleanFetchedSQL(
                    conn.execute(command, **params).fetchall()
                )[0]["count"]
                return (
                    jsonify({"day": dayCount, "week": weekCount, "month": monthCount}),
                    200,
                )
        except:
            traceback.print_exc()
            return jsonify({}), 500
    return jsonify({}), 404


@report_bp.route("/analytics/<action>", methods=["POST"])
def analytics(action, **kwargs):
    if action == "logs":

        def extractFeature(feature_name, cleaned_df):
            transformed_df = cleaned_df[
                cleaned_df.contents.str.contains(feature_name, na=False, regex=False)
            ]
            transformed_df["contents"] = [
                float(content.split(": ")[1]) for content in transformed_df["contents"]
            ]
            transformed_df["time"] = [
                time_str.strftime("%H:%M:%S.%f")[:-3]
                for time_str in transformed_df["time"]
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
                            * 1000,
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

        body = json.loads(request.data)

        # Read and clean logs into Pandas dataframe

        try:
            df = pd.read_csv(
                body["filename"], sep="|", header=None, error_bad_lines=False
            )
            df.columns = ["time", "level", "file", "location", "contents"]
            r = df.time.apply(
                lambda x: ":".join(x.split(":")[:-1]) + "." + x.split(":")[-1]
            )
            df.time = pd.to_datetime(r, errors="coerce")
            cleaned_df = df[df.time.notnull()]
        except Exception as e:
            print("Error reading {filename}".format(filename=body["filename"]))
            return (
                jsonify({}),
                400,
            )

        # Get number of errors
        error_df = cleaned_df[
            cleaned_df.level.str.contains("ERROR", na=False, regex=False)
        ]
        number_of_errors = error_df.shape[0]
        error_rate = float(number_of_errors) / float(cleaned_df.shape[0])

        if body["sender"].upper() == "SERVER":
            encode_time_stats = extractFeature("Average Encode Time", cleaned_df)
            encode_size_stats = extractFeature("Average Encode Size", cleaned_df)

            return (
                jsonify(
                    {
                        "debug": {
                            "errors": list(error_df["contents"]),
                            "number_of_errors": number_of_errors,
                            "error_rate": error_rate,
                        },
                        "encode_time": encode_time_stats,
                        "encode_size": encode_size_stats,
                    }
                ),
                200,
            )
        else:
            decode_time_stats = extractFeature("Avg Decode Time", cleaned_df)
            latency_stats = extractFeature("Latency", cleaned_df)

            return (
                jsonify(
                    {
                        "debug": {
                            "errors": list(error_df["contents"]),
                            "number_of_errors": number_of_errors,
                            "error_rate": error_rate,
                        },
                        "decode_time": decode_time_stats,
                        "latency": latency_stats,
                    }
                ),
                200,
            )


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
