# Linux Setup

## WebHID Device Permission

WebHID still needs the operating system to allow the browser process to open the
HID raw device. The RP2040 appears as:

```text
cafe:4020 keyremap RP2040-Zero Keyremap Config
/dev/hidrawN root:root 0600
```

Install the udev rule:

```sh
sudo cp udev/60-keyremap.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then unplug and replug the RP2040.

Check:

```sh
ls -l /dev/hidraw*
udevadm info -q property -n /dev/hidrawN | sort
```

The keyremap hidraw device should become accessible to the logged-in user,
usually through `TAG+="uaccess"` and/or the `plugdev` group.

## Web Page

GitHub Pages is a valid target because it serves HTTPS. WebHID requires a secure
context, so these are valid:

```text
https://<user>.github.io/<repo>/
http://localhost:<port>/
```

For local testing from this repository:

```sh
make web-start
```

Open:

```text
http://localhost:8000
```

Stop the local server with:

```sh
make web-stop
```

Use Chrome or Edge, click `Connect`, and select:

```text
RP2040-Zero Keyremap Config
```

After connection, the page should show board and firmware info. The LED color
buttons should change the on-board WS2812 color.
