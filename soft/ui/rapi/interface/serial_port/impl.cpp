#if defined(_WIN32)

#include <sstream>

#include "impl.hpp"
#include "tools/debug_tools.h"

//#define SERIAL_OVERLAP_MODE

using serial::bytesize_t;
using serial::flowcontrol_t;
using serial::IOException;
using serial::parity_t;
using serial::PortNotOpenedException;
using serial::Serial;
using serial::SerialException;
using serial::stopbits_t;
using serial::Timeout;
using std::invalid_argument;
using std::string;
using std::stringstream;
using std::wstring;

inline wstring _prefix_port_if_needed(const wstring &input)
{
    static wstring windows_com_port_prefix = L"\\\\.\\";
    if(input.compare(windows_com_port_prefix) != 0)
    {
        return windows_com_port_prefix + input;
    }
    return input;
}

Serial::SerialImpl::SerialImpl(const string &port, unsigned long baudrate,
                               bytesize_t bytesize,
                               parity_t parity, stopbits_t stopbits,
                               flowcontrol_t flowcontrol)
    : port_(port.begin(), port.end()), fd_(INVALID_HANDLE_VALUE), is_open_(false),
      baudrate_(baudrate), parity_(parity),
      bytesize_(bytesize), stopbits_(stopbits), flowcontrol_(flowcontrol)
{
    if(port_.empty() == false)
        open();
    read_mutex = CreateMutex(nullptr, false, nullptr);
    write_mutex = CreateMutex(nullptr, false, nullptr);
}

Serial::SerialImpl::~SerialImpl()
{
    this->close();
    CloseHandle(read_mutex);
    CloseHandle(write_mutex);
}

void Serial::SerialImpl::open()
{
    if(port_.empty())
    {
        throw invalid_argument("Empty port is invalid.");
    }
    if(is_open_ == true)
    {
        throw SerialException("Serial port already open.");
    }

    // See: https://github.com/wjwwood/serial/issues/84
    wstring port_with_prefix = _prefix_port_if_needed(port_);
    LPCWSTR lp_port = port_with_prefix.c_str();
#ifdef SERIAL_OVERLAP_MODE
    fd_ = CreateFileW(lp_port,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      nullptr,
                      OPEN_EXISTING,
                      FILE_FLAG_OVERLAPPED,
                      nullptr);
#else
    fd_ = CreateFileW(lp_port,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      nullptr,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      nullptr);
#endif

    if(fd_ == INVALID_HANDLE_VALUE)
    {
        DWORD create_file_err = GetLastError();
        stringstream ss;
        switch(create_file_err)
        {
        case ERROR_FILE_NOT_FOUND:
            // Use this->getPort to convert to a std::string
            ss << "Specified port, " << this->getPort() << ", does not exist.";
            THROW(IOException, ss.str().c_str());

        case ERROR_ACCESS_DENIED:
            ss << "Access denied when opening serial port, " << this->getPort();
            THROW(IOException, ss.str().c_str());

        default:
            ss << "Unknown error opening the serial port: " << create_file_err;
            THROW(IOException, ss.str().c_str());
        }
    }

    reconfigurePort();
    is_open_ = true;
}

