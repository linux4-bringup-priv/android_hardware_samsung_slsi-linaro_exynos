/*
 * Copyright (c) 2013-2020 TRUSTONIC LIMITED
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// C++
#include <condition_variable>
#include <mutex>
#include <thread>
#include <sstream>
// C
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
// Kinibi
#include "mcimcp.h"             // mclfHeaderV2_t and friends
// Logging
#include "dynamic_log.h"
// Driver
#include "mc_user.h"
#include "mc_admin.h"
// Registry
#include "PrivateRegistry.h"
// Self
#include "SecureWorld.h"

#define MAX_SO_CONT_SIZE 512

#define MY_LOG_ERRNO(fmt, ...) \
    LOG_E("%s: %s " fmt, __func__, strerror(errno), ##__VA_ARGS__)

struct SecureWorld::Impl {
    int device_fd = -1;
    uint32_t command_id = 0;
    mcVersionInfo_t tee_versions;

    // Thread variables
    std::thread thread;
    // State of this object
    enum {
        CLOSED,
        OPEN,                   // Device is open
        RUNNING,                // Thread is running
        STOPPED,                // Thread has stopped, device is closed
    } state = CLOSED;
    std::condition_variable state_cond;
    std::mutex state_mutex;

    // This callback is called whenever the TEE death is first detected
    std::function<void()> tee_dead_cb;

    void run();
};

SecureWorld::SecureWorld(): pimpl_(new Impl) {}

SecureWorld::~SecureWorld() {
    delete pimpl_;
}

static bool readTrustlet(const std::string& path, std::string* buffer,
                         uint32_t* service_type) {
    *service_type = SERVICE_TYPE_ILLEGAL;

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_W("Cannot open trustlet %s (%s)", path.c_str(), strerror(errno));
        return false;
    }

    struct stat stat;
    if (::fstat(fd, &stat) < 0) {
        MY_LOG_ERRNO("getting size for trustlet %s", path.c_str());
        ::close(fd);
        return false;
    }

    void* addr = ::mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        MY_LOG_ERRNO("mapping file to memory");
        ::close(fd);
        return false;
    }

    /* Give service type to driver so it knows how to allocate and copy */
    mclfHeaderV2_t* header = static_cast<mclfHeaderV2_t*>(addr);
    *service_type = header->serviceType;
    buffer->resize(stat.st_size);
    ::memcpy(&(*buffer)[0], addr, buffer->length());
    ::munmap(addr, stat.st_size);
    ::close(fd);
    return true;
}

static bool getTrustlet(const struct mc_admin_request* request,
                        struct mc_admin_response* response,
                        std::string* buffer) {
    const mcUuid_t* uuid = reinterpret_cast<const mcUuid_t*>(&request->uuid);
    std::string path;

    mcResult_t res = mcRegistryGetTrustletInfo(uuid, request->is_gp,
                                               path);
    switch (res) {
        case MC_DRV_OK:
            response->error_no = 0;
            break;
        case MC_DRV_ERR_INVALID_PARAMETER:
            response->error_no = EINVAL;
            break;
        case MC_DRV_ERR_TRUSTLET_NOT_FOUND:
            response->error_no = ENOENT;
            break;
        default:
            // Some kind of default
            response->error_no = EPERM;
    }

    if (response->error_no) {
        return false;
    }

    if (!readTrustlet(path, buffer, &response->service_type)) {
        response->error_no = errno;
    } else {
        response->length = static_cast<uint32_t>(buffer->length());
        LOG_D("mmap'd trustlet from %s, total size: %u",
              path.c_str(), response->length);
    }
    return true;
}

int SecureWorld::setup() {
    pimpl_->device_fd = ::open("/dev/" MC_ADMIN_DEVNODE, O_RDWR);
    if (pimpl_->device_fd < 0) {
        MY_LOG_ERRNO("opening admin device");
        return -1;
    }

    LOG_D("Check version of Kinibi driver");
    struct mc_admin_driver_info info;
    if (::ioctl(pimpl_->device_fd, MC_ADMIN_IO_GET_INFO, &info) != 0) {
        LOG_ERRNO("ioctl MC_ADMIN_IO_GET_INFO");
        ::close(pimpl_->device_fd);
        return -1;
    }

    unsigned int major = info.drv_version >> 16;
    unsigned int minor = info.drv_version & 0xffff;
    LOG_I("driver version: v%d.%d", major, minor);
    if ((major != MCDRVMODULEAPI_VERSION_MAJOR) ||
            (minor != MCDRVMODULEAPI_VERSION_MINOR)) {
        ::close(pimpl_->device_fd);
        errno = EHOSTDOWN;
        LOG_E("%s in %s (driver version mismatch detected: expected v%d.%d)",
              strerror(errno), __FUNCTION__, MCDRVMODULEAPI_VERSION_MAJOR,
              MCDRVMODULEAPI_VERSION_MINOR);
        return -1;
    }
    pimpl_->command_id = info.initial_cmd_id;

    // We wait 10 times 100ms for the device file to appear
    int counter = 10;
    do {
        int fd = ::open("/dev/" MC_USER_DEVNODE, O_RDWR);
        if (fd >= 0) {
            ::close(fd);
            break;
        }

        if (errno != ENOENT) {
            MY_LOG_ERRNO("opening user device");
            return -1;
        }

        /* Device driver didn't appear yet: give some time */
        usleep(100000);
    } while (counter--);

    if (counter < 0) {
        LOG_C("timed out while waiting for user device to appear");
        return -1;
    }

    auto ret = mcOpenDevice(MC_DEVICE_ID_DEFAULT);
    if (ret != MC_DRV_OK) {
        LOG_C("failed to open device: %x", ret);
        return -1;
    }

    ret = mcGetMobiCoreVersion(MC_DEVICE_ID_DEFAULT, &pimpl_->tee_versions);
    if (ret != MC_DRV_OK) {
        LOG_C("failed to get TEE version: %x", ret);
        return -1;
    }

    mcCloseDevice(MC_DEVICE_ID_DEFAULT);

    std::lock_guard<std::mutex> lock(pimpl_->state_mutex);
    pimpl_->state = Impl::OPEN;
    LOG_I("TEE is ready, version: %s", pimpl_->tee_versions.productId);
    return 0;
}

