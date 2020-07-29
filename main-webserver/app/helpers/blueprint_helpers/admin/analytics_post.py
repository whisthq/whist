from app import *
from app.helpers.utils.general.analytics import *


def analyticsLogsHelper(body):
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

    # Read and clean logs into Pandas dataframe

    try:
        df = pd.read_csv(body["filename"], sep="|", header=None, error_bad_lines=False)
        df.columns = ["time", "level", "file", "location", "contents"]
        r = df.time.apply(
            lambda x: ":".join(str(x).split(":")[:-1]) + "." + str(x).split(":")[-1]
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
    error_df = cleaned_df[cleaned_df.level.str.contains("ERROR", na=False, regex=False)]
    number_of_errors = error_df.shape[0]
    error_rate = float(number_of_errors) / float(cleaned_df.shape[0])

    if body["sender"].upper() == "SERVER":
        encode_time_stats = extractFeature("Average Encode Time", cleaned_df, 1000)
        encode_size_stats = extractFeature("Average Encode Size", cleaned_df, 1)

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
        decode_time_stats = extractFeature("Avg Decode Time", cleaned_df, 1000)
        latency_stats = extractFeature("Latency", cleaned_df, 1000)

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
