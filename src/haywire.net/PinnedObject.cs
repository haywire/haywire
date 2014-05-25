using System;
using System.Runtime.InteropServices;

namespace haywire
{
    /// <summary>
    /// A helper class for pinning a managed structure so that it is suitable for
    /// unmanaged calls. A pinned object will not be collected and will not be moved
    /// by the GC until explicitly freed.
    /// </summary>
    public class PinnedObject<T> : IDisposable where T : struct
    {
        protected T managedObject;
        protected GCHandle handle;
        protected IntPtr ptr;
        protected bool disposed;

        public T ManangedObject
        {
            get
            {
                return (T)handle.Target;
            }
            set
            {
                Marshal.StructureToPtr(value, ptr, false);
            }
        }

        public IntPtr Pointer
        {
            get { return ptr; }
        }

        public PinnedObject()
        {
            handle = GCHandle.Alloc(managedObject, GCHandleType.Pinned);
            ptr = handle.AddrOfPinnedObject();
        }

        ~PinnedObject()
        {
            Dispose();
        }

        public void Dispose()
        {
            if (!disposed)
            {
                handle.Free();
                ptr = IntPtr.Zero;
                disposed = true;
            }
        }
    }
}