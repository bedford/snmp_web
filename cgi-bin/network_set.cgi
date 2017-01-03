#!/bin/sh

printf "Content type: text/html; charset=utf-8\n\n"
cp /opt/work/boa_web/www/param/device_param_set.json /opt/work/boa_web/www/param/device_param.json 
cat /opt/work/boa_web/www/param/device_param_set.json

