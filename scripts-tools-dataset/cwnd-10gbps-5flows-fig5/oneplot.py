import matplotlib.pyplot as plt
import sys
import datetime
import numpy as np
import pandas as pd
import matplotlib


def plot_cwnd_stats(fname='', pdf_dir=''):

    df = []
    for host in ['.link']:
        filename = 'cwnd_over_convergence_' + fname + '_10000_flows_5_run_1.dat' + host
        df.append(pd.read_csv(filename, header=None, sep=','))

    # Set as index the first column - Timestamps
    df[0].set_index(0, inplace=True)

    # Relative time - Start Time from 0
    df[0].index = [idx0 - df[0].index[0] for idx0 in df[0].index]

    df[0].replace(0, np.nan)
    
    # Plot graph
    ax = df[0].plot(linewidth=2, fontsize=20)

    # sns.set_style("ticks", {'grid.linestyle': '--'})
    plt.locator_params(axis='x', nbins=11)
    ax.set_xlabel('Time (s)', fontsize=20)
    ax.set_ylabel('Congestion window (pkts.)', fontsize=20)

    plt.yticks(fontsize=18)
    plt.xticks(fontsize=18)

    plt.grid(True, which='major', lw=0.65, ls='--', dashes=(3, 7), zorder=70)
    #plt.legend(fontsize=20, loc='best')

    ax.set_ylim(bottom=0)
    
    # Hide the right and top spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    # Only show ticks on the left and bottom spines
    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')
    
    ax.get_legend().remove()
    plt.margins(x=0.02)
    plt.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)
    plt.savefig(pdf_dir + '/' + 'cwnd_' + fname + '.pdf',
                format='pdf',
                dpi=1200,
                bbox_inches='tight',
                pad_inches=0.025)

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
    plt.xticks(fontsize=15)
    plt.yticks(fontsize=15)

    if 'tput' in fname:
        ax.set_ylim(ymin=400, ymax=1010)
    
    if 'qlen' in fname:
        plt.ylabel('Queue size [pkts.]', fontsize=20)
    elif 'qdelay' in fname:
        plt.ylabel('Queuing delay [\u03BCs]', fontsize=20)
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
    plot_cwnd_stats(filename, pdf_dir='.')