void SecureWorld::close() {
    // In case close is called while waiting is active
    stop();

    std::lock_guard<std::mutex> lock(pimpl_->state_mutex);
    if (pimpl_->state == Impl::OPEN) {
        ::close(pimpl_->device_fd);
    }
    pimpl_->state = Impl::CLOSED;

    LOG_W("closed");
}

void SecureWorld::receiveSignal(int signum) {
    if ((signum == SIGINT) || (signum == SIGTERM)) {
        LOG_E("Signal %d received, closing TEE link", signum);
        stop();
    }
}

int SecureWorld::open() {
    // Create thread
    int ret = 0;
    std::lock_guard<std::mutex> lock(pimpl_->state_mutex);
    // We don't check the current state here, because we trust the caller
    pimpl_->thread = std::thread(&Impl::run, pimpl_);
    pimpl_->state = Impl::RUNNING;
    return ret;
}

void SecureWorld::wait() {
    LOG_D("wait for thread to stop");
    std::unique_lock<std::mutex> lock(pimpl_->state_mutex);
    while (pimpl_->state == Impl::RUNNING) {
        pimpl_->state_cond.wait(lock);
    }
    LOG_D("thread stopped");
}

void SecureWorld::stop() {
    if (pimpl_->thread.joinable()) {
        LOG_D("stop thread");
        ::pthread_kill(pimpl_->thread.native_handle(), SIGUSR1);
        pimpl_->thread.join();
        LOG_D("thread joined");
    }
}

int SecureWorld::loadDriver(const std::string& path) {
    struct mc_admin_load_info info;
    bool load_with_uuid = false;
    uint32_t service_type;

    std::string buffer;
    size_t slash_pos = path.find_first_of('/');
    if (slash_pos != std::string::npos) {
        // a path to blob is passed.
        if (!readTrustlet(path, &buffer, &service_type)) {
            return -1;
        }
    } else {
        // a UUID is passed. we check first for its validity
        if (path.length() != sizeof(mcUuid_t) * 2 ) {
            LOG_E("Invalid UUID length %s", path.c_str());
            return -1;
        }

        // check if the UUID is comprised of only hex digits.
        size_t wrong_char_pos = path.find_first_not_of("0123456789abcdefABCDEF");
        if (wrong_char_pos != std::string::npos) {
            LOG_E("Invalid UUID %s", path.c_str());
            return -1;
        }

        mcUuid_t uuid;
        auto ptr = reinterpret_cast<uint8_t*>(&uuid);
        for (size_t i = 0; i < path.length(); i += 2) {
            std::string byte_string = path.substr(i, 2);
            if (::sscanf(byte_string.c_str(), "%" SCNx8, ptr) != 1) {
                return -1;
            }
            ptr++;
        }

        std::string srv_path_in_registry;
        mcResult_t res = mcRegistryGetDriverInfo(&uuid, srv_path_in_registry);
        if (res != MC_DRV_OK) {
            return -1;
        }
        // check if the uuid corresponds to a blob in the registry
        if (!readTrustlet(srv_path_in_registry, &buffer, &service_type)) {
            // otherwise we pass the uuid without the blob to the SWd.
            LOG_D("Passing only UUID %s to SWd", path.c_str());
            ::memcpy(&info.uuid, &uuid, sizeof(uuid));
            load_with_uuid = true;
        }
    }

    if (!load_with_uuid) {
        info.address = reinterpret_cast<uintptr_t>(&buffer[0]);
        info.length = static_cast<uint32_t>(buffer.length());
    } else {
        info.address = 0;
        info.length = 0;
    }

    LOG_D("Load secure driver %s of size %d", path.c_str(), info.length);

#if defined(__QNX__)
    iov_t sv[2];
    SETIOV(&sv[0], &info, sizeof(info));
    SETIOV(&sv[1], info.address, info.length);
    int ret = ::devctlv(pimpl_->device_fd, MC_ADMIN_IO_LOAD_DRIVER, 2, 0, sv, NULL, NULL);
#else
    int ret = ::ioctl(pimpl_->device_fd, MC_ADMIN_IO_LOAD_DRIVER, &info);
#endif
    if (ret != 0) {
        if (errno == EBUSY) {
            LOG_I("Driver %s already loaded", path.c_str());
            // Ignore the error
            ret = 0;
        } else {
            LOG_ERRNO("ioctl MC_ADMIN_IO_LOAD_DRIVER");
        }
    }

    return (ret == 0) ? 0 : -1;
}

