using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace haywire
{
    public class HaywireResponse
    {
        public IntPtr handle;

        public HaywireResponse(IntPtr handle)
        {
            //handle = HaywireInterop.CreateResponse();
            this.handle = handle;
        }

        public void SetCode(string statusCode)
        {
            // TODO: add memory check, store str somewhere 
            HaywireInterop.SetResponseStatusCode(handle, (HaywireString)statusCode);
        }

        public void SetHeader(string name, string value)
        {
            // TODO: add memory check, store str somewhere 
            HaywireInterop.SetResponseHeader(handle, (HaywireString)name, (HaywireString)value);
        }

        public void SetBody(string body)
        {
            // TODO: add memory check, store str somewhere 
            HaywireInterop.SetResponseBody(handle, (HaywireString)body);
        }

        public void SetHttpVersion(int major, int minor)
        {
            HaywireInterop.SetResponseHttpVersion(this.handle, (UInt16)major, (UInt16)minor);
        }


        public void Send(HaywireResponseCompleteCallback callback)
        {
            HaywireInterop.SendResponse(this.handle, IntPtr.Zero, callback);
        }

        //public Task<HaywireResponse> SendAsync(Func<HaywireResponse> function) 
        //{ 
        //    if (function == null)
        //    {
        //        throw new ArgumentNullException("function");
        //    }

        //    var tcs = new TaskCompletionSource<HaywireResponse>(); 
        //    try
        //    {
        //        HaywireInterop.SendResponse(this.handle, IntPtr.Zero);
        //        tcs.SetResult(this);
        //    }
        //    catch (Exception exc)
        //    {
        //        tcs.SetException(exc);
        //    } 

        //    return tcs.Task; 
        //}




    }
}
