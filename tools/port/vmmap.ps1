param([int]$ProcId, [int]$Top = 18)
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Collections.Generic;
public class VM {
  [StructLayout(LayoutKind.Sequential)] public struct MBI { public IntPtr Base, AllocBase; public uint AllocProt; public ulong Size; public uint State, Prot, Type; }
  [DllImport("kernel32.dll")] public static extern IntPtr OpenProcess(uint a, bool b, int pid);
  [DllImport("kernel32.dll")] public static extern int VirtualQueryEx(IntPtr h, IntPtr a, out MBI m, int len);
  [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
  public static List<MBI> Dump(int pid) {
    var l = new List<MBI>(); IntPtr h = OpenProcess(0x0410, false, pid); ulong a = 0; MBI m;
    while (VirtualQueryEx(h, (IntPtr)(long)a, out m, Marshal.SizeOf(typeof(MBI))) != 0) { l.Add(m); a = (ulong)(long)m.Base + m.Size; if (a >= 0x80000000UL) break; }
    CloseHandle(h); return l; }
}
"@
$r = [VM]::Dump($ProcId)
$sum = @{}
foreach ($m in $r) {
    $st = switch ($m.State) { 0x1000 {'commit'} 0x2000 {'reserve'} default {'free'} }
    $ty = switch ($m.Type) { 0x1000000 {'image'} 0x40000 {'mapped'} 0x20000 {'private'} default {'-'} }
    $k = "$st/$ty"; if (-not $sum[$k]) { $sum[$k] = 0 }; $sum[$k] += $m.Size
}
$sum.GetEnumerator() | Sort-Object Name | ForEach-Object { '{0,-18} {1,8:N0} MB' -f $_.Name, ($_.Value / 1MB) }
'--- biggest non-free regions'
$r | Where-Object { $_.State -ne 0x10000 } | Sort-Object Size -Descending | Select-Object -First $Top | ForEach-Object {
    '{0,10:X} {1,7:N0} MB {2} {3}' -f [long]$_.Base, ($_.Size / 1MB), $(if ($_.State -eq 0x1000) {'commit'} else {'reserve'}), $(switch ($_.Type) { 0x1000000 {'image'} 0x40000 {'mapped'} 0x20000 {'private'} default {'-'} })
}
$free = ($r | Where-Object { $_.State -eq 0x10000 } | Measure-Object Size -Sum).Sum
$maxfree = ($r | Where-Object { $_.State -eq 0x10000 } | Measure-Object Size -Maximum).Maximum
'free total {0:N0} MB, largest free block {1:N0} MB' -f ($free / 1MB), ($maxfree / 1MB)
