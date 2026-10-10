param([string]$Out)
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W { [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r); [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; } }
"@
Add-Type -AssemblyName System.Drawing
$p = Get-Process BO1Zombies -ErrorAction Stop | Select-Object -First 1
[W]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 800
$fg = [W+RECT]::new(); [W]::GetWindowRect($p.MainWindowHandle, [ref]$fg) | Out-Null
$w = $fg.R - $fg.L; $h = $fg.B - $fg.T
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($fg.L, $fg.T, 0, 0, (New-Object System.Drawing.Size $w, $h))
$bmp.Save($Out)
"saved ${w}x${h} title=[$($p.MainWindowTitle)]"
