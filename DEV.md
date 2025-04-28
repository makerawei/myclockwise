如果出现ssl相关的编译错误，可以参考这个[issue](https://github.com/jnthas/clockwise/issues/90)  
Component config -> ESP-TLS -> Enable PSK verification  
Partition Table -> Partition Table -> Single factory app(large), no OTA (because the firmwere will be bigger after PSK enabled)  


