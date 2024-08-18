//
//  largeFileReaderTests.swift
//  largeFileReaderTests
//
//  Created by Peter de Vroomen on 22/07/2024.
//

import Testing
import LargeFileReaderLib

// NOTE: The setup and teardown (init/deinit) is run on start/end of every test function. There seems to be
// no way to do a setup/teardown for the whole suite, the suite is created/deleted for every function. This
// means that if the tests are run in parallel, the files are being copied and deleted multiple times, leading
// to tests failing. Therefore, we run the tests in series.
//
// We might be able to run the tests in parallel if we don't delete the files, or if we open the files directly
// from the bundle.
//
// LargeFileReaderLib is basically thread-safe. It only needs the file to not be deleted while it has the file
// open. We could add an event/callback if the library finds that the file has been removed, for nicer handling.

@Suite("LargeFileReaderLibTests", .serialized) final class LargeFileReaderLibTests {
    
    init() async throws {
        copyTestFiles()
    }
    
    deinit {
        deleteTestFiles()
    }
    
    @Test @MainActor func testOpenAndCloseEmptyFile() async throws {
        let largeFileReader = LargeFileReader()
        #expect(largeFileReader.isOpen == false)
        let openResult = largeFileReader.open(testPathForFile("test_empty.log").path(percentEncoded: false))
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
    }
    
    @Test @MainActor func testOpenAndCloseSmallFile() async throws {
        let largeFileReader = LargeFileReader()
        #expect(largeFileReader.isOpen == false)
        let openResult = largeFileReader.open(testPathForFile("test_small.log").path(percentEncoded: false))
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
    }
    
    @Test @MainActor func testOpenAndCloseLargeFile() async throws {
        let largeFileReader = LargeFileReader()
        #expect(largeFileReader.isOpen == false)
        let openResult = largeFileReader.open(testPathForFile("test_large.log").path(percentEncoded: false))
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
    }
    
    @Test @MainActor func testOpenAndReadSmallFile() async throws {
        let largeFileReader = LargeFileReader()
        #expect(largeFileReader.isOpen == false)
        
        // test_small.log contains text-only and is 24548 bytes long.
        let openResult = largeFileReader.open(testPathForFile("test_small.log").path(percentEncoded: false))
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        
        let buffer: UnsafeMutablePointer<UInt8> = UnsafeMutablePointer<UInt8>.allocate(capacity: 65536)
        buffer.initialize(repeating: 0xFF, count: 65536)
        var bytesRead: Int = 0
        
        // Basic seek and read tests.

        // Seek to 0, read 15 characters.
        largeFileReader.lseek(0, whence: 0)
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "Jul 24 00:07:36")
        buffer.initialize(repeating: 0xFF, count: 65536)

        // Seek to 60, read 15 characters.
        largeFileReader.lseek(60, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "Jul 24 00:23:35")
        buffer.initialize(repeating: 0xFF, count: 65536)
        
        // Make continuous reads from last position.