void Serial::SerialImpl::reconfigurePort()
{
    if(fd_ == INVALID_HANDLE_VALUE)
    {
        // Can only operate on a valid file descriptor
        THROW(IOException, "Invalid file descriptor, is the serial port open?");
    }

    DCB dcbSerialParams;
    memset(&dcbSerialParams, 0, sizeof(DCB));

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if(!GetCommState(fd_, &dcbSerialParams))
    {
        //error getting state
        THROW(IOException, "Error getting the serial port state.");
    }

#if 0
    PRINT_MSG("Current settings:");
    PRINT_MSG((int)dcbSerialParams.BaudRate);
    PRINT_MSG((int)dcbSerialParams.ByteSize);
    PRINT_MSG((int)dcbSerialParams.Parity);
    PRINT_MSG((int)dcbSerialParams.StopBits);

    // setup baud rate
    switch(baudrate_)
    {
#ifdef CBR_0
    case 0: dcbSerialParams.BaudRate = CBR_0; break;
#endif
#ifdef CBR_50
    case 50: dcbSerialParams.BaudRate = CBR_50; break;
#endif
#ifdef CBR_75
    case 75: dcbSerialParams.BaudRate = CBR_75; break;
#endif
#ifdef CBR_110
    case 110: dcbSerialParams.BaudRate = CBR_110; break;
#endif
#ifdef CBR_134
    case 134: dcbSerialParams.BaudRate = CBR_134; break;
#endif
#ifdef CBR_150
    case 150: dcbSerialParams.BaudRate = CBR_150; break;
#endif
#ifdef CBR_200
    case 200: dcbSerialParams.BaudRate = CBR_200; break;
#endif
#ifdef CBR_300
    case 300: dcbSerialParams.BaudRate = CBR_300; break;
#endif
#ifdef CBR_600
    case 600: dcbSerialParams.BaudRate = CBR_600; break;
#endif
#ifdef CBR_1200
    case 1200: dcbSerialParams.BaudRate = CBR_1200; break;
#endif
#ifdef CBR_1800
    case 1800: dcbSerialParams.BaudRate = CBR_1800; break;
#endif
#ifdef CBR_2400
    case 2400: dcbSerialParams.BaudRate = CBR_2400; break;
#endif
#ifdef CBR_4800
    case 4800: dcbSerialParams.BaudRate = CBR_4800; break;
#endif
#ifdef CBR_7200
    case 7200: dcbSerialParams.BaudRate = CBR_7200; break;
#endif
#ifdef CBR_9600
    case 9600: dcbSerialParams.BaudRate = CBR_9600; break;
#endif
#ifdef CBR_14400
    case 14400: dcbSerialParams.BaudRate = CBR_14400; break;
#endif
#ifdef CBR_19200
    case 19200: dcbSerialParams.BaudRate = CBR_19200; break;
#endif
#ifdef CBR_28800
    case 28800: dcbSerialParams.BaudRate = CBR_28800; break;
#endif
#ifdef CBR_57600
    case 57600: dcbSerialParams.BaudRate = CBR_57600; break;
#endif
#ifdef CBR_76800
    case 76800: dcbSerialParams.BaudRate = CBR_76800; break;
#endif
#ifdef CBR_38400
    case 38400: dcbSerialParams.BaudRate = CBR_38400; break;
#endif
#ifdef CBR_115200
    case 115200: dcbSerialParams.BaudRate = CBR_115200; break;
#endif
#ifdef CBR_128000
    case 128000: dcbSerialParams.BaudRate = CBR_128000; break;
#endif
#ifdef CBR_153600
    case 153600: dcbSerialParams.BaudRate = CBR_153600; break;
#endif
#ifdef CBR_230400
    case 230400: dcbSerialParams.BaudRate = CBR_230400; break;
#endif
#ifdef CBR_256000
    case 256000: dcbSerialParams.BaudRate = CBR_256000; break;
#endif
#ifdef CBR_460800
    case 460800: dcbSerialParams.BaudRate = CBR_460800; break;
#endif
#ifdef CBR_921600
    case 921600: dcbSerialParams.BaudRate = CBR_921600; break;
#endif
    default:
        // Try to blindly assign it
        dcbSerialParams.BaudRate = baudrate_;
    }

    // setup char len
    if(bytesize_ == eightbits)
        dcbSerialParams.ByteSize = 8;
    else if(bytesize_ == sevenbits)
        dcbSerialParams.ByteSize = 7;
    else if(bytesize_ == sixbits)
        dcbSerialParams.ByteSize = 6;
    else if(bytesize_ == fivebits)
        dcbSerialParams.ByteSize = 5;
    else
        throw invalid_argument("invalid char len");

    // setup stopbits
    if(stopbits_ == stopbits_one)
        dcbSerialParams.StopBits = ONESTOPBIT;
    else if(stopbits_ == stopbits_one_point_five)
        dcbSerialParams.StopBits = ONE5STOPBITS;
    else if(stopbits_ == stopbits_two)
        dcbSerialParams.StopBits = TWOSTOPBITS;
    else
        throw invalid_argument("invalid stop bit");

    // setup parity
    if(parity_ == parity_none)
    {
        dcbSerialParams.Parity = NOPARITY;
    }
    else if(parity_ == parity_even)
    {
        dcbSerialParams.Parity = EVENPARITY;
    }
    else if(parity_ == parity_odd)
    {
        dcbSerialParams.Parity = ODDPARITY;
    }
    else if(parity_ == parity_mark)
    {
        dcbSerialParams.Parity = MARKPARITY;
    }
    else if(parity_ == parity_space)
    {
        dcbSerialParams.Parity = SPACEPARITY;
    }
    else
    {
        throw invalid_argument("invalid parity");
    }

    // setup flowcontrol
    if(flowcontrol_ == flowcontrol_none)
    {
        dcbSerialParams.fOutxCtsFlow = false;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.fOutX = false;
        dcbSerialParams.fInX = false;
    }
    if(flowcontrol_ == flowcontrol_software)
    {
        dcbSerialParams.fOutxCtsFlow = false;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.fOutX = true;
        dcbSerialParams.fInX = true;
    }
    if(flowcontrol_ == flowcontrol_hardware)
    {
        dcbSerialParams.fOutxCtsFlow = true;
        dcbSerialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcbSerialParams.fOutX = false;
        dcbSerialParams.fInX = false;
    }
#endif
    // activate settings
//    if(!SetCommState(fd_, &dcbSerialParams))
//    {
//        CloseHandle(fd_);
//        DWORD create_file_err = GetLastError();
//        std::string err = "Error setting serial port settings." + std::to_string(create_file_err);
//        THROW(IOException, err.c_str());
//    }

    // Setup timeouts
    COMMTIMEOUTS timeouts;
    memset(&timeouts, 0, sizeof(COMMTIMEOUTS));
    timeouts.ReadIntervalTimeout = timeout_.inter_byte_timeout;
    timeouts.ReadTotalTimeoutConstant = timeout_.read_timeout_constant;
    timeouts.ReadTotalTimeoutMultiplier = timeout_.read_timeout_multiplier;
    timeouts.WriteTotalTimeoutConstant = timeout_.write_timeout_constant;
    timeouts.WriteTotalTimeoutMultiplier = timeout_.write_timeout_multiplier;

#ifdef SERIAL_OVERLAP_MODE
    timeouts.ReadIntervalTimeout         = 0;
    timeouts.ReadTotalTimeoutConstant    = 0;
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.WriteTotalTimeoutConstant   = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
#endif

    if(!SetCommTimeouts(fd_, &timeouts))
    {
        THROW(IOException, "Error setting timeouts.");
    }

#ifdef SERIAL_OVERLAP_MODE
    if(!SetCommMask(fd_, EV_RXCHAR))
    {
        THROW(IOException, "Error setting CommMask.");
    }

    memset(&fd_overlap_read, 0, sizeof(OVERLAPPED));
    memset(&fd_overlap_write, 0, sizeof(OVERLAPPED));
    memset(&fd_overlap_evt, 0, sizeof(OVERLAPPED));

    fd_overlap_read.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    fd_overlap_write.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    fd_overlap_evt.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if(fd_overlap_read.hEvent == nullptr ||
       fd_overlap_write.hEvent == nullptr ||
       fd_overlap_evt.hEvent == nullptr)
    {
        THROW(IOException, "Error creating overlap objects");
    }

PurgeComm(fd_,  PURGE_RXCLEAR |
          PURGE_TXCLEAR |
          PURGE_TXABORT |
          PURGE_RXABORT);
#endif
}

void Serial::SerialImpl::close()
{
    if(is_open_ == true)
    {
        if(fd_ != INVALID_HANDLE_VALUE)
        {
            int ret;
            ret = CloseHandle(fd_);
            if(ret == 0)
            {
                stringstream ss;
                ss << "Error while closing serial port: " << GetLastError();
                THROW(IOException, ss.str().c_str());
            }
            else
            {
                fd_ = INVALID_HANDLE_VALUE;
            }
        }
        is_open_ = false;
    }
}

bool Serial::SerialImpl::isOpen() const
{
    return is_open_;
}

size_t Serial::SerialImpl::available()
{
    if(!is_open_)
    {
        return 0;
    }
    COMSTAT cs;
    if(!ClearCommError(fd_, nullptr, &cs))
    {
        stringstream ss;
        ss << "Error while checking status of the serial port: " << GetLastError();
        THROW(IOException, ss.str().c_str());
    }
    return static_cast<size_t>(cs.cbInQue);
}

[[ noreturn ]] bool Serial::SerialImpl::waitReadable(uint32_t /*timeout*/)
{
    THROW(IOException, "waitReadable is not implemented on Windows.");
}

