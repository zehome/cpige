====================================
cPige - archiving tool for webradios
====================================

Quick start
===========

```shell
wget https://github.com/zehome/cpige/releases/download/cpige-1.6/{cpige_linux_x86_64,cpige.conf.example}
chmod +x cpige_linux_x86_64
# create a configuration file
cp cpige.conf.example cpige.conf
vim cpige.conf
# :x
./cpige_linux_x86_64 -c cpige.conf
```

Command line options
====================
```
    -c path/to/cpige.conf
```

Download
========

https://github.com/zehome/cpige/releases/latest

Using the GUI
=============
A GTK2 GUI is now available for cPige, but you'll need to build it yourself:
```shell
cd gui; ./configure && make
```

now, copy cpige binary somewere the GUI will find in PATH, then launch
cpigeGUI.