int SecureWorld::loadToken(const void* data, size_t length) {
    struct mc_admin_load_info token;

    LOG_D("Load authentication token %p of size %zu", data, length);
    token.address = reinterpret_cast<uintptr_t>(data);
    token.length = static_cast<uint32_t>(length);

#if defined(__QNX__)
    (void)token;
    int rc = ::devctl(pimpl_->device_fd, MC_ADMIN_IO_LOAD_TOKEN, (void*)data, length, NULL);
#else
    int rc = ::ioctl(pimpl_->device_fd, MC_ADMIN_IO_LOAD_TOKEN, &token);
#endif
    if (rc != 0) {
        LOG_ERRNO("ioctl MC_ADMIN_IO_LOAD_TOKEN");
        return -1;
    }

    return 0;
}

// SIGUSR1 is required to unlock the iotcl function,
// but we have to override the handler with the following dummy function to
// override the default handler : "Term"
static void dummyHandler(int /* sig */) {
    LOG_D("dummyHandler");
    return;
}

void SecureWorld::Impl::run() {
    ::pthread_setname_np(pthread_self(), "McDaemon.SWd");
    // Process SIGUSR1 action
    struct sigaction actionSIGUSR1;
    ::memset(&actionSIGUSR1, 0, sizeof(actionSIGUSR1));
    actionSIGUSR1.sa_handler = dummyHandler;
    ::sigemptyset(&actionSIGUSR1.sa_mask);
    // Explicitely specify "no flags" to avoid the reexecution of the
    // ioctl in case of interruption.
    actionSIGUSR1.sa_flags = 0;
    if (::sigaction(SIGUSR1, &actionSIGUSR1, NULL)) {
        LOG_ERRNO("actionSIGUSR1");
        return;
    }
    ::sigaddset(&actionSIGUSR1.sa_mask, SIGUSR1);
    ::pthread_sigmask(SIG_UNBLOCK, &actionSIGUSR1.sa_mask, NULL);

    LOG_I("Start listening to secure world");
    while (true) {
        struct mc_admin_request request;
        if (::ioctl(device_fd, MC_ADMIN_IO_GET_DRIVER_REQUEST, &request)) {
            if (errno == EINTR) {
                LOG_D("Giving up on signal");
            } else {
                LOG_ERRNO("Getting request from driver");
            }
            break;
        }

        LOG_D("Request %d received (ID %u)", request.command,
              request.request_id);
        if (command_id != request.request_id) {
            LOG_E("Request ID counters are not synchronised (expected %u, got %u)",
                  command_id, request.request_id);
            break;
        }

        struct mc_admin_response response;
        memset(&response, 0, sizeof(response));
        response.request_id = command_id++;
        std::string buffer;

        switch (request.command) {
            case MC_DRV_GET_TRUSTLET:
                getTrustlet(&request, &response, &buffer);
                break;
            case MC_DRV_SIGNAL_CRASH: {
                LOG_C("TEE HALTED");
                std::string crashdump;
                crashdump.resize(1024);
                ssize_t ret = ::read(device_fd, &crashdump[0], crashdump.size());
                if (ret > 0) {
                    crashdump.resize(ret);
                    std::istringstream stream(crashdump);
                    std::string line;
                    while (std::getline(stream, line)) {
                        LOG_I("%s", line.c_str());
                    }
                } else {
                    // Version otherwise logged as part of the crash dump
                    LOG_I("TEE version: %s", tee_versions.productId);
                }

                if (tee_dead_cb) {
                    tee_dead_cb();
                }

                break;
            }
            default:
                LOG_E("Unknown command");
                response.error_no = EBADRQC;
        }

        LOG_D("Response errno %d length %u", response.error_no, response.length);
        ssize_t ret = ::write(device_fd, &response, sizeof(response));
        if (ret != sizeof(response)) {
            LOG_ERRNO("Sending response to driver");
            ret = -1;
        } else if (response.length > 0) {
            ssize_t expected_length = response.length;
            ret = ::write(device_fd, buffer.c_str(), response.length);
            if (ret != expected_length) {
                LOG_ERRNO("Sending response data to driver");
                ret = -1;
            }
        }
    }

    // An error has occured, signal the ending of the thread
    LOG_I("Stop listening to secure world");
    std::lock_guard<std::mutex> lock(state_mutex);
    state = STOPPED;
    ::close(device_fd);
    state_cond.notify_all();
    LOG_D("end thread");
}

void SecureWorld::registerTeeDeathCallback(std::function<void()> tee_dead_cb) {
    pimpl_->tee_dead_cb = tee_dead_cb;
}
