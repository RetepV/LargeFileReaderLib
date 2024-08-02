LargeFileReaderLib

Manage a file for reading using a caching mechanism, where the cache block size and the total size of the cache can be specified. This makes the amount
of memory used deterministic, which is sometimes what we want, if we are managing huge files.

There are two targets: 1) the library itself, 2) a Swift Testing test target.

This library is useful, but the main reason for developing it was to test Swift and C++ interoperability.

I started out by reading these:

* https://www.swift.org/documentation/cxx-interop/
* https://www.swift.org/documentation/cxx-interop/project-build-setup/#mixing-swift-and-c-using-swift-package-manager
* https://www.swift.org/documentation/cxx-interop/project-build-setup/#mixing-swift-and-c-using-xcode

Then got into this WWDC session:

* https://developer.apple.com/videos/play/wwdc2023/10172/

Donwloaded and tried this sample project:

* https://developer.apple.com/documentation/Swift/CallingAPIsAcrossLanguageBoundaries

But then I found that the accompanying sample project https://developer.apple.com/documentation/Swift/CallingAPIsAcrossLanguageBoundaries does not compile
anymore since Xcode 15.4. Xcode does not compile the static C++ functions into the `ForestBuilder-Swift.h` anymore for some reason.
https://forums.swift.org/t/use-swift-framework-code-in-c-app-missing-swift-h-header/70914/10

So, for now, it seems we need to use a `.mm` wrapper again.