        // Read next few characters in steps of 15.
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == " KacBoom syslog")
        buffer.initialize(repeating: 0xFF, count: 65536)
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "d[540]: ASL Sen")
        buffer.initialize(repeating: 0xFF, count: 65536)
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "der Statistics\n")
        buffer.initialize(repeating: 0xFF, count: 65536)
        
        // Seek back to 0, check again.
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "Jul 24 00:07:36")
        buffer.initialize(repeating: 0xFF, count: 65536)

        // Reading beyond end of file.

        // Read exactly the last bytes of the file. File length is 24548, move to
        // 24533 (end - 15) and read last 15 bytes.
        largeFileReader.lseek(-15, whence: 2)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == true)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "ux-device:1080\n")
        buffer.initialize(repeating: 0xFF, count: 65536)
        
        // Try to read 15 bytes from a position of 14 bytes from the end of the file.
        largeFileReader.lseek(-14, whence: 2)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 14)
        #expect(largeFileReader.isEof == true)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "x-device:1080\n")
        buffer.initialize(repeating: 0xFF, count: 65536)
        
        // Try to read 15 bytes until trying to read beyond the end of the file. Seek to -25
        // from the end of the file, first read should return 15 bytes, second one should
        // return 10 bytes. Also check the EOF flag.
        largeFileReader.lseek(-25, whence: 2)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "nnected, mux-de")
        buffer.initialize(repeating: 0xFF, count: 65536)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 10)
        #expect(largeFileReader.isEof == true)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "vice:1080\n")
        buffer.initialize(repeating: 0xFF, count: 65536)
        
        // Read the whole file in small batches (241 is a prime) until we reach EOF.
        
        largeFileReader.lseek(0, whence: 0)
        var totalBytesRead = 0
        while largeFileReader.isEof == false {
            bytesRead = largeFileReader.read(buffer, bytes: 241)
            // Should read 241 bytes every time except the last one is 207, but then EOF should be true
            #expect(((largeFileReader.isEof == false) && (bytesRead == 241)) ||
                    ((largeFileReader.isEof == true) && (bytesRead == 207)))
            totalBytesRead += bytesRead
        }
        #expect(totalBytesRead == 24548)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == ":00 KacBoom AMPDeviceDiscoveryAgent[1542]: Entered:_AMMuxedDeviceDisconnected, mux-device:1080\nJul 24 15:09:00 KacBoom AMPDeviceDiscoveryAgent[1542]: Entered:__thr_AMMuxedDeviceDisconnected, mux-device:1080\n")

        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
    }
    
    @Test @MainActor func testOpenAndReadLargeFileLargeBlocks() async throws {
        let largeFileReader = LargeFileReader()
        #expect(largeFileReader.isOpen == false)
        
        // test_large.log contains text-only and is 7626904 bytes long.
        var openResult = largeFileReader.open(testPathForFile("test_large.log").path(percentEncoded: false))
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        
        // Create a buffer that can hold the whole file (7626904) but is also a multiple of cacheBlockSize, so that it for
        // sure can hold the whole file plus some extra.
        let bufferLength = Int(ceil(7626904.0 / Double(largeFileReader.cacheBlockSize)) * Double(largeFileReader.cacheBlockSize))
        let buffer: UnsafeMutablePointer<UInt8> = UnsafeMutablePointer<UInt8>.allocate(capacity: bufferLength)
        
        buffer.initialize(repeating: 0xFF, count: bufferLength)
        var bytesRead: Int = 0
        
        // Basic seek and read tests.
        
        // Seek to 0, read 15 characters.
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "Jun 14 10:38:10")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Seek to 60, read 15 characters.
        largeFileReader.lseek(60, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "ted\nJun 14 10:3")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Read 1 cache block
        
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: largeFileReader.cacheBlockSize)
        #expect(bytesRead == largeFileReader.cacheBlockSize)
        #expect(largeFileReader.isEof == false)
        // Check only 64 first bytes.
        buffer[64] = 0
        #expect(String(cString: buffer) == "Jun 14 10:38:10 localhost powerd[50]: powerd process is started\n")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Read 2 cache blocks
        
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 2 * largeFileReader.cacheBlockSize)
        #expect(bytesRead == (2 * largeFileReader.cacheBlockSize))
        #expect(largeFileReader.isEof == false)
        // Check first 64 bytes from 0.
        buffer[64] = 0
        #expect(String(cString: buffer) == "Jun 14 10:38:10 localhost powerd[50]: powerd process is started\n")
        // Check first 64 bytes from cacheBlockSize.
        var subBuffer = buffer.advanced(by: largeFileReader.cacheBlockSize)
        subBuffer[64] = 0
        #expect(String(cString: subBuffer) == "ll), updates queued for later: (\n\t)\n2024-06-14 03:41:00-07 MacBo")
        // Check 64 bytes in some random spot in the second block.
        subBuffer = buffer.advanced(by: largeFileReader.cacheBlockSize + 37545)
        subBuffer[64] = 0
        #expect(String(cString: subBuffer) == "Book-Pro softwareupdated[211]: Removing client SUUpdateServiceCl")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Make small cache, with 3 cache blocks, check caching.
        
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
        
        openResult = largeFileReader.open(testPathForFile("test_large.log").path(percentEncoded: false), cacheMaxSize: 3 * largeFileReader.cacheDefaultBlockSize, cacheBlockSize: largeFileReader.cacheDefaultBlockSize)
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        
        // Read full 3 blocks
        
        bytesRead = largeFileReader.read(buffer, bytes: 3 * largeFileReader.cacheBlockSize)
        #expect(bytesRead == 3 * largeFileReader.cacheBlockSize)
        #expect(largeFileReader.isEof == false)
        // Check last 64 bytes.
        subBuffer = buffer.advanced(by: 3 * largeFileReader.cacheBlockSize - 64)
        subBuffer[64] = 0
        #expect(String(cString: subBuffer) == "UpdateSettingsExtension[1732]: SUOSUPowerPolicy: Sufficient powe")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Read one more block, now a cache block will be reused.
        bytesRead = largeFileReader.read(buffer, bytes: largeFileReader.cacheBlockSize)
        #expect(bytesRead == largeFileReader.cacheBlockSize)
        #expect(largeFileReader.isEof == false)
        buffer[64] = 0
        #expect(String(cString: buffer) == "r: 1 (ac = 1, battery = 63%)\n2024-06-14 12:57:59+02 Peters-MacBo")
        
        // Read from start to finish, in blocks of 8192 bytes. Last-read block will be 152 bytes.
        // 931 reads of 8192 bytes and 1 of 152 bytes.
        
        largeFileReader.lseek(0, whence: 0)
        
        var totalBytesRead = 0
        var totalBlocksRead = 0;
        while largeFileReader.isEof == false {
            bytesRead = largeFileReader.read(buffer, bytes: 8192)
            #expect(((largeFileReader.isEof == false) && (bytesRead == 8192)) ||
                    ((largeFileReader.isEof == true) && (bytesRead == 152)))
            totalBlocksRead += 1;
            totalBytesRead += bytesRead
        }
        #expect(totalBlocksRead == 932)
        #expect(totalBytesRead == 7626904)

        largeFileReader.lseek(0, whence: 0)

        // Do 3 reads of 64 bytes. Only one cache block should be reused.
        bytesRead = largeFileReader.read(buffer, bytes: 64)
        #expect(bytesRead == 64)
        #expect(largeFileReader.isEof == false)
        bytesRead = largeFileReader.read(buffer, bytes: 64)
        #expect(bytesRead == 64)
        #expect(largeFileReader.isEof == false)
        bytesRead = largeFileReader.read(buffer, bytes: 64)
        #expect(bytesRead == 64)
        #expect(largeFileReader.isEof == false)
        buffer[64] = 0
        #expect(String(cString: buffer) == "00:39:42+02 KacBoom system_installd[965]: PackageKit: Adding cli")
        
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
    }
    
    @Test @MainActor func testOpenAndReadLargeFileSmallBlocks() async throws {
        
        let cacheBlockSize = 4096
        
        let largeFileReader = LargeFileReader()
        #expect(largeFileReader.isOpen == false)
        
        // test_large.log contains text-only and is 7626904 bytes long.
        var openResult = largeFileReader.open(testPathForFile("test_large.log").path(percentEncoded: false), cacheMaxSize: 1048576, cacheBlockSize: cacheBlockSize)
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)

        // Buffer of 64Kb is more than large enough
        let buffer: UnsafeMutablePointer<UInt8> = UnsafeMutablePointer<UInt8>.allocate(capacity: 65536)
        buffer.initialize(repeating: 0xFF, count: 65536)
        var bytesRead: Int = 0
        
        // Basic seek and read tests.
        
        // Seek to 0, read 15 characters.
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "Jun 14 10:38:10")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Seek to 60, read 15 characters.
        largeFileReader.lseek(60, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 15)
        #expect(bytesRead == 15)
        #expect(largeFileReader.isEof == false)
        buffer[bytesRead] = 0
        #expect(String(cString: buffer) == "ted\nJun 14 10:3")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Read 1 cache block
        
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: largeFileReader.cacheBlockSize)
        #expect(bytesRead == largeFileReader.cacheBlockSize)
        #expect(largeFileReader.isEof == false)
        // Check only 64 first bytes.
        buffer[64] = 0
        #expect(String(cString: buffer) == "Jun 14 10:38:10 localhost powerd[50]: powerd process is started\n")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Read 2 cache blocks
        
        largeFileReader.lseek(0, whence: 0)
        
        bytesRead = largeFileReader.read(buffer, bytes: 2 * largeFileReader.cacheBlockSize)
        #expect(bytesRead == (2 * largeFileReader.cacheBlockSize))
        #expect(largeFileReader.isEof == false)
        // Check first 64 bytes from 0.
        buffer[64] = 0
        #expect(String(cString: buffer) == "Jun 14 10:38:10 localhost powerd[50]: powerd process is started\n")
        // Check first 64 bytes from cacheBlockSize.
        var subBuffer = buffer.advanced(by: largeFileReader.cacheBlockSize)
        subBuffer[64] = 0
        #expect(String(cString: subBuffer) == " Progress[64]: currentPhase = \"<IASPPhase: 0x600000455720: \'Rese")
        // Check 64 bytes in some random spot in the second block.
        subBuffer = buffer.advanced(by: largeFileReader.cacheBlockSize + 1325)
        subBuffer[64] = 0
        #expect(String(cString: subBuffer) == "geKey = 35;\n\t    }\n\t)\nJun 14 10:38:11 MacBook-Pro mobile_obliter")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Make small cache, with 3 cache blocks, check caching.
        
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
        
        // test_large.log contains text-only and is 7626904 bytes long.
        openResult = largeFileReader.open(testPathForFile("test_large.log").path(percentEncoded: false), cacheMaxSize: 3 * cacheBlockSize, cacheBlockSize: cacheBlockSize)
        try #require(openResult == true)
        #expect(largeFileReader.isOpen == true)
        
        // Read full 3 blocks
        
        bytesRead = largeFileReader.read(buffer, bytes: 3 * largeFileReader.cacheBlockSize)
        #expect(bytesRead == 3 * largeFileReader.cacheBlockSize)
        #expect(largeFileReader.isEof == false)
        // Check last 64 bytes.
        subBuffer = buffer.advanced(by: 3 * largeFileReader.cacheBlockSize - 64)
        subBuffer[64] = 0
        #expect(String(cString: subBuffer) == " Interest in disk0\nJun 14 10:38:11 MacBook-Pro mobile_obliterato")
        buffer.initialize(repeating: 0xFF, count: bytesRead)
        
        // Read one more block, now a cache block will be reused.
        bytesRead = largeFileReader.read(buffer, bytes: largeFileReader.cacheBlockSize)
        #expect(bytesRead == largeFileReader.cacheBlockSize)
        #expect(largeFileReader.isEof == false)
        buffer[64] = 0
        #expect(String(cString: buffer) == "r[90]:  TopLevelDisk: ioType: GUID_partition_scheme, Type:GUID_p")
        
        // Read from start to finish, in blocks of 8192 bytes. Last-read block will be 152 bytes.
        // 931 reads of 8192 bytes and 1 of 152 bytes. This reads 2 cache blocks for every read.
        
        largeFileReader.lseek(0, whence: 0)
        
        var totalBytesRead = 0
        var totalBlocksRead = 0;
        while largeFileReader.isEof == false {
            bytesRead = largeFileReader.read(buffer, bytes: 8192)
            #expect(((largeFileReader.isEof == false) && (bytesRead == 8192)) ||
                    ((largeFileReader.isEof == true) && (bytesRead == 152)))
            totalBlocksRead += 1;
            totalBytesRead += bytesRead
        }
        #expect(totalBlocksRead == 932)
        #expect(totalBytesRead == 7626904)

        largeFileReader.lseek(0, whence: 0)

        // Do 3 reads of 64 bytes. Only one cache block should be reused.
        bytesRead = largeFileReader.read(buffer, bytes: 64)
        #expect(bytesRead == 64)
        #expect(largeFileReader.isEof == false)
        bytesRead = largeFileReader.read(buffer, bytes: 64)
        #expect(bytesRead == 64)
        #expect(largeFileReader.isEof == false)
        bytesRead = largeFileReader.read(buffer, bytes: 64)
        #expect(bytesRead == 64)
        #expect(largeFileReader.isEof == false)
        buffer[64] = 0
        #expect(String(cString: buffer) == "689]: SUOSUServiceDaemon: availableMobileSoftwareUpdates = (\n\t  ")
        
        largeFileReader.close()
        #expect(largeFileReader.isOpen == false)
    }
    
    
    func copyTestFiles() {
        copyTestFile(filename: "test_empty.log")
        copyTestFile(filename: "test_small.log")
        copyTestFile(filename: "test_large.log")
    }
    
    func deleteTestFiles() {
        deleteTestFile(filename: "test_empty.log")
        deleteTestFile(filename: "test_small.log")
        deleteTestFile(filename: "test_large.log")
    }
    
    func bundlePathForFile(_ filename: String) -> URL {
        Bundle(for: LargeFileReaderLibTests.self).url(forResource: filename, withExtension: nil)!
    }
    
    func testPathForFile(_ filename: String) -> URL {
        FileManager.default.temporaryDirectory.appendingPathComponent(filename)
    }
    
    func copyTestFile(filename: String) {
        let fromFile = bundlePathForFile(filename)
        let toFile = testPathForFile(filename)

        do {
            try FileManager.default.copyItem(at: fromFile, to: toFile)
        }
        catch {
            print(error)
        }
    }
    
    func deleteTestFile(filename: String) {
        let toFile = testPathForFile(filename)

        try? FileManager.default.removeItem(at: toFile)
    }
}
