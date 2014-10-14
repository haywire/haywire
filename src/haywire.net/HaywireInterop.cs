using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace haywire
{
    public class StatusCodes
    {
        /// HTTP_STATUS_100 -> "100 Continue"
        public const string HTTP_STATUS_100 = "100 Continue";

        /// HTTP_STATUS_101 -> "101 Switching Protocols"
        public const string HTTP_STATUS_101 = "101 Switching Protocols";

        /// HTTP_STATUS_102 -> "102 Processing"
        public const string HTTP_STATUS_102 = "102 Processing";

        /// HTTP_STATUS_200 -> "200 OK"
        public const string HTTP_STATUS_200 = "200 OK";

        /// HTTP_STATUS_201 -> "201 Created"
        public const string HTTP_STATUS_201 = "201 Created";

        /// HTTP_STATUS_202 -> "202 Accepted"
        public const string HTTP_STATUS_202 = "202 Accepted";

        /// HTTP_STATUS_203 -> "203 Non-Authoritative Information"
        public const string HTTP_STATUS_203 = "203 Non-Authoritative Information";

        /// HTTP_STATUS_204 -> "204 No Content"
        public const string HTTP_STATUS_204 = "204 No Content";

        /// HTTP_STATUS_205 -> "205 Reset Content"
        public const string HTTP_STATUS_205 = "205 Reset Content";

        /// HTTP_STATUS_206 -> "206 Partial Content"
        public const string HTTP_STATUS_206 = "206 Partial Content";

        /// HTTP_STATUS_207 -> "207 Multi-Status"
        public const string HTTP_STATUS_207 = "207 Multi-Status";

        /// HTTP_STATUS_300 -> "300 Multiple Choices"
        public const string HTTP_STATUS_300 = "300 Multiple Choices";

        /// HTTP_STATUS_301 -> "301 Moved Permanently"
        public const string HTTP_STATUS_301 = "301 Moved Permanently";

        /// HTTP_STATUS_302 -> "302 Moved Temporarily"
        public const string HTTP_STATUS_302 = "302 Moved Temporarily";

        /// HTTP_STATUS_303 -> "303 See Other"
        public const string HTTP_STATUS_303 = "303 See Other";

        /// HTTP_STATUS_304 -> "304 Not Modified"
        public const string HTTP_STATUS_304 = "304 Not Modified";

        /// HTTP_STATUS_305 -> "305 Use Proxy"
        public const string HTTP_STATUS_305 = "305 Use Proxy";

        /// HTTP_STATUS_307 -> "307 Temporary Redirect"
        public const string HTTP_STATUS_307 = "307 Temporary Redirect";

        /// HTTP_STATUS_400 -> "400 Bad Request"
        public const string HTTP_STATUS_400 = "400 Bad Request";

        /// HTTP_STATUS_401 -> "401 Unauthorized"
        public const string HTTP_STATUS_401 = "401 Unauthorized";

        /// HTTP_STATUS_402 -> "402 Payment Required"
        public const string HTTP_STATUS_402 = "402 Payment Required";

        /// HTTP_STATUS_403 -> "403 Forbidden"
        public const string HTTP_STATUS_403 = "403 Forbidden";

        /// HTTP_STATUS_404 -> "404 Not Found"
        public const string HTTP_STATUS_404 = "404 Not Found";

        /// HTTP_STATUS_405 -> "405 Method Not Allowed"
        public const string HTTP_STATUS_405 = "405 Method Not Allowed";

        /// HTTP_STATUS_406 -> "406 Not Acceptable"
        public const string HTTP_STATUS_406 = "406 Not Acceptable";

        /// HTTP_STATUS_407 -> "407 Proxy Authentication Required"
        public const string HTTP_STATUS_407 = "407 Proxy Authentication Required";

        /// HTTP_STATUS_408 -> "408 Request Time-out"
        public const string HTTP_STATUS_408 = "408 Request Time-out";

        /// HTTP_STATUS_409 -> "409 Conflict"
        public const string HTTP_STATUS_409 = "409 Conflict";

        /// HTTP_STATUS_410 -> "410 Gone"
        public const string HTTP_STATUS_410 = "410 Gone";

        /// HTTP_STATUS_411 -> "411 Length Required"
        public const string HTTP_STATUS_411 = "411 Length Required";

        /// HTTP_STATUS_412 -> "412 Precondition Failed"
        public const string HTTP_STATUS_412 = "412 Precondition Failed";

        /// HTTP_STATUS_413 -> "413 Request Entity Too Large"
        public const string HTTP_STATUS_413 = "413 Request Entity Too Large";

        /// HTTP_STATUS_414 -> "414 Request-URI Too Large"
        public const string HTTP_STATUS_414 = "414 Request-URI Too Large";

        /// HTTP_STATUS_415 -> "415 Unsupported Media Type"
        public const string HTTP_STATUS_415 = "415 Unsupported Media Type";

        /// HTTP_STATUS_416 -> "416 Requested Range Not Satisfiable"
        public const string HTTP_STATUS_416 = "416 Requested Range Not Satisfiable";

        /// HTTP_STATUS_417 -> "417 Expectation Failed"
        public const string HTTP_STATUS_417 = "417 Expectation Failed";

        /// HTTP_STATUS_418 -> "418 I'm a teapot"
        public const string HTTP_STATUS_418 = "418 I\'m a teapot";

        /// HTTP_STATUS_422 -> "422 Unprocessable Entity"
        public const string HTTP_STATUS_422 = "422 Unprocessable Entity";

        /// HTTP_STATUS_423 -> "423 Locked"
        public const string HTTP_STATUS_423 = "423 Locked";

        /// HTTP_STATUS_424 -> "424 Failed Dependency"
        public const string HTTP_STATUS_424 = "424 Failed Dependency";

        /// HTTP_STATUS_425 -> "425 Unordered Collection"
        public const string HTTP_STATUS_425 = "425 Unordered Collection";

        /// HTTP_STATUS_426 -> "426 Upgrade Required"
        public const string HTTP_STATUS_426 = "426 Upgrade Required";

        /// HTTP_STATUS_428 -> "428 Precondition Required"
        public const string HTTP_STATUS_428 = "428 Precondition Required";

        /// HTTP_STATUS_429 -> "429 Too Many Requests"
        public const string HTTP_STATUS_429 = "429 Too Many Requests";

        /// HTTP_STATUS_431 -> "431 Request Header Fields Too Large"
        public const string HTTP_STATUS_431 = "431 Request Header Fields Too Large";

        /// HTTP_STATUS_500 -> "500 Internal Server Error"
        public const string HTTP_STATUS_500 = "500 Internal Server Error";

        /// HTTP_STATUS_501 -> "501 Not Implemented"
        public const string HTTP_STATUS_501 = "501 Not Implemented";

        /// HTTP_STATUS_502 -> "502 Bad Gateway"
        public const string HTTP_STATUS_502 = "502 Bad Gateway";

        /// HTTP_STATUS_503 -> "503 Service Unavailable"
        public const string HTTP_STATUS_503 = "503 Service Unavailable";

        /// HTTP_STATUS_504 -> "504 Gateway Time-out"
        public const string HTTP_STATUS_504 = "504 Gateway Time-out";

        /// HTTP_STATUS_505 -> "505 HTTP Version Not Supported"
        public const string HTTP_STATUS_505 = "505 HTTP Version Not Supported";

        /// HTTP_STATUS_506 -> "506 Variant Also Negotiates"
        public const string HTTP_STATUS_506 = "506 Variant Also Negotiates";

        /// HTTP_STATUS_507 -> "507 Insufficient Storage"
        public const string HTTP_STATUS_507 = "507 Insufficient Storage";

        /// HTTP_STATUS_509 -> "509 Bandwidth Limit Exceeded"
        public const string HTTP_STATUS_509 = "509 Bandwidth Limit Exceeded";

        /// HTTP_STATUS_510 -> "510 Not Extended"
        public const string HTTP_STATUS_510 = "510 Not Extended";

        /// HTTP_STATUS_511 -> "511 Network Authentication Required"
        public const string HTTP_STATUS_511 = "511 Network Authentication Required";
    }

    [StructLayoutAttribute(LayoutKind.Sequential)]
    public class HaywireString
    {
        public HaywireString() { }
        public HaywireString(String content)
        {
            // TODO: add memory check
            IntPtr str = Marshal.StringToHGlobalAnsi(content);

            this.value = str;
            this.length = (uint)content.Length;
        }
        /// char*
        //[MarshalAsAttribute(UnmanagedType.LPStr)]
        public IntPtr value;

        /// size_t->unsigned int
        public uint length;

        public static explicit operator HaywireString(String b)
        {
            return new HaywireString(b);
        }
        public static explicit operator String(HaywireString b)
        {
            return Marshal.PtrToStringAnsi(b.value, (int)b.length);
        }
        public override string ToString()
        {
            return Marshal.PtrToStringAnsi(this.value, (int)this.length);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct HaywireConfiguration
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string ListenAddress;
        public int ListenPort;
    }

    [StructLayoutAttribute(LayoutKind.Sequential)]
    public struct HaywireRequest
    {
        /// unsigned short
        public ushort http_major;

        /// unsigned short
        public ushort http_minor;

        /// unsigned char
        public byte method;

        /// int
        public int keep_alive;

        /// char*
        [MarshalAsAttribute(UnmanagedType.LPStr)]
        public string url;

        /// void*
        public IntPtr headers;

        /// char*
        [MarshalAsAttribute(UnmanagedType.LPStr)]
        public string body;

        /// int
        public int body_length;
    }

    public delegate void HaywireRequestCallback(HaywireRequest request, IntPtr response, IntPtr state);
    public delegate void HaywireResponseCompleteCallback(IntPtr state);

    public class HaywireInterop
    {
        [System.Security.SuppressUnmanagedCodeSecurity]
        [DllImport("haywire", EntryPoint = "hw_init_with_config", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int InitWithConfig(HaywireConfiguration config);

        [DllImport("haywire", EntryPoint = "hw_http_open", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Open(int threads);

        [DllImport("haywire", EntryPoint = "hw_http_add_route", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddRoute(string route, HaywireRequestCallback callback);

        [DllImport("haywire", EntryPoint = "hw_register_http_response_complete_callback", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddRoute(HaywireResponseCompleteCallback callback);

        [DllImport("haywire", EntryPoint = "hw_create_http_response", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr CreateResponse();

        [DllImport("haywire", EntryPoint = "hw_set_response_status_code", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetResponseStatusCode(IntPtr response, HaywireString statusCode);

        [DllImport("haywire", EntryPoint = "hw_set_body", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetResponseBody(IntPtr response, HaywireString statusCode);

        [DllImport("haywire", EntryPoint = "hw_set_response_header", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetResponseHeader(IntPtr response, HaywireString name, HaywireString value);

        [DllImport("haywire", EntryPoint = "hw_http_response_send", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SendResponse(IntPtr response, IntPtr state, HaywireResponseCompleteCallback completeCallback);

        [DllImport("haywire", EntryPoint = "hw_set_http_version", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetResponseHttpVersion(IntPtr response, UInt16 major, UInt16 minor);

    }
}
