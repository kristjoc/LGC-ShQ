#!/usr/local/bin/python3

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
# from matplotlib.ticker import (FixedFormatter, FormatStrFormatter, FixedLocator)

labels = ['Flow 1', 'Flow 2', 'Flow 3', 'Flow 4', 'Flow 5']
filename = ('conv.dat')
imported_data = np.genfromtxt(filename)

def autolabe(ax, rects):
    '''Attach a text label above each bar in *rects*, displaying its height.
    '''
    for rect in rects:
        height = rect.get_height()
        # height = round(height, 3)
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 6),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom',
                    fontsize=8, zorder=10)

def autolabel(ax, rects):
    '''Attach a text label above each bar displaying its height
    '''
    data_line, capline, barlinecols = rects.errorbar

    for err_segment, rect in zip(barlinecols[0].get_segments(), rects):
        height = err_segment[1][1]  # Use height of error bar
        value = rect.get_height()

        ax.text(rect.get_x() + rect.get_width() / 2,
                value,
                height,
                ha='center', va='bottom',
                fontsize=9, zorder=10)

def run_conv():
    ''' Everything inside this function
    '''
    dctcp_hull = np.array([])
    dctcp_hull = np.append(dctcp_hull, imported_data[:, 0])

    lgc_shq = np.array([])
    lgc_shq = np.append(lgc_shq, imported_data[:, 1])

    x = np.array([1, 1.1, 1.2, 1.3, 1.4])  # the label locations

    width = 0.025  # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(x - width/2,
                    dctcp_hull,
                    width,
                    label='DCTCP-HULL',
                    color='firebrick',
                    edgecolor='k',
                    align='center',
                    zorder=10,
                    hatch='//')

    rects2 = ax.bar(x + width/2,
                    lgc_shq,
                    width,
                    label='LGC-ShQ',
                    color='green',
                    edgecolor='k',
                    align='center',
                    zorder=10,
                    hatch='xx')

    # mpl.rc('hatch', color='k', linewidth=0.5)
    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Convergence Time [ms]', fontsize=13)
    # ax.set_title('Link Capacity: 10Gbps', fontsize=16)
    ax.set_xticks(x)
    # ax.set_yticks(np.arange(0, 12.1, 3))
    # ax.set_yticks(np.array([0, 3, 6, 9, 11]))
    ax.set_xticklabels(labels, fontsize=11)
    plt.yticks(fontsize=12)

    legend = ax.legend(fontsize=12, loc='upper left', frameon=False)
    ax.grid(True, which='major', axis='y', lw=0.65, ls='--', dashes=(3, 7), zorder=0)

    # autolabel(ax, rects1)
    # autolabel(ax, rects2)

    fig.set_size_inches(4.5, 3)
    plt.draw() # Draw the figure so you can find the positon of the legend.

    # Hide the right and top spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    # plt.margins(x=0.02)
    plt.tight_layout()
    # Get the bounding box of the original legend
    bb = legend.get_bbox_to_anchor().inverse_transformed(ax.transAxes)

    # Change to location of the legend.
    xOffset = 0.015
    yOffset = 0.015
    bb.x0 += xOffset
    bb.x1 += xOffset
    bb.y0 += yOffset
    bb.y1 += yOffset
    legend.set_bbox_to_anchor(bb, transform=ax.transAxes)
    legend.get_frame().set_linewidth(0.5)
    plt.savefig('conv-time.pdf',
                format='pdf',
                dpi=1200,
                bbox_inches='tight',
                pad_inches=0.01)

if __name__ == '__main__':
    run_conv()
