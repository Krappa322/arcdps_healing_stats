#include "Socket.h"

#include "../src/Log.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#endif

#include <charconv>
#include <cstdint>
#include <cstring>
#include <system_error>
#include <string>

namespace
{
class SockErrStr
{
public:
#ifdef _WIN32
	static SockErrStr Wsa(int pWsaErr)
	{
		SockErrStr newObj;
		int res = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			pWsaErr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			newObj.mBuf,
			sizeof(newObj.mBuf),
			NULL);
		if (res == 0)
		{
			LogD("FormatMessageA failed - {}", GetLastError());
		}
		newObj.mBufUsed = res;
		return newObj;
	}
#else
	static SockErrStr Errno(int pErrno)
	{
		SockErrStr newObj;
		const char* res = strerror_r(pErrno, newObj.mBuf, sizeof(newObj.mBuf));
		if (res == newObj.mBuf)
		{
			newObj.mBufUsed = strlen(newObj.mBuf);
		}
		else
		{
			// Static string, copy it into our own buffer
			newObj.mBufUsed = std::min(strlen(res), sizeof(newObj.mBuf));
			memcpy(newObj.mBuf, res, newObj.mBufUsed);
		}
		return newObj;
	}
#endif
	static SockErrStr Last()
	{
#ifdef _WIN32
		return Wsa(WSAGetLastError());
#else
		return Errno(errno);
#endif
	}

	std::string_view AsView() const
	{
		return std::string_view{mBuf, mBufUsed};
	}

private:
	SockErrStr() = default;

	uint32_t mBufUsed;
	char mBuf[128];
};

#ifdef _WIN32
SocketError SocketErrorWsa(int pWsaErr)
{
	switch (pWsaErr)
	{
	case WSAEWOULDBLOCK:
		return SocketError::WouldBlock;

	case WSAEINTR: // Intentionally unmapped because it doesn't mean signal on win32
	default:
		LogW("Can't translate wsaerr {} - {}", pWsaErr, SockErrStr::Wsa(pWsaErr));
		return SocketError::Unknown;
	}
}
#else
SocketError SocketErrorErrno(int pErrno)
{
	switch (pErrno)
	{
	case EWOULDBLOCK:
		static_assert(EAGAIN == EWOULDBLOCK);
		return SocketError::WouldBlock;
	case EINTR:
		return SocketError::Interrupted;

	default:
		LogW("Can't translate errno {} - {}", pErrno, SockErrStr::Errno(pErrno));
		return SocketError::Unknown;
	}
}
#endif

class SockAddrStr
{
public:
	SockAddrStr(const sockaddr* pSockAddr, socklen_t pSockAddrLen)
	{
		const char* inet_ntop_ret;
		uint16_t port;

		switch (pSockAddr->sa_family)
		{
		case AF_INET:
		{
			const sockaddr_in* addr = reinterpret_cast<const sockaddr_in*>(pSockAddr);
			inet_ntop_ret = inet_ntop(AF_INET, &addr->sin_addr, mBuf, sizeof(mBuf));
			port = addr->sin_port;
			break;
		}

		case AF_INET6:
		{
			const sockaddr_in6* addr = reinterpret_cast<const sockaddr_in6*>(pSockAddr);
			inet_ntop_ret = inet_ntop(AF_INET6, &addr->sin6_addr, mBuf, sizeof(mBuf));
			port = addr->sin6_port;
			break;
		}

		default:
		{
			LogD("Unknown sockaddr family {}", pSockAddr->sa_family);

			static constexpr const char ERR_STR[] = "<unknown sockaddr family>";
			mBufUsed = sizeof(ERR_STR) - 1;
			memcpy(mBuf, ERR_STR, mBufUsed);
			return;
		}
		}

		if (inet_ntop_ret != nullptr)
		{
			mBufUsed = static_cast<uint32_t>(strlen(mBuf));
		}
		else
		{
			LogD("inet_ntop failed - {}", SockErrStr::Last());

			static constexpr const char ERR_STR[] = "<inet_ntop error>";
			mBufUsed = sizeof(ERR_STR) - 1;
			memcpy(mBuf, ERR_STR, mBufUsed);
		}


		mBuf[mBufUsed++] = ':';
		std::to_chars_result res = std::to_chars(mBuf + mBufUsed, mBuf + sizeof(mBuf), port);
		if (res.ec == std::errc{})
		{
			mBufUsed = static_cast<uint32_t>(res.ptr - mBuf);
		}
		else
		{
			LogD("std::to_chars port {} failed - {}", port, std::make_error_code(res.ec).message());
		}
	}

