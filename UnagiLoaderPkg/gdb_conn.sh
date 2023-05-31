#!/bin/bash
gdb -ex "target remote 127.0.0.1:1234" -ex "b *UefiMain" -ex "layout split" 
