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

public static class ChordPanMidi
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

    public static uint DeviceCount { get { return midiOutGetNumDevs(); } }

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

    public static void Send(IntPtr handle, int status, int data1, int data2)
    {
        uint message = (uint)(status | (data1 << 8) | (data2 << 16));
        uint result = midiOutShortMsg(handle, message);
        if (result != 0)
            throw new InvalidOperationException("Could not send MIDI message. Error " + result + ".");
    }

    public static void Reset(IntPtr handle) { midiOutReset(handle); }
    public static void Close(IntPtr handle) { midiOutClose(handle); }
}
'@

$deviceCount = [ChordPanMidi]::DeviceCount

if ($ListDevices) {
    if ($deviceCount -eq 0) {
        Write-Host "No Windows MIDI output devices were found."
    }
    else {
        for ($id = 0; $id -lt $deviceCount; $id++) {
            Write-Host "$id`: $([ChordPanMidi]::GetDeviceName($id))"
        }
    }
    exit 0
}

if ([string]::IsNullOrWhiteSpace($Port)) {
    throw "Specify the Uno's COM port with -Port, for example: -Port COM3"
}

if ($MidiDeviceId -lt 0 -or $MidiDeviceId -ge $deviceCount) {
    throw "MIDI device $MidiDeviceId was not found. Run with -ListDevices to see IDs."
}

$serial = $null
$midiHandle = [IntPtr]::Zero

try {
    $midiHandle = [ChordPanMidi]::Open($MidiDeviceId)
    $serial = [System.IO.Ports.SerialPort]::new($Port, $BaudRate)
    $serial.NewLine = "`n"
    $serial.ReadTimeout = 500
    $serial.DtrEnable = $true
    $serial.Open()

    Start-Sleep -Seconds 2
    Write-Host "Listening on $Port."
    Write-Host "Sending MIDI channel $Channel to: $([ChordPanMidi]::GetDeviceName($MidiDeviceId))"
    Write-Host "Turn the pan knob or press a chord button. Press Ctrl+C to stop."

    while ($true) {
        try {
            $parts = $serial.ReadLine().Trim() -split '\s+'
            if ($parts.Count -ne 3) { continue }

            $data1 = 0
            $data2 = 0
            if (-not [int]::TryParse($parts[1], [ref]$data1) -or
                -not [int]::TryParse($parts[2], [ref]$data2) -or
                $data1 -lt 0 -or $data1 -gt 127 -or
                $data2 -lt 0 -or $data2 -gt 127) {
                continue
            }

            $statusBase = switch ($parts[0]) {
                "ON"  { 0x90 }
                "OFF" { 0x80 }
                "CC"  { 0xB0 }
                default { $null }
            }

            if ($null -ne $statusBase) {
                [ChordPanMidi]::Send(
                    $midiHandle,
                    $statusBase + $Channel - 1,
                    $data1,
                    $data2)
            }
        }
        catch [System.TimeoutException] {
            # A timeout keeps Ctrl+C responsive while the controller is idle.
        }
    }
}
finally {
    if ($null -ne $serial) {
        if ($serial.IsOpen) { $serial.Close() }
        $serial.Dispose()
    }

    if ($midiHandle -ne [IntPtr]::Zero) {
        [ChordPanMidi]::Reset($midiHandle)
        [ChordPanMidi]::Close($midiHandle)
    }
}