[[ noreturn ]] void Serial::SerialImpl::waitByteTimes(size_t /*count*/)
{
    THROW(IOException, "waitByteTimes is not implemented on Windows.");
}

size_t Serial::SerialImpl::read(std::vector<uint8_t> &buf)
{
    if(!is_open_)
    {
        throw PortNotOpenedException("Serial::read");
    }

#ifdef SERIAL_OVERLAP_MODE
    DWORD bytes_to_read = 0;
    DWORD err;
    COMSTAT stat;

    if(!evt_waiting)
    {
        evt_mask = EV_RXCHAR;
        if(!WaitCommEvent(fd_, &evt_mask, &fd_overlap_evt))
        {
            if(GetLastError() != ERROR_IO_PENDING)
            {
                //                LOG_INFO(debug_log, "%s: WaitCommEvent failed", __func__);
                PRINT_ERROR("WaitCommEvent failed");
                return 0;
            }
            else
            {
                evt_waiting = true;
            }
        }
        else
        {
            ClearCommError(fd_, &err, &stat);
            bytes_to_read = stat.cbInQue;
            evt_waiting = false;
        }
    }

    if(evt_waiting)
    {
        switch(WaitForSingleObject(fd_overlap_evt.hEvent, 10 /* ms */))
        {
        case WAIT_OBJECT_0:
        {
            GetOverlappedResult(fd_, &fd_overlap_evt, &evt_mask_len, false);
            ResetEvent(fd_overlap_evt.hEvent);
            ClearCommError(fd_, &err, &stat);
            bytes_to_read = stat.cbInQue;
            evt_waiting = FALSE;
        }
            break;

        case WAIT_TIMEOUT:
            return 0;

        default:
            PRINT_ERROR("GetOverlappedResult failed");
            //                LOG_INFO(debug_log, "%s: GetOverlappedResult failed", __func__);
            evt_waiting = FALSE;
            return 0;
        }
    }

    if(bytes_to_read)
    {
        buf.resize(bytes_to_read);
        if(!ReadFile(fd_, buf.data(), bytes_to_read, &l, &fd_overlap_read))
        {
            PRINT_ERROR("ReadFile failed to read buffered data");
            //            LOG_ERROR(debug_log, "%s: ReadFile failed to read buffered data", __func__);
            return 0;
        }
        if(l != bytes_to_read)
        {
            PRINT_ERROR("SMTH FAILED!!!");
        }
        PRINT_MSG("BTR:" << l << "" << bytes_to_read);
        return bytes_to_read;
    }
    else
    {
        return 0;
    }
#else
    DWORD bytes_read;

    size_t av = available();
    if(av)
    {
//        PRINT_MSG("AV: " << av);
    }
    buf.resize(av);
    if(!ReadFile(fd_, buf.data(), static_cast<DWORD>(av), &bytes_read, nullptr))
    {
        stringstream ss;
        ss << "Error while reading from the serial port: " << GetLastError();
        THROW(IOException, ss.str().c_str());
    }
    if(bytes_read != buf.size())
    {
        PRINT_ERROR("Mismatch: " << bytes_read << " " << buf.size());
    }
    return bytes_read;
#endif
}

size_t Serial::SerialImpl::write(const uint8_t *data, size_t length)
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::write");
    }
    DWORD bytes_written;
#ifdef SERIAL_OVERLAP_MODE
    if(!WriteFile(fd_, data, static_cast<DWORD>(length), &bytes_written, &fd_overlap_write))
    {
//                PRINT_ERROR("Write to COM port failed!");
//                stringstream ss;
//                ss << "Error while writing to the serial port: " << GetLastError();
//                THROW(IOException, ss.str().c_str());
        switch(WaitForSingleObject(fd_overlap_write.hEvent, INFINITE))
        {
        case WAIT_OBJECT_0:
            GetOverlappedResult(fd_, &fd_overlap_write, &bytes_written, FALSE);
            ResetEvent(&fd_overlap_write.hEvent);
            break;

        case WAIT_TIMEOUT:
        default:
            bytes_written = 0;
            stringstream ss;
            ss << "Error while writing to the serial port: " << GetLastError();
            THROW(IOException, ss.str().c_str());
            //                break;
        }
    }
    return bytes_written;
#else
    if(!WriteFile(fd_, data, static_cast<DWORD>(length), &bytes_written, nullptr))
    {
        stringstream ss;
        ss << "Error while writing to the serial port: " << GetLastError();
        THROW(IOException, ss.str().c_str());
    }
    return bytes_written;
#endif
}

void Serial::SerialImpl::setPort(const string &port)
{
    port_ = wstring(port.begin(), port.end());
}

string Serial::SerialImpl::getPort() const
{
    return string(port_.begin(), port_.end());
}

void Serial::SerialImpl::setTimeout(serial::Timeout &timeout)
{
    timeout_ = timeout;
    if(is_open_)
    {
        reconfigurePort();
    }
}

serial::Timeout Serial::SerialImpl::getTimeout() const
{
    return timeout_;
}

void Serial::SerialImpl::setBaudrate(unsigned long baudrate)
{
    baudrate_ = baudrate;
    if(is_open_)
    {
        reconfigurePort();
    }
}

unsigned long Serial::SerialImpl::getBaudrate() const
{
    return baudrate_;
}

void Serial::SerialImpl::setBytesize(serial::bytesize_t bytesize)
{
    bytesize_ = bytesize;
    if(is_open_)
    {
        reconfigurePort();
    }
}

serial::bytesize_t Serial::SerialImpl::getBytesize() const
{
    return bytesize_;
}

void Serial::SerialImpl::setParity(serial::parity_t parity)
{
    parity_ = parity;
    if(is_open_)
    {
        reconfigurePort();
    }
}

serial::parity_t Serial::SerialImpl::getParity() const
{
    return parity_;
}

void Serial::SerialImpl::setStopbits(serial::stopbits_t stopbits)
{
    stopbits_ = stopbits;
    if(is_open_)
    {
        reconfigurePort();
    }
}

serial::stopbits_t Serial::SerialImpl::getStopbits() const
{
    return stopbits_;
}

void Serial::SerialImpl::setFlowcontrol(serial::flowcontrol_t flowcontrol)
{
    flowcontrol_ = flowcontrol;
    if(is_open_)
    {
        reconfigurePort();
    }
}

serial::flowcontrol_t Serial::SerialImpl::getFlowcontrol() const
{
    return flowcontrol_;
}

void Serial::SerialImpl::flush()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::flush");
    }
    FlushFileBuffers(fd_);
}

