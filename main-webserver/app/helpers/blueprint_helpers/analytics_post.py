from app import *
from app.helpers.utils.general.analytics import *


def logsHelper(filename, sender):
    # Read and clean logs into Pandas dataframe
    try:
        df = pd.read_csv(filename, sep="|", header=None, error_bad_lines=False)
        df.columns = ["time", "level", "file", "location", "contents"]
        r = df.time.apply(
            lambda x: ":".join(str(x).split(":")[:-1]) + "." + str(x).split(":")[-1]
        )
        df.time = pd.to_datetime(r, errors="coerce")
        cleaned_df = df[df.time.notnull()]
    except Exception as e:
        print("Error reading {filename}".format(filename=filename))
        return (
            jsonify({}),
            400,
        )

    # Get number of errors
    error_df = cleaned_df[cleaned_df.level.str.contains("ERROR", na=False, regex=False)]
    number_of_errors = error_df.shape[0]
    error_rate = float(number_of_errors) / float(cleaned_df.shape[0])

    if sender.upper() == "SERVER":
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
