1. Add launch scripts in ubuntu

a. create my_service.service under systemd/system.

[Unit]
Description=My custom startup service
 
[Service]
ExecStart=/bin/echo "Service started"
 
[Install]
WantedBy=multi-user.target

b. register my service with commands below
sudo systemctl daemon-reload
sudo systemctl enable my_service.service
sudo systemctl start my_service.service


c. check if service work normally.
journalctl -u my_service.service 