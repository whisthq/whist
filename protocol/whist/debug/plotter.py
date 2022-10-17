#!/usr/bin/env python3

import json
from optparse import OptionParser
import matplotlib.pyplot as plt
import re
import signal

# allow to kill this program with ctrl-c
signal.signal(signal.SIGINT, signal.SIG_DFL)

# define the options
parser = OptionParser(usage="usage: %prog [options] filename1 [filename2 ...]")
parser.add_option(
    "-s", "--scatter", action="store_true", dest="use_scatter", help="draw scatter instead of line"
)
parser.add_option(
    "-d",
    "--distribution",
    action="store_true",
    dest="draw_distribution",
    help="draw distribution of y, instead of the x-y plot",
)
parser.add_option(
    "-f",
    "--filter",
    action="store",
    dest="filter_pattern",
    type="string",
    help="regex expression to filter label names",
)

parser.add_option(
    "-v",
    "--inverse-match",
    action="store_true",
    dest="inverse_match",
    help="when used together with -f, do inverse match",
)

parser.add_option(
    "-r",
    "--range",
    action="store",
    dest="range_x",
    type="string",
    help='range of x, e.g. "10.0~15.0"',
)

parser.add_option(
    "",
    "--figsize",
    action="store",
    dest="figsize",
    type="string",
    help='size of figure, e.g. "16x8"',
)

parser.add_option(
    "-w",
    "--weight",
    action="store",
    type="float",
    default=0.5,
    help="weight of the line or point, default: 0.5",
)

parser.add_option(
    "-t",
    "--tail",
    type="int",
    action="store",
    dest="tail_n",
    default=0,
    help="parse only last n lines, only for stream format",
)

# parse options by the optparse lib
(options, args) = parser.parse_args()

# print(options)

# further manual parsing of the range option
range_x = options.range_x
range_x_min = 0
range_x_max = 0
if range_x:
    if len(range_x.split("~")) != 2:
        parser.error("invalid format: %s" % (rang_x))
    range_x_min = float(range_x.split("~")[0])
    range_x_max = float(range_x.split("~")[1])

figsize = options.figsize
figsize_x = -1
figsize_y = -1
if figsize:
    if len(figsize.split("x")) != 2:
        parser.error("invalid format: %s" % (figsize))
    figsize_x = int(figsize.split("x")[0])
    figsize_y = int(figsize.split("x")[1])


# need at least 1 input file to plot
if len(args) < 1:
    parser.error("need input file")


# draw a collection of dataset with specific labels
def draw(label, arr):
    x = []
    y = []
    # handle the `-r` option
    for i in arr:
        if range_x:
            if i[0] < range_x_min or i[0] > range_x_max:
                continue
        x.append(i[0])
        y.append(i[1])

    # handle the -d option
    if options.draw_distribution:
        # y values are sorted and x values are modified to percentage
        y.sort()
        if len(x) == 1:
            x[0] = 0.0
        else:
            l = len(x)
            for i in range(0, l):
                x[i] = i / (l - 1) * 100

    # use scatter() or plot() depend on the option
    if options.use_scatter:
        plt.scatter(x, y, label=label, s=options.weight)
    else:
        plt.plot(x, y, label=label, linewidth=options.weight)


# load data from file
def load_from_file(file_name):
    s = open(file_name).read()
    if len(s) < 2:
        print("invalid file:", file_name)
        os.exit(-1)
    # if it's json with pretty print, the first line
    # must be  "{\n".
    if s[0] == "{" and s[1] == "\n":
        # load whole content as json
        data = json.loads(s)
        return data
    else:  # otherwise we try to parse it as jsonline
        lines = s.split("\n")
        # each completed line should finish with a '\n'
        # if there are <= 1 line, then no data to parse
        if len(lines) <= 1:
            print("no valid data")
            os.exit(-1)
        lines = lines[0:-1]
        if options.tail_n > 0:  # support the --tail option
            lines = lines[-options.tail_n :]
        data = {}
        # we merge all data in each line of jsonline
        # into a single structure
        for i in range(0, len(lines)):
            tmp_data = json.loads(lines[i])
            for label in tmp_data.keys():
                if not label in data:
                    data[label] = []
                data[label].extend(tmp_data[label])
        return data


if figsize_x > 0 and figsize_y > 0:
    plt.figure(figsize=(figsize_x, figsize_y))

# handle input of multiple files
for file_name in args:
    # read in the file
    s = open(file_name).read()
    data = load_from_file(file_name)
    # handle each lables in one file
    for i in data.keys():
        if options.filter_pattern:
            m = re.match(options.filter_pattern, str(i))
            if options.inverse_match and m:
                continue
            elif not options.inverse_match and not m:
                continue
        label = i
        # if there are more than one file, append the file name to the label to indicate which file the lable comes from
        if len(args) > 1:
            label = file_name.split("/")[-1] + " : " + i
        # print out how many sampels are contained in the file
        print("%s has %d samples" % (i, len(data[i])))
        # call the draw logical
        draw(label, data[i])

# print out description of lables
plt.legend()

# depend on the mode, print xlabel
if options.draw_distribution:
    plt.xlabel("percentage rank")
else:
    plt.xlabel("x value")

# print y label
plt.ylabel("y value")

# show the plot
plt.show()
