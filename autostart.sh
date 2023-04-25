#!/bin/bash

# Path and name of the executable file to be started
executable_file="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/build/udp_link"

echo "Do you want to set $executable_file as a startup service? (Y/n)"
read choice

if [[ $choice =~ ^[Yy]$ ]]; then
  # Create the service file
  sudo bash -c "cat >/etc/systemd/system/udp_link.service" <<EOL
[Unit]
Description=udp_link service
After=network.target

[Service]
ExecStart=$executable_file
Restart=always
User=pi

[Install]
WantedBy=multi-user.target
EOL

  # Reload the systemd daemon configuration file
  sudo systemctl daemon-reload

  # Enable the startup service
  sudo systemctl enable udp_link.service

  echo "$executable_file has been set as a startup service."
else
  # Disable the startup service
  sudo systemctl disable udp_link.service

  echo "$executable_file is no longer a startup service."
fi