void Serial::SerialImpl::flushInput()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::flushInput");
    }
    PurgeComm(fd_, PURGE_RXCLEAR);
}

void Serial::SerialImpl::flushOutput()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::flushOutput");
    }
    PurgeComm(fd_, PURGE_TXCLEAR);
}

void Serial::SerialImpl::sendBreak(int /*duration*/)
{
    THROW(IOException, "sendBreak is not supported on Windows.");
}

void Serial::SerialImpl::setBreak(bool level)
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::setBreak");
    }
    if(level)
    {
        EscapeCommFunction(fd_, SETBREAK);
    }
    else
    {
        EscapeCommFunction(fd_, CLRBREAK);
    }
}

void Serial::SerialImpl::setRTS(bool level)
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::setRTS");
    }
    if(level)
    {
        EscapeCommFunction(fd_, SETRTS);
    }
    else
    {
        EscapeCommFunction(fd_, CLRRTS);
    }
}

void Serial::SerialImpl::setDTR(bool level)
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::setDTR");
    }
    if(level)
    {
        EscapeCommFunction(fd_, SETDTR);
    }
    else
    {
        EscapeCommFunction(fd_, CLRDTR);
    }
}

bool Serial::SerialImpl::waitForChange()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::waitForChange");
    }
    DWORD dwCommEvent;

    if(!SetCommMask(fd_, EV_CTS | EV_DSR | EV_RING | EV_RLSD))
    {
        // Error setting communications mask
        return false;
    }

    if(!WaitCommEvent(fd_, &dwCommEvent, nullptr))
    {
        // An error occurred waiting for the event.
        return false;
    }
    else
    {
        // Event has occurred.
        return true;
    }
}

bool Serial::SerialImpl::getDSR()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::getDSR");
    }
    DWORD dwModemStatus;
    if(!GetCommModemStatus(fd_, &dwModemStatus))
    {
        THROW(IOException, "Error getting the status of the DSR line.");
    }

    return (MS_DSR_ON & dwModemStatus) != 0;
}

bool Serial::SerialImpl::getRI()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::getRI");
    }
    DWORD dwModemStatus;
    if(!GetCommModemStatus(fd_, &dwModemStatus))
    {
        THROW(IOException, "Error getting the status of the RI line.");
    }

    return (MS_RING_ON & dwModemStatus) != 0;
}

bool Serial::SerialImpl::getCD()
{
    if(is_open_ == false)
    {
        throw PortNotOpenedException("Serial::getCD");
    }
    DWORD dwModemStatus;
    if(!GetCommModemStatus(fd_, &dwModemStatus))
    {
        // Error in GetCommModemStatus;
        THROW(IOException, "Error getting the status of the CD line.");
    }

    return (MS_RLSD_ON & dwModemStatus) != 0;
}

void Serial::SerialImpl::readLock()
{
    if(WaitForSingleObject(read_mutex, INFINITE) != WAIT_OBJECT_0)
    {
        THROW(IOException, "Error claiming read mutex.");
    }
}

void Serial::SerialImpl::readUnlock()
{
    if(!ReleaseMutex(read_mutex))
    {
        THROW(IOException, "Error releasing read mutex.");
    }
}

void Serial::SerialImpl::writeLock()
{
    if(WaitForSingleObject(write_mutex, INFINITE) != WAIT_OBJECT_0)
    {
        THROW(IOException, "Error claiming write mutex.");
    }
}

void Serial::SerialImpl::writeUnlock()
{
    if(!ReleaseMutex(write_mutex))
    {
        THROW(IOException, "Error releasing write mutex.");
    }
}

#endif // #if defined(_WIN32)


#if !defined(_WIN32)

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <errno.h>
#include <paths.h>
#include <sysexits.h>
#include <termios.h>
#include <sys/param.h>
#include <pthread.h>

#if defined(__linux__)
# include <linux/serial.h>
#endif

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#ifdef __MACH__
#include <AvailabilityMacros.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "impl.hpp"

#ifndef TIOCINQ
#ifdef FIONREAD
#define TIOCINQ FIONREAD
#else
#define TIOCINQ 0x541B
#endif
#endif

#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
#include <IOKit/serial/ioss.h>
#endif

using std::string;
using std::stringstream;
using std::invalid_argument;
using serial::MillisecondTimer;
using serial::Serial;
using serial::SerialException;
using serial::PortNotOpenedException;
using serial::IOException;


MillisecondTimer::MillisecondTimer (const uint32_t millis)
  : expiry(timespec_now())
{
  int64_t tv_nsec = expiry.tv_nsec + (millis * 1e6);
  if (tv_nsec >= 1e9) {
    int64_t sec_diff = tv_nsec / static_cast<int> (1e9);
    expiry.tv_nsec = tv_nsec % static_cast<int>(1e9);
    expiry.tv_sec += sec_diff;
  } else {
    expiry.tv_nsec = tv_nsec;
  }
}

int64_t
MillisecondTimer::remaining ()
{
  timespec now(timespec_now());
  int64_t millis = (expiry.tv_sec - now.tv_sec) * 1e3;
  millis += (expiry.tv_nsec - now.tv_nsec) / 1e6;
  return millis;
}

timespec
MillisecondTimer::timespec_now ()
{
  timespec time;
# ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  time.tv_sec = mts.tv_sec;
  time.tv_nsec = mts.tv_nsec;
# else
  clock_gettime(CLOCK_MONOTONIC, &time);
# endif
  return time;
}

timespec
timespec_from_ms (const uint32_t millis)
{
  timespec time;
  time.tv_sec = millis / 1e3;
  time.tv_nsec = (millis - (time.tv_sec * 1e3)) * 1e6;
  return time;
}

Serial::SerialImpl::SerialImpl (const string &port, unsigned long baudrate,
                                bytesize_t bytesize,
                                parity_t parity, stopbits_t stopbits,
                                flowcontrol_t flowcontrol)
  : port_ (port), fd_ (-1), is_open_ (false), xonxoff_ (false), rtscts_ (false),
    baudrate_ (baudrate), parity_ (parity),
    bytesize_ (bytesize), stopbits_ (stopbits), flowcontrol_ (flowcontrol)
{
  pthread_mutex_init(&this->read_mutex, NULL);
  pthread_mutex_init(&this->write_mutex, NULL);
  if (port_.empty () == false)
    open ();
}

