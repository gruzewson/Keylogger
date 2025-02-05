# Keylogger
Linux keylogger as kernel module with python server to acquire pressed keyboard keys.

## Build and Run
### Build the module:
```
make
```
### Insert the module:
```
sudo insmod keylogger_module.ko
```
### Run the server:
```
sudo python3 server.py
```

## Clean Up
### Remove the module:
```
sudo rmmod keylogger_module
```
### Clean build files:
```
make clean
```
