import os
import sys
import errno
import time
import datetime
import numpy as np
import pandas as pd
import matplotlib
from matplotlib import container
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def plot_over(fname='', pdf_dir='', out_file=''):

    imported_data = np.genfromtxt(fname)
    labels = imported_data[:, 0]
    hull = imported_data[:, 1]
    hull_sem = imported_data[:, 2]
    lgc = imported_data[:, 3]
    lgc_sem = imported_data[:, 4]

    if 'tput' in fname:
        ideal = imported_data[:, 5]
        ideal_sem = imported_data[:, 6]
        hull_paper = imported_data[:,7]

    if 'drate' in fname:
        x = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8])
        # x = np.array([0, 1, 2, 3, 4, 5])
        my_xticks = ['600', '650', '700', '750', '800', '850', '900', '950', '1000']
        # my_xticks = ['950', '960', '970', '980', '990', '1000']
    elif 'nflows' in fname:
        x = np.array([0, 1, 2, 3, 4])
        my_xticks = ['2', '4', '6', '8', '10']
    
    fig, ax = plt.subplots()

    plt.xticks(x, my_xticks)
    plt.xticks(fontsize=16)
    plt.yticks(fontsize=16)

    if 'tput' in fname:
        ax.set_ylim(bottom=0, top=1100)
    
    if 'qlen' in fname:
        plt.ylabel('Queue size [pkts.]', fontsize=22)
    elif 'qdelay' in fname:
        plt.ylabel('Queuing delay [μs]', fontsize=22)
    elif 'tput' in fname:
        plt.ylabel('Throughput [Mbps]', fontsize=22)

    if 'drate' in fname:
        plt.xlabel('Target rate [Mbps]', fontsize=22)
    elif 'drate' in fname:
        plt.xlabel('Number of Static Flows', fontsize=22)

    plt.grid(True, which='major', lw=0.65, ls='--', dashes=(3, 7), zorder=0)
    
    # plot main ideal line
    if 'tput' in fname:
        # xy=(2, 970)
        ax.annotate('Ideal', xy=(4, 800),  xycoords='data', xytext=(3, 950),
                    fontsize=20, arrowprops=dict(facecolor='black', headwidth=6.,
                                                 width=1., headlength=7.),
                    horizontalalignment='right', verticalalignment='top')
        plt.errorbar(x,
                     ideal,
                     yerr=ideal_sem,
                     # ls='--',
                     dashes=[3, 3],
                     # marker='x',
                     # markersize=4,
                     linewidth=3,
                     color='dimgray',
                     # label='Ideal',
                     zorder=30)
        
        plt.errorbar(x,
                     hull_paper,
                     ls='dashdot',
                     # dashes=[2,2],
                     # marker='x',
                     # markersize=4,
                     linewidth=3,
                     color='tab:orange',
                     label='DCTCP-HULL (original)',
                     zorder=30)

    if ('tput' in fname):
        clabel = 'DCTCP-HULL (our implementation)'
    else:
        clabel = 'DCTCP-HULL'

    plt.errorbar(x,
                 hull,
                 yerr=hull_sem,
                 # ls='--',
                 dashes=[4,1,2,1],
                 # marker='x',
                 # markersize=4,
                 linewidth=3,
                 color='firebrick',
                 label=clabel,
                 zorder=40)
    
    plt.errorbar(x,
                 lgc,
                 yerr=lgc_sem,
                 # ls='--',
                 dashes=[4.5,3],
                 # marker='x',
                 # markersize=4,
                 linewidth=3,
                 color='green',
                 label='LGC-ShQ',
                 zorder=50)

    ax = plt.gca()
    handles, labels = ax.get_legend_handles_labels()
    handles = [h[0] if isinstance(h, container.ErrorbarContainer) else h for h in handles]

    ax.legend(handles, labels)

    if 'tput' in fname:
        ax.legend(handles, labels, fontsize=17, loc='lower right', frameon=False)
    else:
        ax.legend(handles, labels, fontsize=18, loc='upper left', frameon=False)

    # Hide the right and top spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    # Only show ticks on the left and bottom spines
    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')
    
    plt.margins(x=0.02)
    plt.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)
    plt.savefig(pdf_dir + '/' + out_file + '.pdf',
                format='pdf',
                dpi=1200,
                bbox_inches='tight',
                pad_inches=0.025,
                transparent=True)


if __name__ == '__main__':
    filename = sys.argv[1]

    plot_over(filename, '.', filename)
