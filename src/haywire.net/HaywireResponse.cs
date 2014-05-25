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
            IntPtr str = Marshal.StringToHGlobalAnsi(statusCode);
            
            // TODO: add memory check, store str somewhere 
            var code = new HaywireString();
            code.value = str; 
            code.length = (uint)statusCode.Length; 
            HaywireInterop.SetResponseStatusCode(handle, code); 
        }

        public void SetHeader(string name, string value)
        {
            IntPtr nameHandle = Marshal.StringToHGlobalAnsi(name);

            // TODO: add memory check, store str somewhere 
            var nameString = new HaywireString();
            nameString.value = nameHandle;
            nameString.length = (uint)name.Length;

            IntPtr valueHandle = Marshal.StringToHGlobalAnsi(value);
            var valueString = new HaywireString();
            valueString.value = valueHandle;
            valueString.length = (uint)value.Length;

            HaywireInterop.SetResponseHeader(handle, nameString, valueString);
        }

        public void SetBody(string body)
        {
            IntPtr str = Marshal.StringToHGlobalAnsi(body);

            // TODO: add memory check, store str somewhere 
            var code = new HaywireString();
            code.value = str;
            code.length = (uint)body.Length;
            HaywireInterop.SetResponseBody(handle, code);
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
