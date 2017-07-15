# USBGuard-Windows

#### Overview

The windows version disables system USB ports/devices using the registry values stored in 

```
HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\USB

HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\USBSTOR

```

A usefull reference for the technique is the Microsoft documentation:

1. [USB Device Registry Entries](https://msdn.microsoft.com/en-us/library/windows/hardware/jj649944(v=vs.85).aspx)

2. [USB Device Descriptors](https://msdn.microsoft.com/en-us/library/windows/hardware/ff539283(v=vs.85).aspx)

<b> Not finished yet, right now you can only fully enable or fully disable USB ports, no inbetween. </b>


#### Compiling w/Linux Subsystem

To create Windows executables in the linux subsystem, you need to install mingw cross-compiler:

```
sudo apt-get install mingw-w64
```

Then you can create 32-bit Windows executables using the makefile with:

```
make 32bit
```

And 64-bit Windows executables with:

```
make 64bit
```

#### Usage

Run the resulting executable and choose from the available options:

```
.\USBGuard.exe
```

#### References

Microsoft documentation:

1. [USB Device Registry Entries](https://msdn.microsoft.com/en-us/library/windows/hardware/jj649944(v=vs.85).aspx)

2. [USB Device Descriptors](https://msdn.microsoft.com/en-us/library/windows/hardware/ff539283(v=vs.85).aspx)