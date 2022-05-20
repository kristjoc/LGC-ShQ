#!/usr/bin/python3

import sys
import numpy as np
import pandas as pd


def return_qstats(fname='', which=''):

    # Read csv file
    if (which == 'qlen'):
        imported_data = np.genfromtxt(fname, delimiter=',', usecols=1)
        # df = pd.read_csv(fname, header=None, sep=',', usecols=[1])
        # print(df.to_string())
        for i in imported_data:
            if ('shq' in fname):
                if (i < 250):
                    print(i)
            else:
                print(i)
        return
    elif (which == 'qdelay'):
        imported_data = np.genfromtxt(fname, delimiter=',', usecols=2)
        # df = pd.read_csv(fname, header=None, sep=',', usecols=[1])
        # print(df.to_string())
        for i in imported_data:
            if ('shq' in fname):
                if (i < 10.2):
                    print(i)
            else:
                print(i)
        # df = pd.read_csv(fname, header=None, sep=',', usecols=[2])
        # print(df.to_string())
        return
    elif (which == 'tput'):
        df = pd.read_csv(fname, header=None, usecols=[0])
        mean = df.mean().to_string()  # mean
        sem = df.std().to_string()  # standard error of the mean
        print(mean.split()[1] + ' ' + sem.split()[1])
        return
    elif (which == 'mean'):
        df = pd.read_csv(fname, header=None, usecols=[0])
    elif (which == 'sum'):
        df = pd.read_csv(fname, header=None, usecols=[0])
        sum = df.sum().to_string()
        print(sum.split()[1])
        return

    # print(df.quantile(0.5).to_string()) # 50th percentile = median
    # print(df.quantile(0.9).to_string()) # 90th percentile
    mean = df.quantile(0.95).to_string()  # mean
    # mean = df.mean().to_string()	# mean
    sem = df.sem().to_string()  # standard error of the mean
    # print(df.std().to_string())	# standard deviation

    print(mean.split()[1] + ' ' + sem.split()[1])


if __name__ == '__main__':
    filename = sys.argv[2]
    which = sys.argv[1]
    return_qstats(filename, which)
