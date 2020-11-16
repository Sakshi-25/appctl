<!--
*** Thanks for checking out this README Template. If you have a suggestion that would
*** make this better, please fork the repo and create a pull request or simply open
*** an issue with the tag "enhancement".
*** Thanks again! Now go create something AMAZING! :D
-->





<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![GPL License][license-shield]][license-url]



<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://github.com/rlxos/appctl.git">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">appctl</h3>

  <p align="center">
    A modern and extensible package manager for Linux*/Unix
    <br />
    <a href="https://github.com/rlxos/appctl"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/rlxos/appctl">View Demo</a>
    ·
    <a href="https://github.com/rlxos/appctl/issues">Report Bug</a>
    ·
    <a href="https://github.com/rlxos/appctl/issues">Request Feature</a>
  </p>
</p>



<!-- TABLE OF CONTENTS -->
## Table of Contents

* [About the Project](#about-the-project)
  * [Built With](#built-with)
* [Getting Started](#getting-started)
  * [Prerequisites](#prerequisites)
  * [Installation](#installation)
* [Usage](#usage)
* [Roadmap](#roadmap)
* [Contributing](#contributing)
* [License](#license)
* [Contact](#contact)
* [Acknowledgements](#acknowledgements)



<!-- ABOUT THE PROJECT -->
## About The Project

<!-- [![Product Name Screen Shot][product-screenshot]](https://example.com) -->
appctl is a modern and extensible project manager and its can support any type of package format and mode of operation (like installation, compilation, upgradation) can be added in forms of modules.

**librlxpkg** - is a avaliable module to provide support for rlxos recipe files. Its the complete implemenation and can install, compile, search and remove any type

appctl primarly targets rlxos (releax os) but can be easily ported to any linux/unix system even without code modifications

### Built With
appctl mainly uses standard POSIX libraries, but for complete functionality need below files.
* [libcurl 7.68.0](https://curl.haxx.se/libcurl/)
* [libressl 3.2.2](https://www.libressl.org/)
* [librlx 0.1.0](https://github.com/rlxos/librlx)



<!-- GETTING STARTED -->
## Getting Started

appctl can work on any system without conflicting to you existing system. Installation path of both system and user package can be configured through /etc/appctl.conf.

While performing any task like installation, uninstallation or upgradation etc. appctl search for modules specified in /etc/appctl.conf or include/default.hh. then appctl pass app-id & args to every module in order if module support that app-id then that module will be used to perform tasks.

### Prerequisites

appctl is pre install in [rlxos](https://releax.in/) but can compile in on any linux/unix system. To perform compilation you need to install prerequisites libraries 
* libcurl, libressl/openssl
```bash
sudo pacman -Syu curl openssl
```
* [librlx](https://github.com/rlxos/librlx)
* [project](https://github.com/itsmanjeet/project)

### Compilation
* libapp - backend library for appctl
```bash
    project build libapp
```

* appctl - cmdline frontend for appctl
```bash
    project build libappctl
```

* librlxpkg - module implementation for appctl
```bash
    project build librlxpkg
```

### Installation
``` bash
    sudo project do install
```

<!-- USAGE EXAMPLES -->
## Usage
appctl use common and standard command line arguments for performing operations, like
```bash
    appctl install <app-name>
```

```bash
    appctl-0.1.0 : rlxos package manager tool

    Usage: appctl [options]

    Options:
        install     [app-name] 		        install specified application from app-id
        info        [app-name] 		        print information of <app>
        list-files  [app-name] 		        list content of install <app>
        cal-dep     [app-name] 		        calculate required dependencies of <app>
        verify-config [section.variable]    print configuration file
        remove      [app-name] 		        remove app from root directory
        download    [<url> <file>] 		    download file specified
        sync        [] 		                sync data from modules
        gen-hash    [file] 		            generate hash sum of input file
```

_For more examples, please refer to the [Documentation](https://github.com/rlxos/appctl/docs/appctl.html)_



<!-- ROADMAP -->
## Roadmap

See the [open issues](https://github.com/rlxos/appctl/issues) for a list of proposed features (and known issues).



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to be learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b 0.1.0`)
3. Commit your Changes (`git commit -m '[my-id] my awesome Feature'`)
4. Push to the Branch (`git push origin 0.1.0`)
5. Open a Pull Request



<!-- LICENSE -->
## License

Distributed under the GPL3 License. See `license` for more information.



<!-- CONTACT -->
## Contact
Manjeet Singh - [@releaxos](https://twitter.com/releaxos) - itsmanjeet@releax.in

[Join](https://discord.gg/TXTxDTYcdg) our discord server for any query


Project Link: [https://github.com/rlxos/appctl](https://github.com/rlxos/appctl)



<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements
* [Scratch](https://github.com/venomlinux/scratchpkg)





<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/rlxos/appctl.svg?style=flat-square
[contributors-url]: https://github.com/rlxos/appctl/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/rlxos/appctl.svg?style=flat-square
[forks-url]: https://github.com/rlxos/appctl/network/members
[stars-shield]: https://img.shields.io/github/stars/rlxos/appctl.svg?style=flat-square
[stars-url]: https://github.com/rlxos/appctl/stargazers
[issues-shield]: https://img.shields.io/github/issues/rlxos/appctl.svg?style=flat-square
[issues-url]: https://github.com/rlxos/appctl/issues
[license-shield]: https://img.shields.io/github/license/rlxos/appctl.svg?style=flat-square
[license-url]: https://github.com/rlxos/appctl/blob/master/license
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=flat-square&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/releax
