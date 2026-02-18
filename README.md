# Spooler Controller

## Introduction

This software is for controlling the cable driven parallel robot I built for my house. It uses DARTT over UDP to communicate with the actuators, which are using ESP32 bridges to provide networked communication to each of the actuators. 

## Comms Structure

The software uses UDP sockets to communicate with the actuators. This communication model should work well for distributed DARTT actuators - this software serves as a proof of concept. 