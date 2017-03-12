#!/bin/bash

sudo ./ctrl -init ./kci_kmod.ko || {
	exit -1
}

sudo ./ctrl -pid "$1" || {
	exit -1
}
sudo ./ctrl -fd "$2" || {
	exit -1
}
sudo ./ctrl -start || {
	exit -1
}
sleep 40
echo "woke up from sleep"
sudo ./ctrl -stop || {
	exit -1
}
cat ./output
sudo ./ctrl -rm || {
	exit -1
}