Serial::SerialImpl::~SerialImpl ()
{
  close();
  pthread_mutex_destroy(&this->read_mutex);
  pthread_mutex_destroy(&this->write_mutex);
}

void
Serial::SerialImpl::open ()
{
  if (port_.empty ()) {
    throw invalid_argument ("Empty port is invalid.");
  }
  if (is_open_ == true) {
    throw SerialException ("Serial port already open.");
  }

  fd_ = ::open (port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd_ == -1) {
    switch (errno) {
    case EINTR:
      // Recurse because this is a recoverable error.
      open ();
      return;
    case ENFILE:
    case EMFILE:
      THROW (IOException, "Too many file handles open.");
    default:
      THROW (IOException, errno);
    }
  }

  reconfigurePort();
  is_open_ = true;
}

void
Serial::SerialImpl::reconfigurePort ()
{
  if (fd_ == -1) {
    // Can only operate on a valid file descriptor
    THROW (IOException, "Invalid file descriptor, is the serial port open?");
  }

  struct termios options; // The options for the file descriptor

  if (tcgetattr(fd_, &options) == -1) {
    THROW (IOException, "::tcgetattr");
  }

  // set up raw mode / no echo / binary
  options.c_cflag |= (tcflag_t)  (CLOCAL | CREAD);
  options.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL |
                                       ISIG | IEXTEN); //|ECHOPRT

  options.c_oflag &= (tcflag_t) ~(OPOST);
  options.c_iflag &= (tcflag_t) ~(INLCR | IGNCR | ICRNL | IGNBRK);
#ifdef IUCLC
  options.c_iflag &= (tcflag_t) ~IUCLC;
#endif
#ifdef PARMRK
  options.c_iflag &= (tcflag_t) ~PARMRK;
#endif

  // setup baud rate
  bool custom_baud = false;
  speed_t baud;
  switch (baudrate_) {
#ifdef B0
  case 0: baud = B0; break;
#endif
#ifdef B50
  case 50: baud = B50; break;
#endif
#ifdef B75
  case 75: baud = B75; break;
#endif
#ifdef B110
  case 110: baud = B110; break;
#endif
#ifdef B134
  case 134: baud = B134; break;
#endif
#ifdef B150
  case 150: baud = B150; break;
#endif
#ifdef B200
  case 200: baud = B200; break;
#endif
#ifdef B300
  case 300: baud = B300; break;
#endif
#ifdef B600
  case 600: baud = B600; break;
#endif
#ifdef B1200
  case 1200: baud = B1200; break;
#endif
#ifdef B1800
  case 1800: baud = B1800; break;
#endif
#ifdef B2400
  case 2400: baud = B2400; break;
#endif
#ifdef B4800
  case 4800: baud = B4800; break;
#endif
#ifdef B7200
  case 7200: baud = B7200; break;
#endif
#ifdef B9600
  case 9600: baud = B9600; break;
#endif
#ifdef B14400
  case 14400: baud = B14400; break;
#endif
#ifdef B19200
  case 19200: baud = B19200; break;
#endif
#ifdef B28800
  case 28800: baud = B28800; break;
#endif
#ifdef B57600
  case 57600: baud = B57600; break;
#endif
#ifdef B76800
  case 76800: baud = B76800; break;
#endif
#ifdef B38400
  case 38400: baud = B38400; break;
#endif
#ifdef B115200
  case 115200: baud = B115200; break;
#endif
#ifdef B128000
  case 128000: baud = B128000; break;
#endif
#ifdef B153600
  case 153600: baud = B153600; break;
#endif
#ifdef B230400
  case 230400: baud = B230400; break;
#endif
#ifdef B256000
  case 256000: baud = B256000; break;
#endif
#ifdef B460800
  case 460800: baud = B460800; break;
#endif
#ifdef B500000
  case 500000: baud = B500000; break;
#endif
#ifdef B576000
  case 576000: baud = B576000; break;
#endif
#ifdef B921600
  case 921600: baud = B921600; break;
#endif
#ifdef B1000000
  case 1000000: baud = B1000000; break;
#endif
#ifdef B1152000
  case 1152000: baud = B1152000; break;
#endif
#ifdef B1500000
  case 1500000: baud = B1500000; break;
#endif
#ifdef B2000000
  case 2000000: baud = B2000000; break;
#endif
#ifdef B2500000
  case 2500000: baud = B2500000; break;
#endif
#ifdef B3000000
  case 3000000: baud = B3000000; break;
#endif
#ifdef B3500000
  case 3500000: baud = B3500000; break;
#endif
#ifdef B4000000
  case 4000000: baud = B4000000; break;
#endif
  default:
    custom_baud = true;
    // OS X support
#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
    // Starting with Tiger, the IOSSIOSPEED ioctl can be used to set arbitrary baud rates
    // other than those specified by POSIX. The driver for the underlying serial hardware
    // ultimately determines which baud rates can be used. This ioctl sets both the input
    // and output speed.
    speed_t new_baud = static_cast<speed_t> (baudrate_);
    if (-1 == ioctl (fd_, IOSSIOSPEED, &new_baud, 1)) {
      THROW (IOException, errno);
    }
    // Linux Support
#elif defined(__linux__) && defined (TIOCSSERIAL)
    struct serial_struct ser;

    if (-1 == ioctl (fd_, TIOCGSERIAL, &ser)) {
      THROW (IOException, errno);
    }

    // set custom divisor
    ser.custom_divisor = ser.baud_base / static_cast<int> (baudrate_);
    // update flags
    ser.flags &= ~ASYNC_SPD_MASK;
    ser.flags |= ASYNC_SPD_CUST;

    if (-1 == ioctl (fd_, TIOCSSERIAL, &ser)) {
      THROW (IOException, errno);
    }
#else
    throw invalid_argument ("OS does not currently support custom bauds");
#endif
  }
  if (custom_baud == false) {
#ifdef _BSD_SOURCE
    ::cfsetspeed(&options, baud);
#else
    ::cfsetispeed(&options, baud);
    ::cfsetospeed(&options, baud);
#endif
  }

  // setup char len
  options.c_cflag &= (tcflag_t) ~CSIZE;
  if (bytesize_ == eightbits)
    options.c_cflag |= CS8;
  else if (bytesize_ == sevenbits)
    options.c_cflag |= CS7;
  else if (bytesize_ == sixbits)
    options.c_cflag |= CS6;
  else if (bytesize_ == fivebits)
    options.c_cflag |= CS5;
  else
    throw invalid_argument ("invalid char len");
  // setup stopbits
  if (stopbits_ == stopbits_one)
    options.c_cflag &= (tcflag_t) ~(CSTOPB);
  else if (stopbits_ == stopbits_one_point_five)
    // ONE POINT FIVE same as TWO.. there is no POSIX support for 1.5
    options.c_cflag |=  (CSTOPB);
  else if (stopbits_ == stopbits_two)
    options.c_cflag |=  (CSTOPB);
  else
    throw invalid_argument ("invalid stop bit");
  // setup parity
  options.c_iflag &= (tcflag_t) ~(INPCK | ISTRIP);
  if (parity_ == parity_none) {
    options.c_cflag &= (tcflag_t) ~(PARENB | PARODD);
  } else if (parity_ == parity_even) {
    options.c_cflag &= (tcflag_t) ~(PARODD);
    options.c_cflag |=  (PARENB);
  } else if (parity_ == parity_odd) {
    options.c_cflag |=  (PARENB | PARODD);
  }
