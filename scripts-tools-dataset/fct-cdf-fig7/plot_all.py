#!/usr/bin/python3

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import statsmodels.api as sm # recommended import according to the docs
import sys


def plot_over(fname='', pdf_dir='', out_file=''):

    imported_data_hull = np.genfromtxt('fct-hull.dat')
    imported_data_shq = np.genfromtxt('fct-shq.dat')
    # labels = imported_data[:, 0]
    # a = imported_data[:, 0]
    # hull_sem = imported_data[:, 2]
    # lgc = imported_data[:, 3]
    # lgc_sem = imported_data[:, 4]

    sample_hull = imported_data_hull
    sample_shq = imported_data_shq
    ecdf_hull = sm.distributions.ECDF(sample_hull)
    ecdf_shq = sm.distributions.ECDF(sample_shq)

    x = np.linspace(min(sample_shq), max(sample_hull))
    # x = np.linspace(2.0, 4.0)
    y_hull = ecdf_hull(x)
    y_shq = ecdf_shq(x)

    # 3) set to NaN where y = 0
    yh = y_hull.copy()
    yh[y_hull == 0] = np.nan

    ys = y_shq.copy()
    ys[y_shq == 0] = np.nan

    
    fig, ax = plt.subplots()

    # plt.plot(x, yh, linewidth=3, label='DCTCP-HULL', color='firebrick', marker='o',
    #          mfc='white', markersize=4)
    # plt.plot(x, ys, linewidth=3, label='LGC-ShQ', color='green', marker='D',
    #          markerfacecolor='white', markersize=4)

    plt.plot(x, yh, linewidth=3, label='DCTCP-HULL', color='firebrick',
             dashes=[4,1,2,1])
    plt.plot(x, ys, linewidth=3, label='LGC-ShQ', color='green',
             dashes=[4.5,3,4.5,3])

    plt.ylabel('CDF of Flow Completion Times', fontsize=18)
    plt.xlabel('Flow Completion Time [ms]', fontsize=18)
    plt.grid(True, which='major', lw=0.65, ls='--', dashes=(3, 7), zorder=0)


    # Hide the right and top spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    # Only show ticks on the left and bottom spines
    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')

    # Legend
    legend = ax.legend(fontsize=18, loc='lower right', frameon=False)
    
    plt.draw() # Draw the figure so you can find the positon of the legend.

    # Hide the right and top spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    # Get the bounding box of the original legend
    bb = legend.get_bbox_to_anchor().inverse_transformed(ax.transAxes)

    # Change to location of the legend.
    xOffset = -0.35
    yOffset = 0.35
    bb.x0 += xOffset
    bb.x1 += xOffset
    bb.y0 += yOffset
    bb.y1 += yOffset
    legend.set_bbox_to_anchor(bb, transform=ax.transAxes)
    legend.get_frame().set_linewidth(0.5)

    plt.margins(x=0.02)
    plt.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)
    plt.savefig(pdf_dir + '/' + out_file + '.pdf',
                format='pdf',
                dpi=1200,
                bbox_inches='tight',
                pad_inches=0.025,
                transparent=True)

    return

    # a is the data array
    x = np.sort(a)
    y = np.arange(len(x))/float(len(x))
    plt.plot(x, y)

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
    plt.xticks(fontsize=15)
    plt.yticks(fontsize=15)

    if 'tput' in fname:
        ax.set_ylim(bottom=400, top=1010)

    if 'qlen' in fname:
        plt.ylabel('Queue size [pkts.]', fontsize=20)
    elif 'qdelay' in fname:
        plt.ylabel('Queuing delay [us]', fontsize=20)
    elif 'tput' in fname:
        plt.ylabel('Throughput [Mbps]', fontsize=20)

    if 'drate' in fname:
        plt.xlabel('Drain rate [Mbps]', fontsize=20)
    elif 'nflows' in fname:
        plt.xlabel('Number of Static Flows', fontsize=20)

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
                 # dashes=[5, 2],
                 marker='x',
                 markersize='4',
                 linewidth=2,
                 color='firebrick',
                 label='DCTCP & HULL',
                 zorder=30)

    plt.errorbar(x,
                 lgc,
                 yerr=lgc_sem,
                 # ls='--',
                 # dashes=[3, 3],
                 marker='x',
                 markersize='4',
                 linewidth=2,
                 color='green',
                 label='LGC & ShQ',
                 zorder=30)

    if 'tput' in fname:
        plt.legend(fontsize=18, loc='lower right')
    else:
        plt.legend(fontsize=18, loc='best')

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
