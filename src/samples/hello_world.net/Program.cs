using System;

namespace haywire.samples.helloworld
{
    class Program
    {
        private static HaywireRequestCallback rootCallback;

        static void Main(string[] args)
        {
            HaywireConfiguration config;
            config.ListenAddress = "0.0.0.0";

            if (args.Length > 0)
            {
                config.ListenPort = int.Parse(args[0]);
            }
            else
            {
                config.ListenPort = 8000;
            }

            var server = new HaywireServer(config);
            rootCallback = new HaywireRequestCallback(GetRoot);
            server.AddRoute("/", rootCallback);
            server.StartAcceptingRequests();
        }

        private static void GetRoot(HaywireRequest request, IntPtr response, IntPtr state)
        {
            var resp = new HaywireResponse(response);
            resp.SetCode(StatusCodes.HTTP_STATUS_200);
            resp.SetHeader("Content-Type", "text/html");
            resp.SetHeader("Connection", "Keep-Alive");
            resp.SetHeader("foo", "bar");
            resp.SetBody("hello world");
            resp.Send(ResponseComplete);
        }

        private static void ResponseComplete(IntPtr state)
        {
            
        }
    }
}
