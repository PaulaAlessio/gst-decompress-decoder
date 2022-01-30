# gzdec decoder


 - provides a GStreamer plugin to decompress gzip streams
 - compatible with GStreamer-0.10 and GStreamer-1.0

## Requirements
 - zlib

## Build
 Do autogen.sh
configure option 

```
--with-gstreamer-api=1.0/0.10]
```

you can install with 
```
sudo make install
```

## Usage

```
# GStreamer-0.10
gst-launch-0.10 filesrc location=file.txt.gz ! gzdec ! filesink location="file.txt"
# Gstreamer-1.0
gst-launch-1.0 filesrc location=file.txt.gz ! gzdec ! filesink location="file.txt"
```

## Test
The generated stream must be the same as doing