#ifdef CMSPAR
  else if (parity_ == parity_mark) {
    options.c_cflag |=  (PARENB | CMSPAR | PARODD);
  }
  else if (parity_ == parity_space) {
    options.c_cflag |=  (PARENB | CMSPAR);
    options.c_cflag &= (tcflag_t) ~(PARODD);
  }
#else
  // CMSPAR is not defined on OSX. So do not support mark or space parity.
  else if (parity_ == parity_mark || parity_ == parity_space) {
    throw invalid_argument ("OS does not support mark or space parity");
  }
#endif  // ifdef CMSPAR
  else {
    throw invalid_argument ("invalid parity");
  }
  // setup flow control
  if (flowcontrol_ == flowcontrol_none) {
    xonxoff_ = false;
    rtscts_ = false;
  }
  if (flowcontrol_ == flowcontrol_software) {
    xonxoff_ = true;
    rtscts_ = false;
  }
  if (flowcontrol_ == flowcontrol_hardware) {
    xonxoff_ = false;
    rtscts_ = true;
  }
  // xonxoff
#ifdef IXANY
  if (xonxoff_)
    options.c_iflag |=  (IXON | IXOFF); //|IXANY)
  else
    options.c_iflag &= (tcflag_t) ~(IXON | IXOFF | IXANY);
#else
  if (xonxoff_)
    options.c_iflag |=  (IXON | IXOFF);
  else
    options.c_iflag &= (tcflag_t) ~(IXON | IXOFF);
#endif
  // rtscts
#ifdef CRTSCTS
  if (rtscts_)
    options.c_cflag |=  (CRTSCTS);
  else
    options.c_cflag &= (unsigned long) ~(CRTSCTS);
#elif defined CNEW_RTSCTS
  if (rtscts_)
    options.c_cflag |=  (CNEW_RTSCTS);
  else
    options.c_cflag &= (unsigned long) ~(CNEW_RTSCTS);
#else
#error "OS Support seems wrong."
#endif

  // http://www.unixwiz.net/techtips/termios-vmin-vtime.html
  // this basically sets the read call up to be a polling read,
  // but we are using select to ensure there is data available
  // to read before each call, so we should never needlessly poll
  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 0;

  // activate settings
  ::tcsetattr (fd_, TCSANOW, &options);

  // Update byte_time_ based on the new settings.
  uint32_t bit_time_ns = 1e9 / baudrate_;
  byte_time_ns_ = bit_time_ns * (1 + bytesize_ + parity_ + stopbits_);

  // Compensate for the stopbits_one_point_five enum being equal to int 3,
  // and not 1.5.
  if (stopbits_ == stopbits_one_point_five) {
    byte_time_ns_ += ((1.5 - stopbits_one_point_five) * bit_time_ns);
  }
}

void
Serial::SerialImpl::close ()
{
  if (is_open_ == true) {
    if (fd_ != -1) {
      int ret;
      ret = ::close (fd_);
      if (ret == 0) {
        fd_ = -1;
      } else {
        THROW (IOException, errno);
      }
    }
    is_open_ = false;
  }
}

bool
Serial::SerialImpl::isOpen () const
{
  return is_open_;
}

size_t
Serial::SerialImpl::available ()
{
  if (!is_open_) {
    return 0;
  }
  int count = 0;
  if (-1 == ioctl (fd_, TIOCINQ, &count)) {
      THROW (IOException, errno);
  } else {
      return static_cast<size_t> (count);
  }
}

bool
Serial::SerialImpl::waitReadable (uint32_t timeout)
{
  // Setup a select call to block for serial data or a timeout
  fd_set readfds;
  FD_ZERO (&readfds);
  FD_SET (fd_, &readfds);
  timespec timeout_ts (timespec_from_ms (timeout));
  int r = pselect (fd_ + 1, &readfds, NULL, NULL, &timeout_ts, NULL);

  if (r < 0) {
    // Select was interrupted
    if (errno == EINTR) {
      return false;
    }
    // Otherwise there was some error
    THROW (IOException, errno);
  }
  // Timeout occurred
  if (r == 0) {
    return false;
  }
  // This shouldn't happen, if r > 0 our fd has to be in the list!
  if (!FD_ISSET (fd_, &readfds)) {
    THROW (IOException, "select reports ready to read, but our fd isn't"
           " in the list, this shouldn't happen!");
  }
  // Data available to read.
  return true;
}

void
Serial::SerialImpl::waitByteTimes (size_t count)
{
  timespec wait_time = { 0, static_cast<long>(byte_time_ns_ * count)};
  pselect (0, NULL, NULL, NULL, &wait_time, NULL);
}

