# auto_mount
Simple daemon for automounting usb drives. It creates a process that reads the output of
```bash
udevadm monitor --udev -s block
```
and if there is a device addition it just runs 
```bash
udisksctl mount -b device_path
```


