import ctypes

class PASSTHRU_MSG(ctypes.Structure):
    _fields_ = [("ProtocolID", ctypes.c_ulong), ("RxStatus", ctypes.c_ulong),
                ("TxFlags", ctypes.c_ulong), ("Timestamp", ctypes.c_ulong),
                ("DataSize", ctypes.c_ulong), ("ExtraDataIndex", ctypes.c_ulong),
                ("Data", ctypes.c_ubyte * 4128)]

lib = ctypes.CDLL(r"C:\test\OBDXVX_J2534.dll")   # the proxy
lib.PassThruOpen.restype = ctypes.c_long
# ... then simulate a session: Open, ReadVersion, Connect, filter, Write, Read, Close
