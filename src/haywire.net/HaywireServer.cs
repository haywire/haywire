using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace haywire
{
    public class HaywireServer
    {
        public HaywireServer(HaywireConfiguration configuration)
        {
            HaywireInterop.InitWithConfig(configuration);
        }

        public void AddRoute(string route, HaywireRequestCallback callback)
        {
            HaywireInterop.AddRoute(route, callback);
        }

        public void StartAcceptingRequests()
        {
            HaywireInterop.Open(0);
        }
    }
}
