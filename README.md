# Delay-Tolerant ICN and Its Application to LoRa (ACM ICN 2022)

[![Paper][paper-badge]][paper-link]
[![Preprint][preprint-badge]][preprint-link]
[![Video][video-badge]][video-link]

This repository contains code and documentation to reproduce experimental results of the paper **"[Delay-Tolerant ICN and Its Application to LoRa][preprint-link]"** published in Proc. of ACM ICN 2022.

* Peter Kietzmann, José Alamos, Dirk Kutscher, Thomas C. Schmidt, Matthias Wählisch,
**Delay-Tolerant ICN and Its Application to LoRa**,
In: Proc. of 9th ACM Conference on Information-Centric Networking (ICN), ACM : New York, September 2022.

 **Abstract**
 > Connecting low-power long-range wireless networks, such as LoRa,
to the Internet imposes significant challenges because of the vastly
longer round-trip-times (RTTs) in these constrained networks. In
this paper, we present an ICN protocol framework that enables robust and efficient delay-tolerant communication to edge networks,
including but not limited to LoRa. Our approach provides ICN-
idiomatic communication between networks with vastly different
RTTs for different use cases. We applied this framework to LoRa,
enabling end-to-end consumer-to-LoRa-producer interaction over
an ICN-Internet and asynchronous ("push") data production in the
LoRa edge. Instead of using LoRaWAN, we implemented an IEEE
802.15.4e DSME MAC layer on top of the LoRa PHY layer and ICN
protocol mechanisms in the RIOT operating system. Executed on
off-the-shelf IoT hardware, we provide a comparative evaluation
for basic CCNx/NDN-style ICN [60], RICE [31]-like pulling, and
reflexive forwarding [46]. This is the first practical evaluation of
ICN over LoRa using a reliable MAC. Our results show that periodic
polling in CCNx/NDN works inefficiently when facing long and
differing RTTs. RICE reduces polling overhead and exploits gateway knowledge, without violating core ICN principles. Reflexive
forwarding reflects sporadic IoT data generation naturally. Combined with our local unsolicited data trigger mechanism, it operates
very efficiently and enables lifetimes of >1 year for battery powered
LoRa-ICN nodes.

Please follow our [Getting Started](getting_started.md) instructions for further information on how to compile and execute the code.

<!-- TODO: update URLs -->
[paper-link]:https://doi.org/10.1145/3517212.3558081
[preprint-link]:https://arxiv.org/abs/2209.00863
[video-link]:https://youtu.be/PXPXehfOkBs
[paper-badge]:https://img.shields.io/badge/Paper-ACM%20DL-green
[preprint-badge]: https://img.shields.io/badge/Preprint-arXiv-green
[video-badge]:https://img.shields.io/youtube/views/PXPXehfOkBs?style=social
