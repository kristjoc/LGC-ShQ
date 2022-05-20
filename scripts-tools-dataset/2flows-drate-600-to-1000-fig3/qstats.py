import os
import sys
import errno
import time
import datetime
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import seaborn as sns
sns.set()

def return_qstats(fname='', which=''):

    # Read csv file
    if (which == 'qlen'):
        imported_data = np.genfromtxt(fname, delimiter=',', usecols=1)
        # df = pd.read_csv(fname, header=None, sep=',', usecols=[1])
        # print(df.to_string())
        for i in imported_data:
            print(i)
        return
    elif (which == 'qdelay'):
        imported_data = np.genfromtxt(fname, delimiter=',', usecols=2)
        # df = pd.read_csv(fname, header=None, sep=',', usecols=[1])
        # print(df.to_string())
        for i in imported_data:
            print(i)
        # df = pd.read_csv(fname, header=None, sep=',', usecols=[2])
        # print(df.to_string())
        return
    elif (which == 'tput'):
        df = pd.read_csv(fname, header=None, usecols=[0])
        mean = df.mean().to_string()	# mean
        sem = df.std().to_string()	# standard error of the mean
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
    mean = df.quantile(0.9).to_string()	# mean
    # mean = df.mean().to_string()	# mean
    sem = df.sem().to_string()	# standard error of the mean
    # print(df.std().to_string())	# standard deviation

    print(mean.split()[1] + ' ' + sem.split()[1])


if __name__ == '__main__':
    filename = sys.argv[2]
    which = sys.argv[1]
    return_qstats(filename, which)