size_t
Serial::SerialImpl::read (uint8_t *buf, size_t size)
{
  // If the port is not open, throw
  if (!is_open_) {
    throw PortNotOpenedException ("Serial::read");
  }
  size_t bytes_read = 0;

  // Calculate total timeout in milliseconds t_c + (t_m * N)
  long total_timeout_ms = timeout_.read_timeout_constant;
  total_timeout_ms += timeout_.read_timeout_multiplier * static_cast<long> (size);
  MillisecondTimer total_timeout(total_timeout_ms);

  // Pre-fill buffer with available bytes
  {
    ssize_t bytes_read_now = ::read (fd_, buf, size);
    if (bytes_read_now > 0) {
      bytes_read = bytes_read_now;
    }
  }

  while (bytes_read < size) {
    int64_t timeout_remaining_ms = total_timeout.remaining();
    if (timeout_remaining_ms <= 0) {
      // Timed out
      break;
    }
    // Timeout for the next select is whichever is less of the remaining
    // total read timeout and the inter-byte timeout.
    uint32_t timeout = std::min(static_cast<uint32_t> (timeout_remaining_ms),
                                timeout_.inter_byte_timeout);
    // Wait for the device to be readable, and then attempt to read.
    if (waitReadable(timeout)) {
      // If it's a fixed-length multi-byte read, insert a wait here so that
      // we can attempt to grab the whole thing in a single IO call. Skip
      // this wait if a non-max inter_byte_timeout is specified.
      if (size > 1 && timeout_.inter_byte_timeout == Timeout::max()) {
        size_t bytes_available = available();
        if (bytes_available + bytes_read < size) {
          waitByteTimes(size - (bytes_available + bytes_read));
        }
      }
      // This should be non-blocking returning only what is available now
      //  Then returning so that select can block again.
      ssize_t bytes_read_now =
        ::read (fd_, buf + bytes_read, size - bytes_read);
      // read should always return some data as select reported it was
      // ready to read when we get to this point.
      if (bytes_read_now < 1) {
        // Disconnected devices, at least on Linux, show the
        // behavior that they are always ready to read immediately
        // but reading returns nothing.
        throw SerialException ("device reports readiness to read but "
                               "returned no data (device disconnected?)");
      }
      // Update bytes_read
      bytes_read += static_cast<size_t> (bytes_read_now);
      // If bytes_read == size then we have read everything we need
      if (bytes_read == size) {
        break;
      }
      // If bytes_read < size then we have more to read
      if (bytes_read < size) {
        continue;
      }
      // If bytes_read > size then we have over read, which shouldn't happen
      if (bytes_read > size) {
        throw SerialException ("read over read, too many bytes where "
                               "read, this shouldn't happen, might be "
                               "a logical error!");
      }
    }
  }
  return bytes_read;
}

size_t
Serial::SerialImpl::write (const uint8_t *data, size_t length)
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::write");
  }
  fd_set writefds;
  size_t bytes_written = 0;

  // Calculate total timeout in milliseconds t_c + (t_m * N)
  long total_timeout_ms = timeout_.write_timeout_constant;
  total_timeout_ms += timeout_.write_timeout_multiplier * static_cast<long> (length);
  MillisecondTimer total_timeout(total_timeout_ms);

  bool first_iteration = true;
  while (bytes_written < length) {
    int64_t timeout_remaining_ms = total_timeout.remaining();
    // Only consider the timeout if it's not the first iteration of the loop
    // otherwise a timeout of 0 won't be allowed through
    if (!first_iteration && (timeout_remaining_ms <= 0)) {
      // Timed out
      break;
    }
    first_iteration = false;

    timespec timeout(timespec_from_ms(timeout_remaining_ms));

    FD_ZERO (&writefds);
    FD_SET (fd_, &writefds);

    // Do the select
    int r = pselect (fd_ + 1, NULL, &writefds, NULL, &timeout, NULL);

    // Figure out what happened by looking at select's response 'r'
    /** Error **/
    if (r < 0) {
      // Select was interrupted, try again
      if (errno == EINTR) {
        continue;
      }
      // Otherwise there was some error
      THROW (IOException, errno);
    }
    /** Timeout **/
    if (r == 0) {
      break;
    }
    /** Port ready to write **/
    if (r > 0) {
      // Make sure our file descriptor is in the ready to write list
      if (FD_ISSET (fd_, &writefds)) {
        // This will write some
        ssize_t bytes_written_now =
          ::write (fd_, data + bytes_written, length - bytes_written);
        // write should always return some data as select reported it was
        // ready to write when we get to this point.
        if (bytes_written_now < 1) {
          // Disconnected devices, at least on Linux, show the
          // behavior that they are always ready to write immediately
          // but writing returns nothing.
          throw SerialException ("device reports readiness to write but "
                                 "returned no data (device disconnected?)");
        }
        // Update bytes_written
        bytes_written += static_cast<size_t> (bytes_written_now);
        // If bytes_written == size then we have written everything we need to
        if (bytes_written == length) {
          break;
        }
        // If bytes_written < size then we have more to write
        if (bytes_written < length) {
          continue;
        }
        // If bytes_written > size then we have over written, which shouldn't happen
        if (bytes_written > length) {
          throw SerialException ("write over wrote, too many bytes where "
                                 "written, this shouldn't happen, might be "
                                 "a logical error!");
        }
      }
      // This shouldn't happen, if r > 0 our fd has to be in the list!
      THROW (IOException, "select reports ready to write, but our fd isn't"
                          " in the list, this shouldn't happen!");
    }
  }
  return bytes_written;
}

void
Serial::SerialImpl::setPort (const string &port)
{
  port_ = port;
}

string
Serial::SerialImpl::getPort () const
{
  return port_;
}

void
Serial::SerialImpl::setTimeout (serial::Timeout &timeout)
{
  timeout_ = timeout;
}

serial::Timeout
Serial::SerialImpl::getTimeout () const
{
  return timeout_;
}

void
Serial::SerialImpl::setBaudrate (unsigned long baudrate)
{
  baudrate_ = baudrate;
  if (is_open_)
    reconfigurePort ();
}

unsigned long
Serial::SerialImpl::getBaudrate () const
{
  return baudrate_;
}

void
Serial::SerialImpl::setBytesize (serial::bytesize_t bytesize)
{
  bytesize_ = bytesize;
  if (is_open_)
    reconfigurePort ();
}

serial::bytesize_t
Serial::SerialImpl::getBytesize () const
{
  return bytesize_;
}

void
Serial::SerialImpl::setParity (serial::parity_t parity)
{
  parity_ = parity;
  if (is_open_)
    reconfigurePort ();
}

serial::parity_t
Serial::SerialImpl::getParity () const
{
  return parity_;
}

void
Serial::SerialImpl::setStopbits (serial::stopbits_t stopbits)
{
  stopbits_ = stopbits;
  if (is_open_)
    reconfigurePort ();
}

serial::stopbits_t
Serial::SerialImpl::getStopbits () const
{
  return stopbits_;
}

void
Serial::SerialImpl::setFlowcontrol (serial::flowcontrol_t flowcontrol)
{
  flowcontrol_ = flowcontrol;
  if (is_open_)
    reconfigurePort ();
}

serial::flowcontrol_t
Serial::SerialImpl::getFlowcontrol () const
{
  return flowcontrol_;
}

