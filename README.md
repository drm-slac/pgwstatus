# pgwstatus

# Purpose

This software displays the status of a running EPICS 7 (PVAccess) Gateway.

# Command Line Options

Usage: %s [OPTIONS] gateway-name-prefix ...

> [!IMPORTANT]
> The 'gateway-name-prefix' is the value of the gateway's
*statusprefix* entry in its configuration file.

The following options are supported:
  -h   print this message then exit.
  -r   Include the rates measured for requests, both coming
       into the gateway, and then from the gateway to IOCs
  -m N Report only the top 'N' transfer rates for PVs and Computers
  -c N Report only the top 'N' transfer rates for Computers
  -p N Report only the top 'N' transfer rates for PVs

