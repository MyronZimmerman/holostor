HoloStor is a library that makes use of mathematical and processor optimizations to provide high throughput software based erasure correction.  The library can be easily adapted to implementing triple parity (and higher level) RAID solutions using the spare compute cycles available on multi-core processors.  Both user space and kernel space operation is supported.

The need for erasure correcting codes that can recover from multiple losses has been well established in communications.  There is a growing awareness, however, that storage systems also need higher redundancy to offset the increasing disk repair times that accompany advances in disk capacity (Ref 1).  HoloStor provides a high performance implementation of an erasure correcting code that can recover two, three or more failed units in a storage group.

Not all erasure correcting codes have the same performance.  HoloStor’s patented technology starts with storage optimal erasure correcting codes implemented in Galois Fields and mathematically relates these codes to XOR operations that can be performed on potentially large fixed sized chunks of data.  Having established the connection between a code and the performance needed to implement it as XOR operations allows the best performing codes to be chosen.

The HoloStor library includes an optimal erasure code that can be applied to storage groups as large as 17 units (including up to 4 redundant units) and implements the XOR operations in the fastest possible manner on Intel processors (using SSE instructions).  The library is fully configurable, has high test coverage and is packaged for easy integration with other open source projects.

Alternative licensing is available.

References:

1. Leventhal, A.  Triple-parity RAID and beyond.  In Proceedings of Commun. ACM. 2010, 58-63. ([abstract](http://cacm.acm.org/magazines/2010/1/55741-triple-parity-raid-and-beyond/abstract))