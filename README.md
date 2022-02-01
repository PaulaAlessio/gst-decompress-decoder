# gzdec decoder

 - This project provides a GStreamer plugin to decompress gzip streams
 - compatible with GStreamer-0.10 and GStreamer-1.0

## Requirements
 - GStreamer 0.10 (>0.10.3) or GStreamer 1.0 (>1.0.0)
 - zlib (>1.2)

## Build and install

Type:
```
autogen.sh
```
This will configure the project for GStreamer 1.0. If you can use the option `--with-gstreamer-api` to choose
between the two versions

```
./configure --with-gstreamer-api=0.10
```
Build the project with `make` (type `make clean` beforehand if it had been already build)
and install it in your system with 
```
sudo make install
```

You can clean the build with, 
```
make clean
```

If you want to clean the build and configuration files, type
```
make distclean
```

## Usage

```
# GStreamer-0.10
gst-launch-0.10 filesrc location=file.txt.gz ! gzdec ! filesink location="file.txt"
# Gstreamer-1.0
gst-launch-1.0 filesrc location=file.txt.gz ! gzdec ! filesink location="file.txt"
```