# auto_mount
Simple daemon for automounting usb drives. It creates a process that reads the output of
```bash
udevadm monitor --udev -s block
```
and if there is a device addition it just runs 
```bash
udisksctl mount -b device_path
```

---

## Create a systemd unit
For starting the program at startup and stoping any time you can create a simple systemd service.
### Step 1: compile 
Inside the project's folder run 
```bash
make
```

### Step 2: create links
Create a symbolic link for the program in the /usr/bin folder
```bash
ln -s /home/$USER/path_to_project/bin/auto_mount /usr/bin/auto_mount
```

Also if you want you can create a link for the auto_mount_stop script for killing the daemon
```bash
ln -s /home/$USER/path_to_project/scipts/auto_mount_stop /usr/bin/auto_mount_stop
```

### Step 3: create the service file
Inside /etc/systemd/system folder add a file auto_mount@.service and write into it
```bash
[Unit]
Description=Auto mount devices

[Service]
User=%I
Type=forking
ExecStart=/usr/bin/auto_mount
ExecStop=/usr/bin/auto_mount_stop
RemainAfterExit=no

[Install]
WantedBy=multi-user.target
```

### Step 4: enable the service
You can enable the service with 
```bash
sudo systemctl enable auto_mount@$USER
```
And whenever you want you can stop it with 
```bash 
sudo systemctl stop auto_mount@$USER
```
