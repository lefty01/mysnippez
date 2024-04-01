#!/usr/bin/env python3


import datetime
import sys
import re

import gpx_parser

import plotly.express as px
import numpy as np
import pandas as pd
from gpx_converter import Converter

import argparse

thisYear = datetime.datetime.now().year

parser = argparse.ArgumentParser(description='Parse gpx file(s)/filenames from stdin and get summary info. Open map in browser.')

parser.add_argument('-L', '--max-length', type=float, default=9999,
                    help='only show tracks less/equal to the max. distance (km)')
parser.add_argument('-l', '--min-length', type=float, default=0,
                    help='only show tracks having at least this min. distance (km)')
parser.add_argument('-y', '--year',  type=int, nargs='*',
                         help='filter this year(s)')
parser.add_argument('-m', '--month',  type=int, nargs='*',
                         help='filter this month(s)')
parser.add_argument('-d', '--day',  type=int, nargs='*',
                         help='filter this day(s)')
parser.add_argument('-W', '--day-of-week',  type=int, nargs='*', default=list(range(0, 7)),
                         help='filter day of week, 0=Mon..6=Sun (eg. --dow 0 1 2 3 4 for week days)')

# parser.add_argument('--parse-gpx-time') ... or parse from filename!?

parser.add_argument('-s', '--show-map', action='store_true', help='show map in browser (for all tracks, each in own tab)')


parser.add_argument('-v', '--verbose', action='count', default=0, help='Enable verbose output.')

args = parser.parse_args()
# print(args.accumulate(args.integers))
print(f"args: -d: {args.max_length}, -D: {args.min_length}, -y: {args.year}, -s: {args.show_map}, --dow: {args.day_of_week}")
# defaults: args: -d: 10.0, -D: 5.0, -y: 2024, -s: False


day2str = {
    0: "Mon",
    1: "Tue",
    2: "Wed",
    3: "Thu",
    4: "Fri",
    5: "Sat",
    6: "Sun"
}

numMatches = 0

# /media/mycloud/backup/old_exitzero_www/exitzero.de/html/tracks/2012-04-19_18-26-02-80-13932.tcx.                 20120419100027.gpx
# /media/mycloud/backup/old_exitzero_www/exitzero.de/html/tracks/forerunner305-training-center-2009-03-29-1217.xml.20090327104631.gpx

for line in sys.stdin:
    filename = line.strip()
    m = re.match(r'.*\.(\d{4,4})(\d{2,2})(\d{2,2})\d{6,6}\.gpx', filename)
    if not m:
        continue

    year     = int(m.group(1))
    month    = int(m.group(2))
    day      = int(m.group(3))
    whichDay = datetime.date(year, int(m.group(2)), int(m.group(3))).weekday()
    dayName  = day2str[whichDay]
    skipDay  = whichDay not in args.day_of_week
    
    # check if we only look at specific day(s) of week
    if skipDay:
        continue
    # check if we only look for specific year(s)
    if args.year != None and year not in args.year:
        continue
    if args.month != None and month not in args.month:
        continue
    if args.month != None and month not in args.month:
        continue
    

    with open(filename, 'r') as gpx_file:
        gpx = gpx_parser.parse(gpx_file)  # todo: error handling

    # fixme: here we always expect single track gpx (which is usually the case for recorded tracks)
    track      = gpx[0]
    seg        = gpx[0][0]              # track[0]
    firstPoint = gpx[0][0][0]           # seg[0]
    trackDist  = gpx.length_2d()/1000   # d.dd km
    trackDistStr = f"{trackDist:.2f}"   # as string

    if trackDist > args.max_length or trackDist < args.min_length:
        if args.verbose:
            print("skipping ... no matching distance")
        continue

    print(f"file: {filename}")
    print(f"length: {gpx.length_2d()/1000:.2f} ({gpx[0].get_points_no()} pts)")
    print(f"start time:  {dayName}, {firstPoint.time}")
    #t1 = datetime.datetime.strptime(firstPoint.time[:-1], "%Y-%m-%dT%H:%M:%S")
    #print(t1)

    numMatches += 1


    if not args.show_map:
        continue

    # time   latitude  longitude  altitude
    df = Converter(input_file=filename).gpx_to_dataframe()
    if args.verbose > 2:
        print(df)

    if firstPoint.time != None:
        fig = px.scatter_mapbox(
            df,
            lat="latitude",
            lon="longitude",
            hover_name="time",
            zoom=12
        )
    else:
        fig = px.scatter_mapbox(
            df,
            lat="latitude",
            lon="longitude",
            zoom=12
        )
    # fixme: can we update hover_name below ... because of some trackpoints will not have time
    fig.update_layout(mapbox_style="open-street-map")
    fig.update_layout(margin=dict(r=0, t=0, l=0, b=0))
    #fig.add_traces()


    fig.show()



# copy to web folder:
# scp filename exitzero.de

print(f"found {numMatches} matches")