void
Serial::SerialImpl::flush ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::flush");
  }
  tcdrain (fd_);
}

void
Serial::SerialImpl::flushInput ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::flushInput");
  }
  tcflush (fd_, TCIFLUSH);
}

void
Serial::SerialImpl::flushOutput ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::flushOutput");
  }
  tcflush (fd_, TCOFLUSH);
}

void
Serial::SerialImpl::sendBreak (int duration)
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::sendBreak");
  }
  tcsendbreak (fd_, static_cast<int> (duration / 4));
}

void
Serial::SerialImpl::setBreak (bool level)
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::setBreak");
  }

  if (level) {
    if (-1 == ioctl (fd_, TIOCSBRK))
    {
        stringstream ss;
        ss << "setBreak failed on a call to ioctl(TIOCSBRK): " << errno << " " << strerror(errno);
        throw(SerialException(ss.str().c_str()));
    }
  } else {
    if (-1 == ioctl (fd_, TIOCCBRK))
    {
        stringstream ss;
        ss << "setBreak failed on a call to ioctl(TIOCCBRK): " << errno << " " << strerror(errno);
        throw(SerialException(ss.str().c_str()));
    }
  }
}

void
Serial::SerialImpl::setRTS (bool level)
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::setRTS");
  }

  int command = TIOCM_RTS;

  if (level) {
    if (-1 == ioctl (fd_, TIOCMBIS, &command))
    {
      stringstream ss;
      ss << "setRTS failed on a call to ioctl(TIOCMBIS): " << errno << " " << strerror(errno);
      throw(SerialException(ss.str().c_str()));
    }
  } else {
    if (-1 == ioctl (fd_, TIOCMBIC, &command))
    {
      stringstream ss;
      ss << "setRTS failed on a call to ioctl(TIOCMBIC): " << errno << " " << strerror(errno);
      throw(SerialException(ss.str().c_str()));
    }
  }
}

void
Serial::SerialImpl::setDTR (bool level)
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::setDTR");
  }

  int command = TIOCM_DTR;

  if (level) {
    if (-1 == ioctl (fd_, TIOCMBIS, &command))
    {
      stringstream ss;
      ss << "setDTR failed on a call to ioctl(TIOCMBIS): " << errno << " " << strerror(errno);
      throw(SerialException(ss.str().c_str()));
    }
  } else {
    if (-1 == ioctl (fd_, TIOCMBIC, &command))
    {
      stringstream ss;
      ss << "setDTR failed on a call to ioctl(TIOCMBIC): " << errno << " " << strerror(errno);
      throw(SerialException(ss.str().c_str()));
    }
  }
}

bool
Serial::SerialImpl::waitForChange ()
{
#ifndef TIOCMIWAIT

while (is_open_ == true) {

    int status;

    if (-1 == ioctl (fd_, TIOCMGET, &status))
    {
        stringstream ss;
        ss << "waitForChange failed on a call to ioctl(TIOCMGET): " << errno << " " << strerror(errno);
        throw(SerialException(ss.str().c_str()));
    }
    else
    {
        if (0 != (status & TIOCM_CTS)
         || 0 != (status & TIOCM_DSR)
         || 0 != (status & TIOCM_RI)
         || 0 != (status & TIOCM_CD))
        {
          return true;
        }
    }

    usleep(1000);
  }

  return false;
#else
  int command = (TIOCM_CD|TIOCM_DSR|TIOCM_RI|TIOCM_CTS);

  if (-1 == ioctl (fd_, TIOCMIWAIT, &command)) {
    stringstream ss;
    ss << "waitForDSR failed on a call to ioctl(TIOCMIWAIT): "
       << errno << " " << strerror(errno);
    throw(SerialException(ss.str().c_str()));
  }
  return true;
#endif
}

bool
Serial::SerialImpl::getCTS ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::getCTS");
  }

  int status;

  if (-1 == ioctl (fd_, TIOCMGET, &status))
  {
    stringstream ss;
    ss << "getCTS failed on a call to ioctl(TIOCMGET): " << errno << " " << strerror(errno);
    throw(SerialException(ss.str().c_str()));
  }
  else
  {
    return 0 != (status & TIOCM_CTS);
  }
}

bool
Serial::SerialImpl::getDSR ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::getDSR");
  }

  int status;

  if (-1 == ioctl (fd_, TIOCMGET, &status))
  {
      stringstream ss;
      ss << "getDSR failed on a call to ioctl(TIOCMGET): " << errno << " " << strerror(errno);
      throw(SerialException(ss.str().c_str()));
  }
  else
  {
      return 0 != (status & TIOCM_DSR);
  }
}

bool
Serial::SerialImpl::getRI ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::getRI");
  }

  int status;

  if (-1 == ioctl (fd_, TIOCMGET, &status))
  {
    stringstream ss;
    ss << "getRI failed on a call to ioctl(TIOCMGET): " << errno << " " << strerror(errno);
    throw(SerialException(ss.str().c_str()));
  }
  else
  {
    return 0 != (status & TIOCM_RI);
  }
}

bool
Serial::SerialImpl::getCD ()
{
  if (is_open_ == false) {
    throw PortNotOpenedException ("Serial::getCD");
  }

  int status;

  if (-1 == ioctl (fd_, TIOCMGET, &status))
  {
    stringstream ss;
    ss << "getCD failed on a call to ioctl(TIOCMGET): " << errno << " " << strerror(errno);
    throw(SerialException(ss.str().c_str()));
  }
  else
  {
    return 0 != (status & TIOCM_CD);
  }
}

void
Serial::SerialImpl::readLock ()
{
  int result = pthread_mutex_lock(&this->read_mutex);
  if (result) {
    THROW (IOException, result);
  }
}

void
Serial::SerialImpl::readUnlock ()
{
  int result = pthread_mutex_unlock(&this->read_mutex);
  if (result) {
    THROW (IOException, result);
  }
}

void
Serial::SerialImpl::writeLock ()
{
  int result = pthread_mutex_lock(&this->write_mutex);
  if (result) {
    THROW (IOException, result);
  }
}

void
Serial::SerialImpl::writeUnlock ()
{
  int result = pthread_mutex_unlock(&this->write_mutex);
  if (result) {
    THROW (IOException, result);
  }
}

std::size_t Serial::SerialImpl::read(std::vector<uint8_t> &buf)
{
    auto count = read(buf_rx, sizeof(buf_rx));
    buf.assign(buf_rx, buf_rx + count);
    return count;
}

#endif // !defined(_WIN32)
