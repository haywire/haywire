using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using haywire;

namespace hello_haywire
{
    class Program
    {
        static int Main(string[] args)
        {



            String route = "/";
            HaywireConfiguration config;
            config.ListenAddress = "0.0.0.0";
            if (args.Length > 1)
            {
                config.ListenPort = Convert.ToInt32(args[1]);
            }
            else
            {
                config.ListenPort = 8000;
            }

            /* hw_init_from_config("hello_world.conf"); */
            HaywireServer server = new HaywireServer(config);



            server.AddRoute(route, GetRoot);
            server.StartAcceptingRequests();
            return 0;


        }

        public static void ResponseCompleteCallback(IntPtr state)
        {
        }

        public static void GetRoot(HaywireRequest request, IntPtr response, IntPtr state)
        {

            HaywireResponse resp = new HaywireResponse(response);

            //hw_string status_code;
            //hw_string content_type_name;
            //hw_string content_type_value;
            //hw_string body;
            //hw_string keep_alive_name;
            //hw_string keep_alive_value;

            //SETSTRING(status_code, HTTP_STATUS_200);
            //hw_set_response_status_code(response, &status_code);
            resp.SetCode(StatusCodes.HTTP_STATUS_200);

            // SETSTRING(content_type_name, "Content-Type");
            // SETSTRING(content_type_value, "text/html");
            // hw_set_response_header(response, &content_type_name, &content_type_value);
            resp.SetHeader("Content-Type", "text/html");

            // SETSTRING(body, "hello world");
            // hw_set_body(response, &body);
            resp.SetBody("hello world");

            if (request.keep_alive > 0)
            {
                //SETSTRING(keep_alive_name, "Connection");
                //SETSTRING(keep_alive_value, "Keep-Alive");
                //hw_set_response_header(response, &keep_alive_name, &keep_alive_value);

                resp.SetHeader("Connection", "Keep-Alive");
            }
            else
            {
                // hw_set_http_version(response, 1, 0);
                resp.SetHttpVersion(1, 0);
            }

            //hw_http_response_send(response, "user_data", response_complete);
            resp.Send(ResponseCompleteCallback);
        }



    }





}
