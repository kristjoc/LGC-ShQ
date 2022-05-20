#!/usr/bin/python3

import matplotlib.pyplot as plt
from matplotlib import container
import sys
import numpy as np


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

    if 'drate' in fname:
        # x = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8])
        x = np.array([0, 1, 2, 3, 4, 5])
        # my_xticks = ['600', '650', '700', '750', '800', '850', '900', '950', '1000']
        my_xticks = ['950', '960', '970', '980', '990', '1000']
    elif 'nflows' in fname:
        x = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8])
        # x = np.array([0, 1, 2, 3, 4])
        my_xticks = ['2', '3', '4', '5', '6', '7', '8', '9', '10']
        # my_xticks = ['2', '4', '6', '8', '10']
    
    fig, ax = plt.subplots()

    plt.xticks(x, my_xticks)
    plt.xticks(fontsize=16)
    plt.yticks(fontsize=16)

    if 'tput' in fname:
        ax.set_ylim(bottom=400, top=1010)
    
    if 'qlen' in fname:
        plt.ylabel('Queue size [pkts.]', fontsize=22)
    elif 'qdelay' in fname:
        plt.ylabel('Queuing delay [μs]', fontsize=22)
    elif 'tput' in fname:
        plt.ylabel('Throughput [Mbps]', fontsize=22)

    if 'drate' in fname:
        plt.xlabel('Drain rate [Mbps]', fontsize=22)
    elif 'nflows' in fname:
        plt.xlabel('Number of Static Flows', fontsize=22)

    plt.grid(True, which='major', lw=0.65, ls='--', dashes=(3, 7), zorder=0)
    
    # plot main ideal line
    # if 'tput' in fname:
    #     # xy=(4, 800)
    #     ax.annotate('Ideal', xy=(2, 970),  xycoords='data', xytext=(1, 1100),
    #                 fontsize=18, arrowprops=dict(facecolor='black', shrink=0.05),
    #                 horizontalalignment='left', verticalalignment='top')
    #     plt.errorbar(x,
    #                  ideal,
    #                  yerr=ideal_sem,
    #                  # ls='--',
    #                  dashes=[3, 3],
    #                  marker='x',
    #                  markersize=4,
    #                  linewidth=2,
    #                  color='dimgray',
    #                  # label='Ideal',
    #                  zorder=30)

    plt.errorbar(x,
                 hull,
                 yerr=hull_sem,
                 # ls='--',
                 dashes=[4,1,2,1],
                 linewidth=3,
                 color='firebrick',
                 label='DCTCP-HULL',
                 zorder=30)
    
    plt.errorbar(x,
                 lgc,
                 yerr=lgc_sem,
                 # ls='--',
                 dashes=[4.5,3,4.5,3],
                 linewidth=3,
                 color='green',
                 label='LGC-ShQ',
                 zorder=30)

    ax = plt.gca()
    handles, labels = ax.get_legend_handles_labels()
    handles = [h[0] if isinstance(h, container.ErrorbarContainer) else h for h in handles]

    if 'tput' in fname:
        ax.legend(handles, labels, fontsize=18, loc='lower right', frameon=False)
    else:
        ax.legend(handles, labels, fontsize=18, loc='best', frameon=False)
        
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
