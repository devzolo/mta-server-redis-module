FROM ubuntu:latest

RUN apt-get upgrade -y \
 && dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install gcc yarn nodejs npm zip bash tar unzip lib32z1 lib32ncurses5 screen wget curl mysql-client -y \
 && apt-get clean

RUN npm install -g yarn

RUN mkdir /usr/src/module

WORKDIR /usr/src/module