	std::string_view AsView() const
	{
		return std::string_view{ mBuf, mBufUsed };
	}

private:
	uint32_t mBufUsed;
	char mBuf[72]; // INET6_ADDRSTRLEN + strlen(":65536")
};
} // anonymous namespace

template<>
struct fmt::formatter<SockErrStr> : SimpleFormatter
{
	// Formats the point p using the parsed format specification (presentation)
	// stored in this formatter.
	template <typename FormatContext>
	auto format(const SockErrStr& pObject, FormatContext& pContext) const
	{
		return fmt::format_to(
			pContext.out(),
			"{}",
			pObject.AsView());
	}
};

template<>
struct fmt::formatter<SockAddrStr> : SimpleFormatter
{
	// Formats the point p using the parsed format specification (presentation)
	// stored in this formatter.
	template <typename FormatContext>
	auto format(const SockAddrStr& pObject, FormatContext& pContext) const
	{
		return fmt::format_to(
			pContext.out(),
			"{}",
			pObject.AsView());
	}
};

#ifdef _WIN32
#define LogTReturnError(pFormatString, ...) \
do {\
	int err = WSAGetLastError();\
	LogT(pFormatString, ##__VA_ARGS__, SockErrStr::Wsa(err));\
	return SocketErrorWsa(err);\
} while (false)
#define LogDReturnError(pFormatString, ...) \
do {\
	int err = WSAGetLastError();\
	LogD(pFormatString, ##__VA_ARGS__, SockErrStr::Wsa(err));\
	return SocketErrorWsa(err);\
} while (false)
#else
#define LogTReturnError(pFormatString, ...) \
do {\
	int err = errno;\
	LogT(pFormatString, ##__VA_ARGS__, SockErrStr::Errno(err));\
	return SocketErrorErrno(err);\
} while (false)
#define LogDReturnError(pFormatString, ...) \
do {\
	int err = errno;\
	LogD(pFormatString, ##__VA_ARGS__, SockErrStr::Errno(err));\
	return SocketErrorErrno(err);\
} while (false)
#endif

Socket::Socket() noexcept
	: mHandle{INVALID_SOCKET}
{}

Socket::~Socket() noexcept
{
	close();
}

Socket::Socket(Socket&& pOther) noexcept
	: mHandle(pOther.mHandle)
{
	pOther.mHandle = INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& pOther) noexcept
{
	mHandle = pOther.mHandle;
	pOther.mHandle = INVALID_SOCKET;
	return *this;
}

SocketError Socket::socket(int domain, int type, int protocol)
{
	close();

	SocketHandleT res = ::socket(domain, type, protocol);
	if (res == INVALID_SOCKET)
	{
		LogDReturnError("socket {} {} {} failed - {}", domain, type, protocol);
	}

	mHandle = res;
	return SocketError::None;
}

void Socket::close()
{
	if (mHandle == INVALID_SOCKET)
	{
		// Socket was never created
		return;
	}

#ifdef _WIN32
	int res = ::closesocket(mHandle);
	if (res != 0)
	{
		LogD("closesocket {} failed - {}", mHandle, SockErrStr::Last());
	}
#else
	int res = ::close(mHandle);
	if (res != 0)
	{
		LogD("close {} failed - {}", mHandle, SockErrStr::Last());
	}
#endif

	mHandle = INVALID_SOCKET;
}

SocketError Socket::bind(const sockaddr* addr, socklen_t addrlen)
{
	int res = ::bind(mHandle, addr, addrlen);
	if (res != 0)
	{
		LogDReturnError("bind {} {} failed - {}", mHandle, SockAddrStr(addr, addrlen));
	}

	return SocketError::None;
}

SocketError Socket::connect(const sockaddr* addr, socklen_t addrlen)
{
	int res = ::connect(mHandle, addr, addrlen);
	if (res != 0)
	{
		LogDReturnError("connect {} {} failed - {}", mHandle, SockAddrStr(addr, addrlen));
	}

	return SocketError::None;
}

SocketError Socket::listen(int backlog)
{
	int res = ::listen(mHandle, backlog);
	if (res != 0)
	{
		LogDReturnError("listen {} {} failed - {}", mHandle, backlog);
	}

	return SocketError::None;
}

SocketError Socket::accept(sockaddr* addr, socklen_t* addrlen, Socket* pOutSocket)
{
	assert(pOutSocket->mHandle == INVALID_SOCKET);

	SocketHandleT res = ::accept(mHandle, addr, addrlen);
	if (res == INVALID_SOCKET)
	{
		LogDReturnError("accept {} failed - {}", mHandle);
	}

	pOutSocket->mHandle = res;
	return SocketError::None;
}

SocketError Socket::send(const void* buf, uint32_t len, int flags, uint32_t* pOutSentBytes)
{
	int res = ::send(mHandle, static_cast<const char*>(buf), len, flags);
	if (res < 0)
	{
		LogTReturnError("send {} {} {:x} failed - {}", mHandle, len, flags);
	}

	*pOutSentBytes = res;
	return SocketError::None;
}

SocketError Socket::recv(void* buf, uint32_t len, int flags, uint32_t* pOutRecvedBytes)
{
	int res = ::recv(mHandle, static_cast<char*>(buf), len, flags);
	if (res < 0)
	{
		LogTReturnError("recv {} {} {:x} failed - {}", mHandle, len, flags);
	}

	*pOutRecvedBytes = res;
	return SocketError::None;
}

SocketError Socket::getsockname(sockaddr* name, socklen_t* namelen)
{
	int res = ::getsockname(mHandle, name, namelen);
	if (res < 0)
	{
		LogDReturnError("getsockname {} failed - {}", mHandle);
	}

	return SocketError::None;
}

SocketError Socket::socket_and_bind(std::string_view pBindEndpoint)
{
	std::string_view addressView;
	uint16_t port;
	if (ParseEndpoint(pBindEndpoint, true, &addressView, &port) == false)
	{
		LogE("Failed to parse {}", pBindEndpoint);
		return SocketError::Unknown;
	}
	
	addrinfo* resolvedInfo;
	{
		// needs null termination
		std::string addressStr{addressView};

		addrinfo hint{};
		hint.ai_flags = AI_ADDRCONFIG;
		hint.ai_family = AF_UNSPEC;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		int res = ::getaddrinfo(addressStr.c_str(), nullptr, &hint, &resolvedInfo);
		if (res != 0)
		{
			LogE("getaddrinfo {} failed - {} - {}", addressStr, res, SockErrStr::Last());
			return SocketError::Unknown;
		}
	}
	
	{
		SocketError err = socket(resolvedInfo->ai_family, resolvedInfo->ai_socktype, resolvedInfo->ai_protocol);
		if (err != SocketError::None)
		{
			freeaddrinfo(resolvedInfo);
			return err;
		}
	}

	{
		SocketError err = bind(resolvedInfo->ai_addr, static_cast<socklen_t>(resolvedInfo->ai_addrlen));
		if (err != SocketError::None)
		{
			freeaddrinfo(resolvedInfo);
			return err;
		}
	}

	freeaddrinfo(resolvedInfo);
	return SocketError::None;
}

SocketHandleT Socket::GetUnderlyingHandle() const
{
	return mHandle;
}

bool Socket::ParseEndpoint(std::string_view pEndpoint, bool pPortRequired, std::string_view* pOutAddress, uint16_t* pOutPort)
{
	std::string_view::size_type colonPos = pEndpoint.find(':');
	if (colonPos == std::string_view::npos)
	{
		if (pPortRequired)
		{
			return false;
		}
			
		*pOutAddress = pEndpoint;
		return true;
	}

	std::string_view portStr = pEndpoint.substr(colonPos + 1);
	*pOutAddress = pEndpoint.substr(0, colonPos);

	std::from_chars_result portParseRes = std::from_chars(portStr.data(), portStr.data() + portStr.size(), *pOutPort);
	if (portParseRes.ec != std::errc{})
	{
		return false;
	}

	return true;
}
