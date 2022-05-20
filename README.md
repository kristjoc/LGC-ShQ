<!-- PROJECT SHIELDS -->

<div align="center">

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]

</div>


<!-- PROJECT LOGO -->
<br />
<p align="center">
  <h3 align="center">LGC-ShQ</h3>

  <p align="center">
    LGC-ShQ: Datacenter Congestion Control with Queueless Load-based ECN Marking
    <br />
    <a href="https://github.com/kr1stj0n/LGC-ShQ/wiki"><strong>Explore the Wiki »</strong></a>
    <br />
    <br />
    <a href="https://github.com/kr1stj0n/pep-dna/issues">Report Bug</a>
    ·
    <a href="https://github.com/kr1stj0n/pep-dna/issues">Request Feature</a>
  </p>
</p>


<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary><h2 style="display: inline-block">Table of Contents</h2></summary>
  <ul>
  <li><a href="#introduction">Introduction</a></li>
  <li><a href="#installation">Installation</a></li>
  <li><a href="#publications">Publications</a></li>
  <li><a href="#reproducibility">Reproducibility</a></li>
  <li><a href="#contributing">Contributing</a></li>
  <li><a href="#license">License</a></li>
  <li><a href="#contact">Contact</a></li>
  <li><a href="#acknowledgements">Acknowledgements</a></li>
  </ul>
</details>


<!-- INTRODUCTION -->
## Introduction

Logistic Growth Congestion Control using a Shadow Queue (LGC-ShQ) is a new
ECN-based congestion control mechanism for datacenters. LGC-ShQ relies on ECN
feedback from a Shadow Queue, and it uses ECN not only to decrease the rate, but
it also increases the rate in relation to this signal.  Real-life tests in a
Linux testbed show that LGC-ShQ keeps the real queue at low levels while
achieving good link utilization and fairness.

This `README.md` file provides information on how to build and install the LGC
Congestion Control and the Shadow Queue scheduler in Linux. For more information
on how to replicate the experiments described in the LGC-ShQ paper submitted for
review, check the <a href="https://github.com/kr1stj0n/pep-dna/wiki">Wiki</a>
page.

<!-- INSTALLATION -->
## Installation

The testbed in which we evaluate LGC-ShQ consists of physical servers that run
Debian 10 OS with custom 5.4.x kernel. We include the kernel source tree in
this repository, but you can build the LGC-ShQ modules in your own machine
without having to recompile the whole kernel.

See [`INSTALL.md`](<https://github.com/kr1stj0n/LGC-ShQ/blob/main/INSTALL.md>)
for intructions on how to build the kernel or the kernel modules.


<!-- PUBLICATIONS -->
## Publications

 - LGC-ShQ: Datacenter Congestion Control with Queueless Load-based ECN Marking
   (Submitted for Review)

<!-- USAGE EXAMPLES -->
## Reproducibility

We aim to make our work entirely reproducible and encourage interested
researchers to test the code and replicate the reported experimental
results. The LGC-ShQ implementation and documentation needed to reproduce all
the experiments described in the paper are available in this public
repository. The tools we developed to the run the experiments were installed at
Step 4 of the previous section. The scripts for automated testing, analysis and
plotting of the generated data are located <a
href="https://github.com/kr1stj0n/LGC-ShQ/tree/main/scripts-tools-dataset">here</a>.

_Please, refer to the <a
href="https://github.com/kr1stj0n/pep-dna/wiki">Wiki</a> page for a detailed
explanation on how to to replicate all the experiments described in the paper._

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to
be learn, inspire, and create. Any contributions you make are **greatly
appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/multi-bit`)
3. Commit your Changes (`git commit -m 'Added multi-bit support'`)
4. Push to the Branch (`git push origin feature/multi-bit`)
5. Open a Pull Request


<!-- LICENSE -->
## License

Distributed under `GPLv2-or-later` License. See
[`LICENSES`](<https://github.com/kr1stj0n/LGC-ShQ/tree/main/web10g-kis-0.13-5.4/LICENSES>)
for more information.


<!-- CONTACT -->
## Contact

Kristjon Ciko - kristjoc@ifi.uio.no

Project Link:
[https://github.com/kr1stj0n/LGC-ShQ](https://github.com/kr1stj0n/LGC-ShQ)


<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements

* [The Linux Foundation](https://github.com/torvalds/linux)
* [Web10G project](https://github.com/rapier1/web10g)
* [TEACUP](http://caia.swin.edu.au/tools/teacup)


<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/kr1stj0n/LGC-ShQ.svg?style=for-the-badge
[contributors-url]: https://github.com/kr1stj0n/LGC-ShQ/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/kr1stj0n/LGC-ShQ.svg?style=for-the-badge
[forks-url]: https://github.com/kr1stj0n/LGC-ShQ/network/members
[stars-shield]: https://img.shields.io/github/stars/kr1stj0n/LGC-ShQ.svg?style=for-the-badge
[stars-url]: https://github.com/kr1stj0n/LGC-ShQ/stargazers
[issues-shield]: https://img.shields.io/github/issues/kr1stj0n/LGC-ShQ.svg?style=for-the-badge
[issues-url]: https://github.com/kr1stj0n/LGC-ShQ/issues
