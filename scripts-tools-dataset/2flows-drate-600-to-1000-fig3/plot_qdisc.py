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

def plot_qlen_stats(fname='', yaxis='', pdf_dir='', out_file=''):

    # Read csv file
    df = pd.read_csv(fname, header=None, sep=',', usecols=[0, 1])
    # Set as index the first column - Timestamps
    df.set_index(0, inplace=True)
    # Relative time - Start Time from 0
    df.index = [idx - df.index[0] for idx in df.index]

    # Plot graph
    ax = df.plot(figsize=(20,10), linewidth=1, fontsize=20, grid=True)

    # sns.set_style("ticks", {'grid.linestyle': '--'})
    plt.locator_params(axis='x', nbins=10)
    ax.set_xlabel('Time (s)', fontsize=20)
    ax.set_ylabel(yaxis, fontsize=20)

    #plt.legend(fontsize=20, loc='best')
    ax.get_legend().remove()
    plt.margins(x=0.02)
    plt.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)
    plt.savefig(pdf_dir + '/' + out_file + '_qlen.pdf',
                format='pdf',
                dpi=1200,
                bbox_inches='tight',
                pad_inches=0.025)


def plot_qdelay_stats(fname='', yaxis='', pdf_dir='', out_file=''):

    # Read csv file
    df = pd.read_csv(fname, header=None, sep=',', usecols=[0, 2])
    # Set as index the first column - Timestamps
    df.set_index(0, inplace=True)
    # Relative time - Start Time from 0
    df.index = [idx - df.index[0] for idx in df.index]

    # Plot graph
    ax = df.plot(figsize=(20,10), linewidth=1, fontsize=20, grid=True)

    # sns.set_style("ticks", {'grid.linestyle': '--'})
    plt.locator_params(axis='x', nbins=10)
    ax.set_xlabel('Time (s)', fontsize=20)
    ax.set_ylabel(yaxis, fontsize=20)

    #plt.legend(fontsize=20, loc='best')
    ax.get_legend().remove()
    plt.margins(x=0.02)
    plt.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)
    plt.savefig(pdf_dir + '/' + out_file + '_qdelay.pdf',
                format='pdf',
                dpi=1200,
                bbox_inches='tight',
                pad_inches=0.025)


if __name__ == '__main__':
    filename = sys.argv[1]
    plot_qlen_stats(filename, 'Queue length', '.', filename)
    plot_qdelay_stats(filename, 'Queuing delay', '.', filename)
