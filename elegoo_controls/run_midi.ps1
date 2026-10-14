param(
    [string]$Port,
    [int]$BaudRate = 115200,
    [int]$MidiDeviceId = 0,
    [ValidateRange(1, 16)]
    [int]$Channel = 1,
    [switch]$ListDevices
)

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class WinMidi
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    private struct MidiOutCaps
    {
        public ushort ManufacturerId;
        public ushort ProductId;
        public uint DriverVersion;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string ProductName;

        public ushort Technology;
        public ushort Voices;
        public ushort Notes;
        public ushort ChannelMask;
        public uint Support;
    }

    [DllImport("winmm.dll")]
    private static extern uint midiOutGetNumDevs();

    [DllImport("winmm.dll", CharSet = CharSet.Auto)]
    private static extern uint midiOutGetDevCaps(
        UIntPtr deviceId,
        out MidiOutCaps capabilities,
        uint capabilitiesSize);

    [DllImport("winmm.dll")]
    private static extern uint midiOutOpen(
        out IntPtr handle,
        uint deviceId,
        IntPtr callback,
        IntPtr instance,
        uint flags);

    [DllImport("winmm.dll")]
    private static extern uint midiOutShortMsg(IntPtr handle, uint message);

    [DllImport("winmm.dll")]
    private static extern uint midiOutReset(IntPtr handle);

    [DllImport("winmm.dll")]
    private static extern uint midiOutClose(IntPtr handle);

    public static uint DeviceCount
    {
        get { return midiOutGetNumDevs(); }
    }

    public static string GetDeviceName(uint deviceId)
    {
        MidiOutCaps capabilities;
        uint result = midiOutGetDevCaps(
            new UIntPtr(deviceId),
            out capabilities,
            (uint)Marshal.SizeOf(typeof(MidiOutCaps)));

        if (result != 0)
            throw new InvalidOperationException("Could not inspect MIDI device " + deviceId + ". Error " + result + ".");

        return capabilities.ProductName;
    }

    public static IntPtr Open(uint deviceId)
    {
        IntPtr handle;
        uint result = midiOutOpen(out handle, deviceId, IntPtr.Zero, IntPtr.Zero, 0);
        if (result != 0)
            throw new InvalidOperationException("Could not open MIDI device " + deviceId + ". Error " + result + ".");

        return handle;
    }

    public static void Send(IntPtr handle, int status, int note, int velocity)
    {
        uint message = (uint)(status | (note << 8) | (velocity << 16));
        uint result = midiOutShortMsg(handle, message);
        if (result != 0)
            throw new InvalidOperationException("Could not send MIDI message. Error " + result + ".");
    }

    public static void Reset(IntPtr handle)
    {
        midiOutReset(handle);
    }

    public static void Close(IntPtr handle)
    {
        midiOutClose(handle);
    }
}
'@

$deviceCount = [WinMidi]::DeviceCount

if ($ListDevices) {
    if ($deviceCount -eq 0) {
        Write-Host "No Windows MIDI output devices were found."
    }
    else {
        for ($id = 0; $id -lt $deviceCount; $id++) {
            Write-Host "$id`: $([WinMidi]::GetDeviceName($id))"
        }
    }
    exit 0
}

if ([string]::IsNullOrWhiteSpace($Port)) {
    throw "Specify the Uno's COM port with -Port, for example: -Port COM3"
}

if ($MidiDeviceId -lt 0 -or $MidiDeviceId -ge $deviceCount) {
    throw "MIDI device $MidiDeviceId was not found. Run this script with -ListDevices to see the available IDs."
}

$serial = $null
$midiHandle = [IntPtr]::Zero

try {
    $midiHandle = [WinMidi]::Open($MidiDeviceId)
    $serial = [System.IO.Ports.SerialPort]::new($Port, $BaudRate)
    $serial.NewLine = "`n"
    $serial.ReadTimeout = 500
    $serial.DtrEnable = $true
    $serial.Open()

    # Opening the port resets most Uno boards. Give the sketch time to restart.
    Start-Sleep -Seconds 2
    Write-Host "Listening on $Port."
    Write-Host "Sending MIDI channel $Channel to: $([WinMidi]::GetDeviceName($MidiDeviceId))"
    Write-Host "Press Ctrl+C to stop."

    while ($true) {
        try {
            $parts = $serial.ReadLine().Trim() -split '\s+'

            if ($parts.Count -eq 3 -and
                ($parts[0] -eq "ON" -or $parts[0] -eq "OFF")) {
                $note = [int]$parts[1]
                $velocity = [int]$parts[2]

                if ($note -ge 0 -and $note -le 127 -and
                    $velocity -ge 0 -and $velocity -le 127) {
                    $status = if ($parts[0] -eq "ON") { 0x90 } else { 0x80 }
                    $status += $Channel - 1
                    [WinMidi]::Send($midiHandle, $status, $note, $velocity)
                }
            }
        }
        catch [System.TimeoutException] {
            # A timeout keeps Ctrl+C responsive when no button is pressed.
        }
    }
}
finally {
    if ($null -ne $serial) {
        if ($serial.IsOpen) {
            $serial.Close()
        }
        $serial.Dispose()
    }

    if ($midiHandle -ne [IntPtr]::Zero) {
        [WinMidi]::Reset($midiHandle)
        [WinMidi]::Close($midiHandle)
    }
}
